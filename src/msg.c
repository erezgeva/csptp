/* SPDX-License-Identifier: GPL-3.0-or-later
   SPDX-FileCopyrightText: Copyright © 2025 Erez Geva <ErezGeva2@gmail.com> */

/** @file
 * @brief PTP message for CSPTP
 *
 * @author Erez Geva <ErezGeva2@@gmail.com>
 * @copyright © 2025 Erez Geva
 *
 */

#include "src/msg.h"
#include "src/log.h"
#include "src/swap.h"

static const size_t _msg_size = sizeof(struct msg_t);
static const size_t _tlv_hdr = sizeof(struct tlv_hdr_t);
static const size_t _csptp_req = sizeof(struct CSPTP_REQUEST_t);
static const uint8_t versionPTP = 2;
static const uint8_t minorVersionPTP = 1;
static const uint8_t majorSdoId = 0x3;
static const uint8_t minorSdoId = 0x00;
static const uint8_t twoStepsFlag = 1 << 1;
static const uint8_t unicastFlag = 1 << 2;
static const uint8_t zeroClockIdentity[8] = { 0 };

static inline bool _isOdd(size_t v) { return (v & 1) > 0; }
static inline size_t _makeEven(size_t v) { return v + (v & 1); }
static inline void detach(pmsg self)
{
    self->_msg = NULL;
    self->_buf = NULL;
    self->_len = 0;
    self->_left = 0;
    self->_end = NULL;
    self->_type = -1;
    self->_num_tlvs = 0;
}
static inline size_t _min_tlv_size(enum tlv_type_id id, bool high)
{
    switch(id) {
        case ALTERNATE_TIME_OFFSET_INDICATOR_id:
            return sizeof(struct ALTERNATE_TIME_OFFSET_INDICATOR_t);
        case CSPTP_REQUEST_id:
            return _csptp_req;
        case CSPTP_RESPONSE_id:
            return sizeof(struct CSPTP_RESPONSE_t);
        case CSPTP_STATUS_id:
            return sizeof(struct CSPTP_STATUS_t);
        case PAD_id:
            return _tlv_hdr;
        default:
            if(high)
                log_err("No such ID 0x%x", id);
            else
                log_info("No such ID 0x%x", id);
            return 0;
    }
}
static inline size_t _adderSize(prot p, bool high)
{
    switch(p) {
        case UDP_IPv4:
            return IPV4_ADDR_LEN;
        case UDP_IPv6:
            return IPV6_ADDR_LEN;
        default:
            if(high)
                log_err("protocol not supported %d", p);
            else
                log_info("protocol not supported %d", p);
            return 0;
    }
}
static inline size_t net_to_cpu_msg(struct msg_t *m)
{
    uint16_t len = net_to_cpu16(m->messageLength);
    m->messageLength = len;
    m->correctionField = net_to_cpu64(m->correctionField);
    /* Always zero
    m->messageTypeSpecific = net_to_cpu32(m->messageTypeSpecific);
    m->sourcePortIdentity.portNumber =
        net_to_cpu16(m->sourcePortIdentity.portNumber); */
    m->sequenceId = net_to_cpu16(m->sequenceId);
    net_to_cpu_ts(&m->timestamp);
    return len;
}
static inline void cpu_to_net_msg(struct msg_t *m, size_t len)
{
    m->messageLength = cpu_to_net16(len);
    m->correctionField = cpu_to_net64(m->correctionField);
    /* The value is not used by software
    m->messageTypeSpecific = cpu_to_net32(m->messageTypeSpecific); */
    /* Always zero
    m->sourcePortIdentity.portNumber =
        cpu_to_net16(m->sourcePortIdentity.portNumber); */
    m->sequenceId = cpu_to_net16(m->sequenceId);
    cpu_to_net_ts(&m->timestamp);
}
static inline enum tlv_type_id net_to_cpu_tlv(uint8_t *p, size_t size,
    size_t *tlv_len)
{
    size_t len, min_sz;
    enum tlv_type_id id;
    struct tlv_hdr_t *h = (struct tlv_hdr_t *)p;
    h->tlvType = net_to_cpu16(h->tlvType);
    h->lengthField = net_to_cpu16(h->lengthField);
    id = h->tlvType;
    len = h->lengthField + _tlv_hdr;
    min_sz = _min_tlv_size(id, false);
    if(min_sz == 0)
        return Invalid_tlv_ID;
    if(len < min_sz) {
        log_info("TLV to short 0x%x", id);
        return Invalid_tlv_ID;
    }
    if(len > size) {
        log_warning("TLV overflow message 0x%x", id);
        return Invalid_tlv_ID;
    }
    switch(id) {
        case ALTERNATE_TIME_OFFSET_INDICATOR_id: {
            struct ALTERNATE_TIME_OFFSET_INDICATOR_t *t =
                (struct ALTERNATE_TIME_OFFSET_INDICATOR_t *)h;
            /* Verify size match */
            if(len != min_sz + _makeEven(t->displayName.lengthField)) {
                log_warning("ALTERNATE_TIME_OFFSET_INDICATOR TLV with wrong size");
                return Invalid_tlv_ID;
            }
            t->currentOffset = net_to_cpu32(t->currentOffset);
            t->jumpSeconds = net_to_cpu32(t->jumpSeconds);
            net_to_cpu48(&t->timeOfNextJump);
            break;
        }
        case CSPTP_REQUEST_id:
            if(len != min_sz) {
                log_warning("CSPTP_REQUEST TLV with wrong size");
                return Invalid_tlv_ID;
            }
            break;
        case CSPTP_RESPONSE_id: {
            if(len != min_sz) {
                log_warning("CSPTP_RESPONSE TLV with wrong size");
                return Invalid_tlv_ID;
            }
            struct CSPTP_RESPONSE_t *t = (struct CSPTP_RESPONSE_t *)h;
            net_to_cpu_ts(&t->reqIngressTimestamp);
            t->reqCorrectionField = net_to_cpu64(t->reqCorrectionField);
            break;
        }
        case CSPTP_STATUS_id: {
            prot pt;
            size_t alen, nlen;
            struct CSPTP_STATUS_t *t = (struct CSPTP_STATUS_t *)h;
            pt = net_to_cpu16(t->parentAddress.networkProtocol);
            alen = net_to_cpu16(t->parentAddress.addressLength);
            nlen = _adderSize(pt, false);
            if(nlen == 0)
                return Invalid_tlv_ID;
            if(nlen != alen || len != min_sz + alen) {
                log_warning("CSPTP_STATUS TLV with wrong size");
                return Invalid_tlv_ID;
            }
            t->parentAddress.networkProtocol = pt;
            t->parentAddress.addressLength = alen;
            t->grandmasterClockQuality.offsetScaledLogVariance =
                net_to_cpu16(t->grandmasterClockQuality.offsetScaledLogVariance);
            t->stepsRemoved = net_to_cpu16(t->stepsRemoved);
            t->currentUtcOffset = net_to_cpu16(t->currentUtcOffset);
            break;
        }
        case PAD_id:
            break;
        default:
            return Invalid_tlv_ID;
    }
    *tlv_len = len;
    return id;
}
static bool cpu_to_net_tlv(enum tlv_type_id id, void *p, size_t tlv_len)
{
    size_t sz = _min_tlv_size(id, false);
    struct tlv_hdr_t *h = (struct tlv_hdr_t *)p;
    if(UNLIKELY_COND(sz == 0 || h == NULL || tlv_len < sz || h->tlvType != id ||
            h->lengthField + _tlv_hdr != tlv_len))
        return false;
    switch(id) {
        case ALTERNATE_TIME_OFFSET_INDICATOR_id: {
            size_t l;
            struct ALTERNATE_TIME_OFFSET_INDICATOR_t *t =
                (struct ALTERNATE_TIME_OFFSET_INDICATOR_t *)h;
            l = t->displayName.lengthField;
            if(_isOdd(l)) {
                /* Ensure pad is zero */
                t->displayName.textField[l] = 0;
                l++;
            }
            /* Verify size match */
            if(UNLIKELY_COND(tlv_len != sz + l))
                return false;
            t->currentOffset = cpu_to_net32(t->currentOffset);
            t->jumpSeconds = cpu_to_net32(t->jumpSeconds);
            cpu_to_net48(&t->timeOfNextJump);
            break;
        }
        case CSPTP_REQUEST_id:
            if(UNLIKELY_COND(tlv_len != sz))
                return false;
            break;
        case CSPTP_RESPONSE_id: {
            if(UNLIKELY_COND(tlv_len != sz))
                return false;
            struct CSPTP_RESPONSE_t *t = (struct CSPTP_RESPONSE_t *)h;
            cpu_to_net_ts(&t->reqIngressTimestamp);
            t->reqCorrectionField = cpu_to_net64(t->reqCorrectionField);
            break;
        }
        case CSPTP_STATUS_id: {
            prot pt;
            size_t alen, nlen;
            struct CSPTP_STATUS_t *t = (struct CSPTP_STATUS_t *)h;
            alen = t->parentAddress.addressLength;
            pt = t->parentAddress.networkProtocol;
            nlen = _adderSize(pt, false);
            if(UNLIKELY_COND(nlen == 0 || nlen != alen || tlv_len != sz + alen))
                return false;
            t->parentAddress.networkProtocol = cpu_to_net16(pt);
            t->parentAddress.addressLength = cpu_to_net16(alen);
            t->grandmasterClockQuality.offsetScaledLogVariance =
                cpu_to_net16(t->grandmasterClockQuality.offsetScaledLogVariance);
            t->stepsRemoved = cpu_to_net16(t->stepsRemoved);
            t->currentUtcOffset = cpu_to_net16(t->currentUtcOffset);
            break;
        }
        case PAD_id:
            break;
        default:
            return false;
    }
    h->tlvType = cpu_to_net16(id);
    h->lengthField = cpu_to_net16(tlv_len - _tlv_hdr);
    return true;
}

static void _free(pmsg self)
{
    free(self);
}
static bool _init(pmsg self, pcparms params, pbuffer buf)
{
    size_t buf_size;
    struct msg_t *m;
    uint8_t type, controlField;
    if(UNLIKELY_COND(self == NULL))
        return false;
    if(params == NULL) {
        log_err("parameters does not exist");
        return false;
    }
    if(buf == NULL) {
        log_err("buffer does not exist");
        return false;
    }
    buf_size = buf->getSize(buf);
    if(buf_size < _msg_size) {
        log_err("buffer is too small");
        return false;
    }
    type = params->type;
    switch(type) {
        case Sync:
            controlField = 0;
            break;
        case Follow_Up:
            controlField = 2;
            break;
        default:
            log_err("Unsupport nessage type");
            return false;
    }
    m = (struct msg_t *)buf->getBuf(buf);
    memset(m, 0, _msg_size);
    m->messageType_majorSdoId = type | (majorSdoId << 4);
    m->versionPTP = (minorVersionPTP << 4) | versionPTP;
    m->domainNumber = params->domainNumber;
    m->minorSdoId = minorSdoId;
    m->flagField[0] = unicastFlag | (params->useTwoSteps ? twoStepsFlag : 0);
    m->flagField[1] = params->flagField2;
    m->correctionField = params->correctionField;
    /* Always zero
    m->messageTypeSpecific = 0;
    m->sourcePortIdentity = { 0 }; */
    m->sequenceId = params->sequenceId;
    m->controlField = controlField;
    m->logMessageInterval = 0x7f; /* for unicast */
    m->timestamp = params->timestamp;
    self->_msg = m;
    self->_buf = buf;
    self->_len = _msg_size;
    buf->setLen(buf, _msg_size);
    self->_left = buf_size - _msg_size;
    /* pointer to unused buffer memory */
    self->_end = (void *)(m + 1);
    self->_type = type;
    self->_num_tlvs = 0;
    return true;
}
static void *_nextTlv(pmsg self, size_t need)
{
    return UNLIKELY_COND(self == NULL) || _makeEven(need) > self->_left ||
        self->_num_tlvs == MAX_TLVS ? NULL : self->_end;
}
static bool _addTlv(pmsg self, enum tlv_type_id id)
{
    size_t sz, num_tlvs;
    struct tlv_hdr_t *h;
    if(UNLIKELY_COND(self == NULL))
        return false;
    if(self->_end == NULL) {
        log_err("message was not initialized");
        return false;
    }
    if(self->_num_tlvs == MAX_TLVS) {
        log_warning("message is full with TLVs");
        return false;
    }
    if(self->_left < _tlv_hdr) {
        log_err("message buffer is too small");
        return false;
    }
    sz = _min_tlv_size(id, true);
    /* Here we check the minimum size of the TLV */
    if(sz == 0)
        return false;
    if(sz > self->_left) {
        log_err("message buffer is too small");
        return false;
    }
    h = (struct tlv_hdr_t *)self->_end;
    h->tlvType = id;
    switch(id) {
        case ALTERNATE_TIME_OFFSET_INDICATOR_id: {
            uint8_t l;
            struct ALTERNATE_TIME_OFFSET_INDICATOR_t *t = (struct
                    ALTERNATE_TIME_OFFSET_INDICATOR_t *)h;
            l = _makeEven(t->displayName.lengthField);
            if(l > MAX_TZ_LEN) {
                log_err("Time zone acronyms exceed allowed maximum %d", l);
                return false;
            }
            sz += l;
            break;
        }
        case CSPTP_REQUEST_id: /* Use addCSPTPReqTlv() !*/
            return false;
        case CSPTP_STATUS_id: {
            struct CSPTP_STATUS_t *t = (struct CSPTP_STATUS_t *)h;
            size_t add = _adderSize(t->parentAddress.networkProtocol, true);
            if(add == 0)
                return false;
            sz += add;
            break;
        }
        case CSPTP_RESPONSE_id:
            break;
        case PAD_id: /* We do not add pad directly! */
            return false;
        default: /* Unknown TLV */
            return false;
    }
    /* Here we check the actual size of the TLV */
    if(sz > self->_left) {
        log_err("message buffer is too small");
        return false;
    }
    /* Update the TLV with its size */
    h->lengthField = sz - _tlv_hdr;
    num_tlvs = self->_num_tlvs;
    /* Store the TLV */
    self->_tlvs[num_tlvs].tlv = self->_end;
    self->_tlvs[num_tlvs].len = sz;
    self->_tlvs[num_tlvs].id = id;
    /* Move the pointer to unused buffer memory */
    self->_end = (void *)((uint8_t *)h + sz);
    self->_left -= sz;
    self->_num_tlvs++;
    self->_len += sz;
    self->_buf->setLen(self->_buf, self->_len);
    return true;
}
bool _addCSPTPReqTlv(pmsg self, uint8_t flags)
{
    size_t num_tlvs;
    struct CSPTP_REQUEST_t *t;
    if(UNLIKELY_COND(self == NULL))
        return false;
    if(self->_end == NULL) {
        log_err("message was not initialized");
        return false;
    }
    if(self->_num_tlvs == MAX_TLVS) {
        log_warning("message is full with TLVs");
        return false;
    }
    if(self->_left < _csptp_req) {
        log_err("message buffer is too small");
        return false;
    }
    t = (struct CSPTP_REQUEST_t *)self->_end;
    t->hdr.tlvType = CSPTP_REQUEST_id;
    t->hdr.lengthField = _csptp_req - _tlv_hdr;
    t->tlvRequestFlags[0] = flags &
        (Flags0_Req_StatusTlv | Flags0_Req_AlternateTimeTlv);
    t->tlvRequestFlags[1] = 0;
    t->tlvRequestFlags[2] = 0;
    t->tlvRequestFlags[3] = 0;
    num_tlvs = self->_num_tlvs;
    /* Store the TLV */
    self->_tlvs[num_tlvs].tlv = self->_end;
    self->_tlvs[num_tlvs].len = _csptp_req;
    self->_tlvs[num_tlvs].id = CSPTP_REQUEST_id;
    /* Move the pointer to unused buffer memory */
    self->_end = (void *)((uint8_t *)t + _csptp_req);
    self->_left -= _csptp_req;
    self->_num_tlvs++;
    self->_len += _csptp_req;
    self->_buf->setLen(self->_buf, self->_len);
    return true;
}
static bool _buildDone(pmsg self, size_t size)
{
    pbuffer buf;
    size_t i, pad_sz;
    if(UNLIKELY_COND(self == NULL))
        return false;
    if(self->_end == NULL || UNLIKELY_COND(self->_msg == NULL ||
            self->_buf == NULL)) {
        log_err("message was not initialized");
        return false;
    }
    buf = self->_buf;
    if(size == 0)
        size = self->_len;
    else {
        if(_isOdd(size)) {
            log_err("size is odd");
            return false;
        }
        if(size < self->_len) {
            log_err("message will not shrink");
            return false;
        }
        if(size > buf->getSize(buf)) {
            log_err("message buffer is too small");
            return false;
        }
    }
    pad_sz = size - self->_len;
    if(pad_sz > 0 && (pad_sz < _tlv_hdr || self->_left < pad_sz))
        return false;
    for(i = 0; i < self->_num_tlvs; i++) {
        if(!cpu_to_net_tlv(self->_tlvs[i].id, self->_tlvs[i].tlv, self->_tlvs[i].len))
            return false;
    }
    buf->setLen(buf, size);
    cpu_to_net_msg(self->_msg, size);
    /* Add the PAD TLV */
    if(pad_sz >= _tlv_hdr) {
        struct tlv_hdr_t *h = (struct tlv_hdr_t *)self->_end;
        /* We add the PAD TLV any way
         * If we have place, we mark it */
        if(self->_num_tlvs < MAX_TLVS) {
            size_t num_tlvs = self->_num_tlvs;
            self->_tlvs[num_tlvs].tlv = self->_end;
            self->_tlvs[num_tlvs].len = pad_sz;
            self->_tlvs[num_tlvs].id = PAD_id;
            self->_num_tlvs++;
        }
        self->_end = (void *)((uint8_t *)h + pad_sz);
        self->_len += pad_sz;
        self->_left -= pad_sz;
        /* Reduce header size for TLV length and zero TLV data */
        pad_sz -= _tlv_hdr;
        h->tlvType = cpu_to_net16(PAD_id);
        h->lengthField = cpu_to_net16(pad_sz);
        if(pad_sz > 0)
            memset(h + 1, 0, pad_sz);
    }
    return true;
}
static bool _parse(pmsg self, pparms params, pbuffer buf)
{
    struct msg_t *m;
    uint8_t type, controlField, *tlv_prt;
    size_t len, size, msg_len, buf_size, num_tlvs;
    if(UNLIKELY_COND(self == NULL))
        return false;
    if(params == NULL) {
        log_err("parameters does not exist");
        return false;
    }
    if(buf == NULL) {
        log_err("buffer does not exist");
        return false;
    }
    /* message data length */
    len = buf->getLen(buf);
    /* buffer size */
    buf_size = buf->getSize(buf);
    /* Ensure buffer data do not pass the buffer size, prevent bugy code */
    if(UNLIKELY_COND(len > buf_size))
        return false;
    /* Ensure we have the minimum PTP message */
    if(len < _msg_size) {
        log_notice("nessage is too short");
        return false;
    }
    /* Pointer to PTP message data */
    m = (struct msg_t *)buf->getBuf(buf);
    /* Convert Network to host order of the PTP message without the TLVs.
     * and return total PTP message length includes the TLVs */
    msg_len = net_to_cpu_msg(m);
    /* Ensure the message do not exceed the recieve data length */
    if(msg_len > len) {
        log_warning("received message is smaller than PTP message length");
        return false;
    }
    /* left TLVs size */
    size = msg_len - _msg_size;
    /* PTP Message type */
    type = m->messageType_majorSdoId & 0xf;
    /* the controlField value depends on PTP message type */
    switch(type) {
        case Sync:
            controlField = 0;
            break;
        case Follow_Up:
            controlField = 2;
            break;
        default:
            log_notice("Unsupport nessage type");
            return false;
    }
    /* Verify fileds with predefined values */
    if(m->controlField != controlField) {
        log_warning("Wrong controlField value");
        return false;
    }
    if(m->logMessageInterval != 0x7f) {
        log_warning("Wrong logMessageInterval value");
        return false;
    }
    if(m->versionPTP != ((minorVersionPTP << 4) | versionPTP)) {
        log_warning("Wrong versionPTP value");
        return false;
    }
    if((m->messageType_majorSdoId >> 4) != majorSdoId) {
        log_warning("Wrong messageType_majorSdoId value");
        return false;
    }
    if(memcmp(m->sourcePortIdentity.clockIdentity, zeroClockIdentity, 8) != 0 ||
        m->sourcePortIdentity.portNumber != 0) {
        log_warning("Wrong sourcePortIdentity value");
        return false;
    }
    if(m->minorSdoId != minorSdoId) {
        log_warning("Wrong minorSdoId value");
        return false;
    }
    if((m->flagField[0] & ~twoStepsFlag) != unicastFlag) {
        log_warning("Wrong flagField[0] value");
        return false;
    }
    if((m->flagField[1] & 0xc0) != 0) {
        log_warning("Wrong flagField[1] value");
        return false;
    }
    tlv_prt = (uint8_t *)(m + 1); /* Pointer to TLV */
    num_tlvs = 0; /* Number of TLVs */
    while(size > _tlv_hdr && num_tlvs < MAX_TLVS) {
        size_t tlv_len;
        enum tlv_type_id id = net_to_cpu_tlv(tlv_prt, size, &tlv_len);
        if(id == Invalid_tlv_ID) {
            log_debug("TLV %d failed", num_tlvs);
            break;
        }
        /* Store the TLV */
        self->_tlvs[num_tlvs].tlv = tlv_prt;
        self->_tlvs[num_tlvs].len = tlv_len;
        self->_tlvs[num_tlvs].id = id;
        tlv_prt += tlv_len;
        size -= tlv_len;
        num_tlvs++;
    };
    /* PTP Message type */
    self->_type = type;
    /* Pointer to PTP message data */
    self->_msg = m;
    self->_buf = buf;
    /* total PTP message length includes the TLVs */
    self->_len = msg_len;
    /* left size avaialable for more TLVs */
    self->_left = size + buf_size - msg_len;
    /* Pointer to unused buffer memory */
    self->_end = tlv_prt;
    /* Number of TLVs */
    self->_num_tlvs = num_tlvs;
    /* Store PTP message values in ptp_params_t structire */
    params->type = type;
    params->useTwoSteps = (m->flagField[0] & twoStepsFlag) == twoStepsFlag;
    params->flagField2 = m->flagField[1];
    params->correctionField = m->correctionField;
    params->domainNumber = m->domainNumber;
    params->sequenceId = m->sequenceId;
    params->timestamp = m->timestamp;
    return true;
}
static bool _copy(pmsg self, pbuffer buf)
{
    size_t len;
    struct msg_t *m;
    if(UNLIKELY_COND(self == NULL))
        return false;
    if(buf == NULL) {
        log_err("buffer does not exist");
        return false;
    }
    m = self->_msg;
    if(m == NULL) {
        log_warning("message miss any buffer");
        return false;
    }
    len = self->_len;
    if(UNLIKELY_COND(len < _msg_size))
        return false;
    if(buf->getSize(buf) < len) {
        log_err("buffer is too small");
        return false;
    }
    if(UNLIKELY_COND(!buf->setLen(buf, len)))
        return false;
    memcpy(buf->getBuf(buf), m, len);
    return true;
}
static void _detach(pmsg self)
{
    if(LIKELY_COND(self != NULL))
        detach(self);
}
static enum messageType_e _getMsgType(pcmsg self)
{
    return UNLIKELY_COND(self == NULL) ? -1 : self->_type;
}
static size_t _getMsgLen(pcmsg self)
{
    return UNLIKELY_COND(self == NULL) ? 0 : self->_len;
}
static size_t _getPTPMsgSize()
{
    return _msg_size;
}
static size_t _getTlvSize(enum tlv_type_id id)
{
    return _min_tlv_size(id, false) +
        (id == ALTERNATE_TIME_OFFSET_INDICATOR_id ? MAX_TZ_LEN : 0);
}
static size_t _getCSPTPStatusTlvSize(prot p)
{
    size_t add = _adderSize(p, false);
    return add == 0 ? 0 : add + _min_tlv_size(CSPTP_STATUS_id, false);
}
static size_t _getTlvs(pcmsg self)
{
    return UNLIKELY_COND(self == NULL) ? 0 : self->_num_tlvs;
}
static size_t _getTlvLen(pcmsg self, size_t index)
{
    return UNLIKELY_COND(self == NULL) ||
        index >= self->_num_tlvs ? 0 : self->_tlvs[index].len;
}
static enum tlv_type_id _getTlvID(pcmsg self, size_t index)
{
    return UNLIKELY_COND(self == NULL) ||
        index >= self->_num_tlvs ? 0 : self->_tlvs[index].id;
}
static void *_getTlv(pcmsg self, size_t index)
{
    return UNLIKELY_COND(self == NULL) ||
        index >= self->_num_tlvs ? NULL : self->_tlvs[index].tlv;
}

pmsg msg_alloc()
{
    pmsg ret = (pmsg)malloc(sizeof(struct ptp_msg_t));
    if(ret != NULL) {
        detach(ret);
#define asg(a) ret->a = _##a
        asg(free);
        asg(init);
        asg(nextTlv);
        asg(addTlv);
        asg(addCSPTPReqTlv);
        asg(buildDone);
        asg(parse);
        asg(copy);
        asg(detach);
        asg(getMsgType);
        asg(getMsgLen);
        asg(getPTPMsgSize);
        asg(getTlvSize);
        asg(getCSPTPStatusTlvSize);
        asg(getTlvs);
        asg(getTlvLen);
        asg(getTlvID);
        asg(getTlv);
    } else
        log_err("memory allocation failed");
    return ret;
}
