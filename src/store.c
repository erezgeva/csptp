/* SPDX-License-Identifier: GPL-3.0-or-later
   SPDX-FileCopyrightText: Copyright © 2025 Erez Geva <ErezGeva2@gmail.com> */

/** @file
 * @brief store time stamps for follow-up messages
 *
 * @author Erez Geva <ErezGeva2@@gmail.com>
 * @copyright © 2025 Erez Geva
 *
 */

#include "src/store.h"
#include "src/log.h"
#include "src/swap.h"

struct _nodeIPData {
    struct timespec ts; /* Last message TX time */
    time_t last; /* Last time in seconds we store tx time of a client */
    uint16_t sequenceId; /* The sequence used with the TS */
    /* uint8_t domainNumber; */ /* Domain number used by client */
    uint8_t ip[]; /* Address of client */
};
struct _cookie_t {
    pcts ts;
    uint16_t sequenceId; /* The sequence used with the TS */
    /* uint8_t domainNumber; */ /* Domain number used by client */
    time_t last;
    const void *ip;
};
typedef struct _nodeIPData *pnodeData;
typedef struct _cookie_t *pcookie;

#define PER_IP(n)\
    static int _cmp##n(void *d, void *c)\
    {\
        return memcmp(((pnodeData)d)->ip, ((pcookie)c)->ip, IPV##n##_ADDR_LEN);\
    }\
    static int _set##n(void *_d, void *_c)\
    {\
        pnodeData d = (pnodeData)_d;\
        pcookie c = (pcookie)_c;\
        c->ts->toTimespec(c->ts, &d->ts);\
        d->last = time(NULL);\
        d->sequenceId = c->sequenceId;\
        /* d->domainNumber = c->domainNumber; */\
        memcpy(d->ip, c->ip, IPV##n##_ADDR_LEN);\
        return 0;\
    }
PER_IP(4)
PER_IP(6)
static int _cleanUp(void *d, void *c)
{
    /* Time is too old */
    return ((pnodeData)d)->last < ((pcookie)c)->last;
}
size_t _hash4(pcstore self, pcipaddr addr)
{
    return addr->getIP4Val(addr) & self->_hashMask;
}
size_t _hash6(pcstore self, pcipaddr addr)
{
    /* Convert low 4 octets to host unsigned 32 bits
       and use the hash bits */
    return net_to_cpu32(*(uint32_t *)(addr->getIP6(addr) + 12)) & self->_hashMask;
}
size_t _hash0(pcstore self, pcipaddr addr)
{
    return 0; /* We do not use hash, we use single list */
}

static void _free(pstore self)
{
    if(LIKELY_COND(self != NULL)) {
        pslistmgr mgr = self->_mgr;
        if(LIKELY_COND(mgr != NULL)) {
            if(LIKELY_COND(self->_data != NULL)) {
                pslist l = self->_data;
                for(size_t i = 0; i < self->_hashSize; i++)
                    mgr->freeList(mgr, l++, false);
            }
            mgr->free(mgr);
        }
        free(self);
    }
}
static bool _update(pstore self, pcipaddr addr, pcts ts, uint16_t sID,
    uint8_t dNum)
{
    struct _cookie_t cookie;
    if(UNLIKELY_COND(self == NULL || addr == NULL || ts == NULL ||
            self->_data == NULL || self->_mgr == NULL))
        return false;
    cookie.ts = ts;
    cookie.last = 0;
    cookie.sequenceId = sID;
    /* cookie.domainNumber = dNum; */
    cookie.ip = addr->getIP(addr);
    return self->_mgr->updateNode(self->_mgr, self->_data + self->_hash(self, addr),
            &cookie);
}
static bool _fetch(pstore self, pcipaddr addr, pts ts, uint16_t sID,
    uint8_t dNum, bool clear)
{
    pnodeData d;
    struct _cookie_t cookie;
    if(UNLIKELY_COND(self == NULL || addr == NULL || ts == NULL ||
            self->_data == NULL || self->_mgr == NULL))
        return false;
    cookie.ts = NULL;
    cookie.last = 0;
    cookie.sequenceId = sID;
    /* cookie.domainNumber = dNum; */
    cookie.ip = addr->getIP(addr);
    d = (pnodeData)self->_mgr->fetchNode(self->_mgr,
            self->_data + self->_hash(self, addr), &cookie);
    if(d == NULL || d->sequenceId != sID /* || d->domainNumber != dNum */)
        return false;
    /* Get timestamp from node */
    ts->fromTimespec(ts, &d->ts);
    if(clear) {
        d->ts.tv_sec = 0;
        d->ts.tv_nsec = 0;
    }
    return true;
}
static size_t _cleanup(pstore self, uint32_t timeGap)
{
    pslist l;
    size_t count;
    struct _cookie_t cookie;
    if(UNLIKELY_COND(self == NULL || self->_data == NULL || self->_mgr == NULL))
        return 0;
    l = self->_data;
    cookie.ts = NULL;
    cookie.last = time(NULL) - timeGap;
    cookie.sequenceId = 0;
    /* cookie.domainNumber = 0; */
    cookie.ip = NULL;
    count = 0;
    for(size_t i = 0; i < self->_hashSize; i++)
        count += self->_mgr->cleanUpNodes(self->_mgr, l++, &cookie);
    return count;
}
size_t _getHashSize(pcstore self)
{
    return UNLIKELY_COND(self == NULL) ? 0 : self->_hashSize;
}
size_t _records(pcstore self)
{
    return UNLIKELY_COND(self == NULL || self->_mgr == NULL) ? 0 :
        self->_mgr->getUsedNodes(self->_mgr);
}

pstore store_alloc(prot type, size_t hashBitsSize)
{
    pstore ret;
    pslistmgr mgr;
    size_t hashSize, hashAllocSize, dataSize = sizeof(struct _nodeIPData);
    switch(type) {
        case UDP_IPv4:
            dataSize += IPV4_ADDR_LEN;
            break;
        case UDP_IPv6:
            dataSize += IPV6_ADDR_LEN;
            break;
        default:
            log_err("protocol not supported %d", type);
            return NULL;
    }
    if(hashBitsSize > 32) {
        log_warning("hash exceed 32 bits\n");
        return NULL;
    }
    hashSize = 1 << hashBitsSize;
    hashAllocSize = hashSize * sizeof(struct s_link_list_t);
    ret = malloc(sizeof(struct store_t) + hashAllocSize);
    if(ret == NULL)
        return NULL;
    switch(type) {
        case UDP_IPv4:
            ret->_hash = _hash4;
            mgr = pslistmgr_alloc(dataSize, _cmp4, _set4, _cleanUp, NULL);
            break;
        case UDP_IPv6:
            ret->_hash = _hash6;
            mgr = pslistmgr_alloc(dataSize, _cmp6, _set6, _cleanUp, NULL);
            break;
        default: /* Should not happen */
            mgr = NULL;
            break;
    }
    if(mgr == NULL) {
        free(ret);
        return NULL;
    }
    ret->_mgr = mgr;
    ret->_hashSize = hashSize;
    ret->_hashMask = hashSize - 1;
    if(hashBitsSize == 0)
        ret->_hash = _hash0;
#define asg(a) ret->a = _##a
    asg(free);
    asg(update);
    asg(fetch);
    asg(cleanup);
    asg(getHashSize);
    asg(records);
    ret->_data = (pslist)(ret + 1);
    memset(ret->_data, 0, hashAllocSize);
    return ret;
}
