/* SPDX-License-Identifier: GPL-3.0-or-later
   SPDX-FileCopyrightText: Copyright © 2025 Erez Geva <ErezGeva2@gmail.com> */

/** @file
 * @brief main of service and client
 *
 * @author Erez Geva <ErezGeva2@@gmail.com>
 * @copyright © 2025 Erez Geva
 *
 */

#ifndef __CSPTP_MAIN_H_
#define __CSPTP_MAIN_H_

#include "src/sock.h"
#include "src/cmdl.h"

struct service_state_t {
    struct ifClk_t *clockInfo;
    struct ptp_params_t params;
    pipaddr address; /** Last recieve Sync address */
    psock socket; /** Service address */
    pmsg message;
    pts rxTs;
    pts t2;
    pbuffer buffer;
    //pstore storage;
};

struct client_state_t {
    prot type;
    size_t size; /* PTP messages size */
    uint8_t tlvReqFlags0; /* CSPTP_REQUEST flags */
    struct ptp_params_t params; /* params for TX */
    pipaddr address; /** Service address */
    pipaddr RxAddress;
    psock socket;
    pmsg message;
    pbuffer buffer;
    pts tmpTs;
    /** T1: Tx of ReqSync: client -> service
     * client may transmitt in ReqSync.originTimestamp
     */
    pts t1;
    /** R1: Rx of ReqSync: client -> service
     * service transmit in RespSync.CSPTP_RESPONSE.reqIngressTimestamp
     */
    pts r1;
    /** T2: Tx of RespSync: service -> client
     * one step:
     * - transmitt in RespSync.originTimestamp
     * two steps:
     * - optionally in transmitt in RespSync.originTimestamp
     * - RespSync.correctionField = 0
     * - transmit in Follow_Up.preciseOriginTimestamp
     */
    pts t2;
    /** R2: Rx of RespSync: service -> client
     * NOT transmitted!
     */
    pts r2;
};

/**
 * service main function
 * @param[in] argc main pass number of arguments passed
 * @param[in] argcv main pass array of strings
 * @return main success of failure
 */
int service_main(int argc, char *argv[]);

/**
 * service main create socket object for client
 * @param[in] address of the service
 * @return new socket or null
 */
psock service_main_create_socket(pcipaddr address);

/**
 * service main send Response Sync message
 * @param[in, out] state service state object
 * @param[in] size of client Request Sync message
 * @param[in] tlvReqFlags0 Flags form client CSPTP_REQUEST TLV
 * @return true on success
 */
bool service_main_sendRespSync(struct service_state_t *state, size_t size,
    uint8_t tlvReqFlags0);

/**
 * servicee main send FollowUp message
 * @param[in, out] state service state object
 * @param[in] size of client Request Sync message
 * @return true on success
 */
bool service_main_sendFollowUp(struct service_state_t *state, size_t size);

/**
 * service main handle received Request Sync message
 * @param[in, out] state service state object
 * @param[in] useTxTwoSteps flag to send two steps packets
 * @return true on success
 */
bool service_main_rcvReqSync(struct service_state_t *state,
    uint8_t *tlvReqFlags0);

/**
 * service main flow
 * @param[in, out] state service state object
 * @param[in] useTxTwoSteps flag to send two steps packets
 * @return true on success
 */
bool service_main_flow(struct service_state_t *state, bool useTxTwoSteps);

/**
 * service main create working objects
 * @param[in] options service options
 * @param[in, out] state service state object
 * @return true on success
 */
bool service_main_allocObjs(struct service_opt *options,
    struct service_state_t *state);

/**
 * servive main free working objects
 * @param[in, out] state service state object
 */
void service_main_clean(struct service_state_t *state);

/**
 * client main function
 * @param[in] argc main pass number of arguments passed
 * @param[in] argcv main pass array of strings
 * @return main success of failure
 */
int client_main(int argc, char *argv[]);

/**
 * client main create address object using client options
 * @param[in] options client options
 * @param[in] type protocol
 * @return new address or null
 */
pipaddr client_main_create_address(struct client_opt *options, prot *type);

/**
 * client main create socket object for client
 * @param[in] type protocol
 * @return new socket or null
 */
psock client_main_create_socket(prot type);

/**
 * client main calculate message size based on options and protocol
 * @param[in] options client options
 * @param[in] message object protocol
 * @param[in] type protocol
 * @return message size
 */
size_t client_main_get_msg_size(struct client_opt *options, pmsg message,
    prot type);

/**
 * client main smooth size
 * @param[in] size to smooth
 * @return smoothed size
 */
size_t client_main_smooth_size(size_t size);

/**
 * client main set parameters abd calculate tlvReqFlags0
 * @param[in] options client options
 * @param[in, out] parameters to set
 * @return tlvReqFlags0 value
 */
uint8_t client_main_set_tx_params(struct client_opt *options,
    pparms parameters);

/**
 * client main send Request Sync message
 * @param[in, out] state client state object
 * @param[in] sequenceId messege sequance ID
 * @return true on success
 */
bool client_main_sendReqSync(struct client_state_t *state, uint16_t sequenceId);

/**
 * client main send FollowUp message
 * @param[in, out] state client state object
 * @param[in] sequenceId messege sequance ID of previous Request Sync message
 * @return true on success
 */
bool client_main_sendFollowUp(struct client_state_t *state,
    uint16_t sequenceId);

/**
 * client main handle received respoce Sync message
 * @param[in, out] state client state object
 * @return true on success
 */
bool client_main_rcvRespSync(struct client_state_t *state);

/**
 * client main flow
 * @param[in, out] state client state object
 * @param[in] domainNumber we use for all packets
 * @param[in] useTwoSteps flag to determine if to send a FollowUp
 * @param[in] sequenceId messege to use with all messages
 * @return true on success
 */
bool client_main_flow(struct client_state_t *state, uint8_t domainNumber,
    bool useTwoSteps, uint16_t *sequenceId);

/**
 * client main create working objects
 * @param[in] options client options
 * @param[in, out] state client state object
 * @return true on success
 */
bool client_main_allocObjs(struct client_opt *options,
    struct client_state_t *state);

/**
 * client main free working objects
 * @param[in, out] state client state object
 */
void client_main_clean(struct client_state_t *state);

/* For service_main_allocObjs, client_main_allocObjs */
#define INIT(a) do{st->a = NULL;}while(false)
#define ALLOC(a, f) do{st->a = f;if(st->a == NULL)return false;}while(false)
/* For service_main_clean, client_main_clean */
#define FREE(a) do{if(LIKELY_COND(st->a != NULL))st->a->free(st->a);}while(false)

#endif /* __CSPTP_MAIN_H_ */
