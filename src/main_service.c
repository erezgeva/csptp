/* SPDX-License-Identifier: GPL-3.0-or-later
   SPDX-FileCopyrightText: Copyright © 2025 Erez Geva <ErezGeva2@gmail.com> */

/** @file
 * @brief main of service
 *
 * @author Erez Geva <ErezGeva2@@gmail.com>
 * @copyright © 2025 Erez Geva
 *
 */

#include "src/main.h"

#include <signal.h>

#define POLL_MS (3000) /* Timeout im milliseconds of a single poll */

/* TODO: fill dummy information */
void dummyClockInfo(struct service_opt *opt, struct ifClk_t *clk)
{
    clk->ifName = opt->ifName;
    memcpy(clk->clockIdentity, "\x1\x2\x3\xfe\xff\x4\x5\x6", 8);
    memcpy(clk->organizationId, "\x1\x2\x3", 3);
    memcpy(clk->organizationSubType, "\x4\x5\x6", 3);
    clk->networkProtocol = UDP_IPv4;
    clk->addressField = (uint8_t *)"\x1\x4\x7"; /* 1.4.7.0 */
    /* Clock parameters, comes from configuration */
    clk->priority1 = 127;
    clk->priority2 = 127;
    clk->clockQuality.clockClass = 12;
    clk->clockQuality.clockAccuracy = 73;
    clk->clockQuality.offsetScaledLogVariance = 7;
    clk->currentUtcOffset = 37;
    clk->keyField = 1;
    clk->currentOffset = 10800; // UTC + 3
    clk->jumpSeconds = 1;
    clk->timeOfNextJump = 175863;
    clk->TZName = "CEST";
}
psock service_main_create_socket(pcipaddr addr)
{
    psock ret = sock_alloc();
    if(ret != NULL) {
        if(!ret->initSrv(ret, addr)) {
            ret->free(ret);
            return NULL;
        }
    }
    return ret;
}
static inline bool addRespTlv(pmsg msg, struct ifClk_t *clk, pts rxTs)
{
    struct CSPTP_RESPONSE_t *rp = (struct CSPTP_RESPONSE_t *)
        msg->nextTlv(msg, msg->getTlvSize(CSPTP_RESPONSE_id));
    if(rp == NULL)
        return false;
    memcpy(rp->organizationId, clk->organizationId, 3);
    memcpy(rp->organizationSubType, clk->organizationSubType, 3);
    rp->reqCorrectionField = 0; // TODO
    return LIKELY_COND(rxTs->toTimestamp(rxTs, &rp->reqIngressTimestamp));
}
static inline bool addStatusTlv(pmsg msg, struct ifClk_t *clk)
{
    struct CSPTP_STATUS_t *st = (struct CSPTP_STATUS_t *)
        msg->nextTlv(msg, msg->getCSPTPStatusTlvSize(clk->networkProtocol));
    if(st == NULL)
        return false;
    memcpy(st->organizationId, clk->organizationId, 3);
    memcpy(st->organizationSubType, clk->organizationSubType, 3);
    st->grandmasterPriority1 = clk->priority1;
    memcpy(&st->grandmasterClockQuality, &clk->clockQuality,
        sizeof(struct ClockQuality_t));
    st->grandmasterPriority2 = clk->priority2;
    st->stepsRemoved = 0;
    st->currentUtcOffset = clk->currentUtcOffset;
    memcpy(st->grandmasterIdentity.clockIdentity, clk->clockIdentity, 8);
    st->parentAddress.networkProtocol = clk->networkProtocol;
    switch(clk->networkProtocol) {
        case UDP_IPv4:
            st->parentAddress.addressLength = IPV4_ADDR_LEN;
            memcpy(st->parentAddress.addressField, clk->addressField, IPV4_ADDR_LEN);
            break;
        case UDP_IPv6:
            st->parentAddress.addressLength = IPV6_ADDR_LEN;
            memcpy(st->parentAddress.addressField, clk->addressField, IPV6_ADDR_LEN);
            break;
        default:
            log_err("Wrong configuration: unkown protocol %d", clk->networkProtocol);
            return false;
    }
    if(!msg->addTlv(msg, CSPTP_STATUS_id))
        return false;
    return true;
}
static inline bool addAltTimeTlv(pmsg msg, struct ifClk_t *clk)
{
    struct ALTERNATE_TIME_OFFSET_INDICATOR_t *a =
        (struct ALTERNATE_TIME_OFFSET_INDICATOR_t *)
        msg->nextTlv(msg, msg->getTlvSize(ALTERNATE_TIME_OFFSET_INDICATOR_id));
    if(a == NULL)
        return false;
    a->keyField = clk->keyField;
    a->currentOffset = clk->currentOffset;
    a->jumpSeconds = clk->jumpSeconds;
    if(!set_uint48(&a->timeOfNextJump, clk->timeOfNextJump))
        return false;
    if(clk->TZName == NULL || *clk->TZName == 0)
        a->displayName.lengthField = 0;
    else {
        size_t l = strnlen(clk->TZName, MAX_TZ_LEN + 1);
        if(l > MAX_TZ_LEN) {
            log_warning("Time Zone string '%s' exceed length of " stringifyVal(MAX_TZ_LEN),
                clk->TZName);
            return false;
        }
        a->displayName.lengthField = l;
        memcpy(a->displayName.textField, clk->TZName, l);
    }
    if(!msg->addTlv(msg, ALTERNATE_TIME_OFFSET_INDICATOR_id))
        return false;
    return true;
}
static inline bool sendRespSync(struct service_state_t *st, size_t size,
    uint8_t tlvReqFlags0)
{
    pts t2 = st->t2;
    pmsg msg = st->message;
    pparms prms = &st->params;
    struct ifClk_t *clk = st->clockInfo;
    prms->type = Sync;
    getUtcClock(t2);// TODO oneStep fill TX in HW or twoSteps fetch later
    return LIKELY_COND(t2->toTimestamp(t2, &prms->timestamp)) &&
        msg->init(msg, prms, st->buffer) && addRespTlv(msg, clk, st->rxTs) &&
        msg->addTlv(msg, CSPTP_RESPONSE_id) &&
        ((tlvReqFlags0 & Flags0_Req_StatusTlv) == 0 || addStatusTlv(msg, clk)) &&
        ((tlvReqFlags0 & Flags0_Req_AlternateTimeTlv) == 0 ||
            addAltTimeTlv(msg, clk)) &&
        msg->buildDone(msg, size) &&
        st->socket->send(st->socket, st->buffer, st->address);
}
bool service_main_sendRespSync(struct service_state_t *st, size_t size,
    uint8_t tlvReqFlags0)
{
    return UNLIKELY_COND(st == NULL || st->message == NULL || st->socket == NULL ||
            st->address == NULL || st->buffer == NULL || st->rxTs == NULL ||
            st->t2 == NULL) ? false : sendRespSync(st, size, tlvReqFlags0);
}
static inline bool sendFollowUp(struct service_state_t *st, size_t size)
{
    pmsg msg = st->message;
    pparms prms = &st->params;
    prms->type = Follow_Up;
    return st->t2->toTimestamp(st->t2, &prms->timestamp) &&
        msg->init(msg, prms, st->buffer) &&
        msg->buildDone(msg, size) &&
        st->socket->send(st->socket, st->buffer, st->address);
}
bool service_main_sendFollowUp(struct service_state_t *st, size_t size)
{
    return UNLIKELY_COND(st == NULL || st->message == NULL || st->socket == NULL ||
            st->address == NULL || st->buffer == NULL || st->t2 == NULL ||
            st->rxTs == NULL) ? false : sendFollowUp(st, size);
}
static inline bool rcvReqSync(struct service_state_t *st, uint8_t *tlvReqFlags0)
{
    bool haveReq = false;
    pmsg m = st->message;
    size_t numTlvs = m->getTlvs(m);
    for(size_t i = 0; i < numTlvs; i++) {
        switch(m->getTlvID(m, i)) {
            case CSPTP_REQUEST_id: {
                struct CSPTP_REQUEST_t *r = (struct CSPTP_REQUEST_t *)m->getTlv(m, i);
                if(UNLIKELY_COND(r == NULL))
                    return false;
                *tlvReqFlags0 = r->tlvRequestFlags[0];
                haveReq = true;
                break;
            }
            default: /* Message check integrety, we just ignore the other */
                break;
        }
    }
    return haveReq;
}
bool service_main_rcvReqSync(struct service_state_t *st, uint8_t *tlvReqFlags0)
{
    return UNLIKELY_COND(st == NULL || st->message == NULL ||
            tlvReqFlags0 == NULL) ? false : rcvReqSync(st, tlvReqFlags0);
}
static bool inline main_flow(struct service_state_t *st, bool useTxTwoSteps)
{
    size_t size;
    uint8_t tlvReqFlags0;
    pmsg msg = st->message;
    psock sock = st->socket;
    pbuffer b = st->buffer;
    if(sock->poll(sock, POLL_MS)) {
        if(sock->recv(sock, b, st->address, st->rxTs)) {
            if(msg->parse(msg, &st->params, b)) {
                switch(st->params.type) {
                    case Sync:
                        size = b->getLen(b);
                        st->params.useTwoSteps = useTxTwoSteps;
                        return rcvReqSync(st, &tlvReqFlags0) && sendRespSync(st, size, tlvReqFlags0) &&
                            (!useTxTwoSteps || sendFollowUp(st, size));
                    case Follow_Up:
                        break;
                    default:
                        log_debug("Recieve unkown PTP message type %d", st->params.type);
                        break;
                }
            } else
                log_warning("parse");
        } else
            log_warning("recv");
    } else
        log_debug("idle");
    return false;
}
bool service_main_flow(struct service_state_t *st, bool useTxTwoSteps)
{
    return LIKELY_COND(st != NULL && st->message != NULL && st->socket != NULL &&
            st->address != NULL && st->buffer != NULL && st->rxTs != NULL &&
            st->t2 != NULL) ? main_flow(st, useTxTwoSteps) : false;
}
bool service_main_allocObjs(struct service_opt *opt, struct service_state_t *st)
{
    if(UNLIKELY_COND(opt == NULL || st == NULL))
        return false;
    st->clockInfo = &opt->clockInfo;
    INIT(address);
    INIT(socket);
    INIT(message);
    INIT(rxTs);
    INIT(t2);
    INIT(buffer);
    //INIT(storage);
    if(opt->useRxTwoSteps && !opt->useRxTwoSteps) {
        log_err("Receiving two steps with sending one step mode is not supported");
        return false;
    }
    ALLOC(address, addr_alloc(opt->type));
    ALLOC(socket, service_main_create_socket(st->address));
    ALLOC(message, msg_alloc());
    ALLOC(rxTs, ts_alloc());
    ALLOC(t2, ts_alloc());
    /* We do not kmow the maximum that the client will send
     * Sending all 3 TLVs possible require 150, 256 should cover
     */
    ALLOC(buffer, buffer_alloc(256));
    #if 0
    /* We start with 1 octet hash, TODO increase to 2 octets? */
    if(opt->useRxTwoSteps)
        ALLOC(storage, store_alloc(opt->type, 8));
    #endif
    dummyClockInfo(opt, st->clockInfo);
    return true;
}
void service_main_clean(struct service_state_t *st)
{
    FREE(address);
    FREE(socket);
    FREE(message);
    FREE(rxTs);
    FREE(t2);
    FREE(buffer);
    //FREE(storage);
    doneLog();
}
static struct service_state_t state;
static void interupt_handler(int signal)
{
    printf(" ...\n"); /* The terminal outout "^C", we complete! */
    log_debug("exit");
    service_main_clean(&state);
    exit(EXIT_SUCCESS);
}
int service_main(int argc, char *argv[])
{
    struct service_opt options;
    CMD_CALL(service);
    if(service_main_allocObjs(&options, &state)) {
        if(signal(SIGINT, interupt_handler) == SIG_ERR) /* Capture Ctrl-C */
            log_err("capture of SIGINT fail");
        else
            for(;;)
                main_flow(&state, options.useTxTwoSteps);
    }
    service_main_clean(&state);
    return EXIT_FAILURE;
}
