/* SPDX-License-Identifier: GPL-3.0-or-later
   SPDX-FileCopyrightText: Copyright © 2025 Erez Geva <ErezGeva2@gmail.com> */

/** @file
 * @brief single liked list
 *
 * @author Erez Geva <ErezGeva2@@gmail.com>
 * @copyright © 2025 Erez Geva
 *
 */

#include "src/slist.h"
#include "src/log.h"

static inline pnode _getFreeNode(pslistmgr self, pnode nxt)
{
    pnode ret;
    if(self->_freeList._head == NULL) {
        size_t l = self->_dataSize + sizeof(struct node_t);
        ret = malloc(l);
        if(ret == NULL)
            return NULL;
        /**
         * Ensure a new allocated node is clean
         * in case it is used with additional allocations
         */
        memset(ret, 0, l);
    } else { /* Fetch node from free nodes list */
        self->_freeCount--;
        ret = self->_freeList._head;
        self->_freeList._head = ret->_nxt;
    }
    ret->_nxt = nxt;
    self->_count++;
    return ret;
}
static inline void _freeNode(pslistmgr self, pnode node, bool useFreeList)
{
    if(self->_release != NULL)
        self->_release(node, &useFreeList);
    if(useFreeList) { /* Store node in free nodes list */
        node->_nxt = self->_freeList._head;
        self->_freeList._head = node;
        self->_freeCount++;
    } else /* Release node memory */
        free(node);
    self->_count--;
}
#define CALL_FUNC(func, node) callFunc(node, cookie, self->_##func)
static inline int callFunc(pnode node, void *cookie, sfunc_f func)
{
    return func(node->_data, cookie);
}
static inline void _freeList0(pslistmgr self, pslist list, bool useFreeList)
{
    pnode cur = list->_head;
    while(cur != NULL) {
        pnode nxt = cur->_nxt;
        _freeNode(self, cur, useFreeList);
        cur = nxt;
    }
}
static inline bool mlock(pslistmgr self)
{
    return self->_mtx->lock(self->_mtx);
}
static inline bool munlock(pslistmgr self)
{
    return self->_mtx->unlock(self->_mtx);
}
static inline bool mclock(pslistmgr self)
{
    if(LIKELY_COND(self != NULL && self->_mtx != NULL))
        return mlock(self);
    return false;
}

static void _free(pslistmgr self)
{
    if(LIKELY_COND(self != NULL)) {
        /* Free memory of nodes on free nodes list */
        _freeList0(self, &self->_freeList, false);
        if(LIKELY_COND(self->_mtx != NULL))
            self->_mtx->free(self->_mtx);
        free(self);
    }
}
static void _freeList(pslistmgr self, pslist list, bool useFreeList)
{
    if(LIKELY_COND(list != NULL) && mclock(self)) {
        _freeList0(self, list, useFreeList);
        munlock(self);
    }
}
static bool _updateNode(pslistmgr self, pslist list, void *cookie)
{
    int c;
    pnode cur, tmp, prv = NULL;
    if(UNLIKELY_COND(list == NULL || cookie == NULL) || !mclock(self) ||
        UNLIKELY_COND(self->_cmp == NULL || self->_set == NULL))
        return false;
    cur = list->_head;
    if(cur == NULL) {
        /* First node */
        cur = _getFreeNode(self, NULL);
        if(cur == NULL) {
            munlock(self);
            return false;
        }
        CALL_FUNC(set, cur);
        list->_head = cur;
        munlock(self);
        return true;
    }
    c = CALL_FUNC(cmp, cur);
    if(c < 0) {
        /* New head */
        tmp = _getFreeNode(self, cur);
        if(tmp == NULL) {
            munlock(self);
            return false;
        }
        CALL_FUNC(set, tmp);
        list->_head = tmp;
        munlock(self);
        return true;
    }
    for(;;) {
        if(c == 0) {
            /* Update node */
            CALL_FUNC(set, cur);
            munlock(self);
            return true;
        } else if(c > 0) {
            prv = cur;
            cur = cur->_nxt;
            if(cur == NULL) {
                /* New tail */
                tmp = _getFreeNode(self, NULL);
                if(tmp == NULL) {
                    munlock(self);
                    return false;
                }
                CALL_FUNC(set, tmp);
                prv->_nxt = tmp;
                munlock(self);
                return true;
            }
        } else {
            /* In middle */
            tmp = _getFreeNode(self, cur);
            if(tmp == NULL) {
                munlock(self);
                return false;
            }
            CALL_FUNC(set, tmp);
            prv->_nxt = tmp;
            munlock(self);
            return true;
        }
        c = CALL_FUNC(cmp, cur);
    }
    /* We never get here */
    munlock(self);
    return false;
}
static void *_fetchNode(pslistmgr self, pslist list, void *cookie)
{
    if(UNLIKELY_COND(list == NULL || cookie == NULL) || !mclock(self) ||
        UNLIKELY_COND(self->_cmp == NULL))
        return NULL;
    for(pnode cur = list->_head; cur != NULL; cur = cur->_nxt) {
        int c = CALL_FUNC(cmp, cur);
        if(c == 0) {
            munlock(self);
            return cur->_data;
        } else if(c < 0)
            break;
    }
    munlock(self);
    return NULL;
}
#define MLOCK do {if(!mlock(self))return cnt;}while(false)
#define MULOCK do {if(!munlock(self))return cnt;}while(false)
static size_t _cleanUpNodes(pslistmgr self, pslist list, void *cookie)
{
    size_t cnt;
    int r;
    pnode cur, prv;
    if(UNLIKELY_COND(self == NULL || list == NULL || cookie == NULL ||
            self->_mtx == NULL) || self->_cleanup == NULL)
        return 0;
    cnt = 0;
    /* remove head node if it need cleaning */
    for(;;) {
        MLOCK;
        cur = list->_head;
        if(cur == NULL) { /* empty list */
            MULOCK;
            return cnt;
        }
        r = CALL_FUNC(cleanup, cur);
        if(r) { /* Move head to next node */
            list->_head = cur->_nxt;
            _freeNode(self, cur, true);
            cnt++;
        } else {
            /* Continue to cleanup other nodes in list */
            MULOCK;
            break;
        }
        MULOCK;
    }
    /* Continue with other nodes,
       if updateNode() add notes they are new anyway,
       We take in acount that we are the only
       thread removing nodes from the list. */
    prv = cur;
    for(;;) {
        MLOCK;
        cur = prv->_nxt;
        if(cur == NULL) {
            MULOCK;
            return cnt;
        }
        r = CALL_FUNC(cleanup, cur);
        if(r) { /* remove node */
            prv->_nxt = cur->_nxt;
            _freeNode(self, cur, true);
            cnt++;
        } else /* Check next node */
            prv = cur;
        MULOCK;
    };
    return 0; /* we never get here! */
}
static size_t _getFreeNodes(pslistmgr self)
{
    size_t ret;
    if(!mclock(self))
        return 0;
    ret = self->_freeCount;
    munlock(self);
    return ret;
}
static size_t _getUsedNodes(pslistmgr self)
{
    size_t ret;
    if(!mclock(self))
        return 0;
    ret = self->_count;
    munlock(self);
    return ret;
}
pslistmgr pslistmgr_alloc(size_t dataSize, sfunc_f cmp, sfunc_f set,
    sfunc_f cleanup, sfunc_f release)
{
    if(UNLIKELY_COND(dataSize == 0 || cmp == NULL || set == NULL))
        return NULL;
    pslistmgr ret = malloc(sizeof(struct s_link_list_mgr_t));
    if(ret != NULL) {
        ret->_mtx = mutex_alloc();
        if(ret->_mtx == NULL) {
            free(ret);
            return NULL;
        }
#define asg(a) ret->a = _##a
        /* methods */
        asg(free);
        asg(freeList);
        asg(updateNode);
        asg(fetchNode);
        asg(cleanUpNodes);
        asg(getFreeNodes);
        asg(getUsedNodes);
#undef asg
#define asg(a) ret->_##a = a
        /* Data size of a node */
        asg(dataSize);
        /* User callbacks */
        asg(cmp);
        asg(set);
        asg(cleanup);
        asg(release);
        /* Counters */
        ret->_freeCount = 0;
        ret->_count = 0;
        /* Free node list */
        ret->_freeList._head = NULL;
    } else
        log_err("memory allocation failed");
    return ret;
}
