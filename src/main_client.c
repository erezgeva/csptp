/* SPDX-License-Identifier: GPL-3.0-or-later
   SPDX-FileCopyrightText: Copyright © 2025 Erez Geva <ErezGeva2@gmail.com> */

/** @file
 * @brief main of client
 *
 * @author Erez Geva <ErezGeva2@@gmail.com>
 * @copyright © 2025 Erez Geva
 *
 */

#include "src/main.h"

#include <signal.h>

/* TODO: configuration? */
#define WAIT_LOOP (50) /* Number of polls to use */
#define POLL_MS   (50) /* Timeout im milliseconds of a single poll */
/* Cycle time TODO: make it a configuration */
static const struct timespec cycleTime = { 1, 0 };

pipaddr client_main_create_address(struct client_opt *opt, prot *type)
{
    pipaddr ret;
    uint8_t bin[IPV6_ADDR_LEN];
    if(UNLIKELY_COND(opt == NULL || type == NULL))
        return NULL;
    if(opt->domainNumber < 128 || opt->domainNumber > 239) {
        log_err("domainNumber %d out of range", opt->domainNumber);
        return NULL;
    }
    if(opt->ip == NULL) {
        log_err("client miss the service IP address");
        return NULL;
    }
    *type = opt->type;
    if(!addressStringToBinary(opt->ip, type, bin))
        return NULL;
    ret = addr_alloc(*type);
    if(ret != NULL) {
        if(UNLIKELY_COND(!ret->setIP(ret, bin))) {
            ret->free(ret);
            return NULL;
        }
        log_debug("Server host %s, IP %s", opt->ip, ret->getIPStr(ret));
    }
    return ret;
}
psock client_main_create_socket(prot type)
{
    psock ret = sock_alloc();
    if(ret != NULL) {
        if(!ret->init(ret, type)) {
            ret->free(ret);
            return NULL;
        }
    }
    return ret;
}
size_t client_main_smooth_size(size_t sz)
{
    sz += 10; /* Add 10 octets, ensure we have space for PAD Tlv */
    /* Align to multiple of 16 */
    return sz + ((0x11 - ((sz + 1) & 0xf)) & 0xf);
}
size_t client_main_get_msg_size(struct client_opt *opt, pmsg msg, prot type)
{
    size_t ret;
    if(UNLIKELY_COND(opt == NULL || msg == NULL))
        return 0;
    ret = msg->getPTPMsgSize() + msg->getTlvSize(CSPTP_RESPONSE_id);
    if(opt->useCSPTPstatus)
        ret += msg->getCSPTPStatusTlvSize(type);
    if(opt->useAltTimeScale)
        ret += msg->getTlvSize(ALTERNATE_TIME_OFFSET_INDICATOR_id);
    return ret;
}
uint8_t client_main_set_tx_params(struct client_opt *opt, pparms prms)
{
    if(UNLIKELY_COND(opt == NULL || prms == NULL))
        return 0;
    memset(prms, 0, sizeof(struct ptp_params_t));
    prms->domainNumber = opt->domainNumber;
    prms->useTwoSteps = opt->useTwoSteps;
    // prms->correctionField = 0; Always remain zero
    /* Return tlvReqFlags0 value */
    return (opt->useCSPTPstatus ? Flags0_Req_StatusTlv : 0) |
        (opt->useAltTimeScale ? Flags0_Req_AlternateTimeTlv : 0);
}
bool client_main_sendReqSync(struct client_state_t *st, uint16_t sequenceId)
{
    pmsg msg;
    pparms prms;
    if(UNLIKELY_COND(st == NULL || st->message == NULL || st->socket == NULL ||
            st->address == NULL || st->buffer == NULL || st->t1 == NULL))
        return false;
    msg = st->message;
    prms = &st->params;
    prms->type = Sync;
    prms->sequenceId = sequenceId;
    getUtcClock(st->t1); // TODO oneStep fill TX in HW or twoSteps fetch later
    /* One step with HW support will overwrite the timestamp */
    return st->t1->toTimestamp(st->t1, &prms->timestamp) &&
        msg->init(msg, prms, st->buffer) &&
        msg->addCSPTPReqTlv(msg, st->tlvReqFlags0) &&
        msg->buildDone(msg, st->size) &&
        st->socket->send(st->socket, st->buffer, st->address);
}
bool client_main_sendFollowUp(struct client_state_t *st, uint16_t sequenceId)
{
    pmsg msg;
    pparms prms;
    if(UNLIKELY_COND(st == NULL || st->message == NULL || st->socket == NULL ||
            st->address == NULL || st->buffer == NULL))
        return false;
    msg = st->message;
    prms = &st->params;
    prms->type = Follow_Up;
    prms->sequenceId = sequenceId;
    memset(&prms->timestamp, 0, sizeof(struct Timestamp_t));
    return msg->init(msg, prms, st->buffer) &&
        msg->buildDone(msg, st->size) &&
        st->socket->send(st->socket, st->buffer, st->address);
}
bool client_main_rcvRespSync(struct client_state_t *st)
{
    pmsg m;
    size_t numTlvs;
    bool haveResp = false;
    if(UNLIKELY_COND(st == NULL || st->message == NULL))
        return false;
    m = st->message;
    numTlvs = m->getTlvs(m);
    for(size_t i = 0; i < numTlvs; i++) {
        switch(m->getTlvID(m, i)) {
            case CSPTP_RESPONSE_id: {
                struct CSPTP_RESPONSE_t *r = (struct CSPTP_RESPONSE_t *)m->getTlv(m, i);
                if(UNLIKELY_COND(r == NULL))
                    return false;
                st->r1->fromTimestamp(st->r1, &r->reqIngressTimestamp);
                haveResp = true;
                /*
                 * TODO
                 * r.organizationId[3];
                 * r.organizationSubType[3];
                 * r.reqCorrectionField;
                 */
                break;
            }
            case CSPTP_STATUS_id:
                // TODO
                break;
            case ALTERNATE_TIME_OFFSET_INDICATOR_id:
                // TODO
                break;
            default: /* Message check integrety, we just ignore the other */
                break;
        }
    }
    return haveResp;
}
bool client_main_flow(struct client_state_t *st, uint8_t domainNumber,
    bool useTwoSteps, uint16_t *sID)
{
    bool ret = false;
    psock sock;
    pbuffer buf;
    pmsg msg;
    pts tmpTs;
    size_t loops = WAIT_LOOP;
    size_t timeouted = 0;
    struct ptp_params_t rxParams;
    /* Wait for RespSync with bit 1 and Follow_Up wit bit 2 */
    uint8_t wait = 3;
    if(UNLIKELY_COND(st == NULL || sID == NULL || st->socket == NULL ||
            st->buffer == NULL || st->message == NULL))
        return false;
    uint16_t sequenceId = *sID;
    if(!client_main_sendReqSync(st, sequenceId) ||
        (useTwoSteps && !client_main_sendFollowUp(st, sequenceId)))
        return false;
    sock = st->socket;
    buf = st->buffer;
    msg = st->message;
    tmpTs = st->tmpTs;
    while(loops > 0 && wait > 0) {
        if(sock->poll(sock, POLL_MS)) {
            if(sock->recv(sock, buf, st->RxAddress, tmpTs) &&
                msg->parse(msg, &rxParams, buf) &&
                rxParams.sequenceId == sequenceId &&
                rxParams.domainNumber == domainNumber &&
                st->address->eq(st->address, st->RxAddress)) {
                /* TODO
                 * rxParams.correctionField
                 * rxParams.flagField2
                 */
                switch(rxParams.type) {
                    case Sync:
                        wait &= 2; /* clear bit 1 */
                        if(!rxParams.useTwoSteps) {
                            wait &= 1; /* clear bit 2 */
                            st->t2->fromTimestamp(st->t2, &rxParams.timestamp);
                        }
                        st->r2->assign(st->r2, tmpTs);
                        if(!client_main_rcvRespSync(st))
                            return false;
                        break;
                    case Follow_Up:
                        wait &= 1; /* clear bit 2 */
                        st->t2->fromTimestamp(st->t2, &rxParams.timestamp);
                        break;
                    default:
                        log_debug("Recieve unkown PTP message type %d", rxParams.type);
                        break;
                }
            } else
                loops--;
        } else {
            loops--;
            timeouted++;
        }
    }
    if(wait == 0) {
        int64_t _t1, _t2;
        _t1 = st->t1->getTs(st->t1);
        _t2 = st->t2->getTs(st->t2);
        log_info("Offset from master %zd", _t2 - _t1);
        log_debug("Summary of times:");
        log_debug("T1: %zd", _t1);
        log_debug("R1: %zd", st->r1->getTs(st->r1));
        log_debug("T2: %zd", _t2);
        log_debug("R2: %zd", st->r2->getTs(st->r2));
        ret = true;
    } else
        log_debug("Tine out waiting for responce");
    if(sequenceId == 0xffff)
        *sID = 1; /* overflow */
    else
        *sID = sequenceId + 1; /* next sequenceId */
    /* Sleep to next cycle */
    tmpTs->fromTimespec(tmpTs, &cycleTime);
    tmpTs->addMilliseconds(tmpTs, (-timeouted) * POLL_MS);
    /* Sleep will ignore negative values! */
    tmpTs->sleep(tmpTs);
    return ret;
}
bool client_main_allocObjs(struct client_opt *opt, struct client_state_t *st)
{
    size_t size;
    INIT(socket);
    INIT(message);
    INIT(buffer);
    INIT(tmpTs);
    INIT(t1);
    INIT(r1);
    INIT(t2);
    INIT(r2);
    ALLOC(address, client_main_create_address(opt, &st->type));
    ALLOC(RxAddress, addr_alloc(st->type));
    ALLOC(socket, client_main_create_socket(st->type));
    ALLOC(message, msg_alloc());
    size = client_main_get_msg_size(opt, st->message, st->type);
    if(UNLIKELY_COND(size == 0))
        return false;
    size = client_main_smooth_size(size);
    ALLOC(buffer, buffer_alloc(size));
    ALLOC(tmpTs, ts_alloc());
    ALLOC(t1, ts_alloc());
    ALLOC(r1, ts_alloc());
    ALLOC(t2, ts_alloc());
    ALLOC(r2, ts_alloc());
    st->size = size;
    st->tlvReqFlags0 = client_main_set_tx_params(opt, &st->params);
    return true;
}
void client_main_clean(struct client_state_t *st)
{
    FREE(address);
    FREE(RxAddress);
    FREE(socket);
    FREE(message);
    FREE(buffer);
    FREE(tmpTs);
    FREE(t1);
    FREE(r1);
    FREE(t2);
    FREE(r2);
    doneLog();
}
static struct client_state_t state;
static void interupt_handler(int signal)
{
    printf(" ...\n"); /* The terminal outout "^C", we complete! */
    log_debug("exit");
    client_main_clean(&state);
    exit(EXIT_SUCCESS);
}
int client_main(int argc, char *argv[])
{
    struct client_opt options;
    CMD_CALL(client);
    if(client_main_allocObjs(&options, &state)) {
        uint16_t sequenceId = 1;
        if(signal(SIGINT, interupt_handler) == SIG_ERR) /* Capture Ctrl-C */
            log_err("capture of SIGINT fail");
        else
            for(;;)
                client_main_flow(&state, options.domainNumber, options.useTwoSteps,
                    &sequenceId);
    }
    client_main_clean(&state);
    return EXIT_FAILURE;
}
