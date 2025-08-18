/* SPDX-License-Identifier: GPL-3.0-or-later
   SPDX-FileCopyrightText: Copyright © 2025 Erez Geva <ErezGeva2@gmail.com> */

/** @file
 * @brief command line parsing
 *
 * @author Erez Geva <ErezGeva2@@gmail.com>
 * @copyright © 2025 Erez Geva
 *
 */

#ifndef __CSPTP_CMDL_H_
#define __CSPTP_CMDL_H_

#include "src/log.h"
#include "src/msg.h"
#include "src/opt.h"

/*
 * TODO contain information on our clock
 * The information will comes from configuration and from the system
 */
struct ifClk_t {
    /* NIC information, can be retrieved from the system */
    const char *ifName;
    uint8_t clockIdentity[8];
    uint8_t organizationId[3];
    uint8_t organizationSubType[3];
    prot networkProtocol;
    uint8_t *addressField; /* IP address */
    /* Clock parameters, comes from configuration */
    uint8_t priority1;
    uint8_t priority2;
    struct ClockQuality_t clockQuality;
    /* Comes from UTC to TAI table */
    uint16_t currentUtcOffset; // shall be the offset of TAI from UTC,
    /* Local clock, can be retrieved from the system
     * Used in ALTERNATE_TIME_OFFSET_INDICATOR
     */
    uint8_t keyField; /* the alternate timescale reported (ID) */
    int32_t currentOffset; /* offset of the alternate timescale in seconds from PTP */
    int32_t jumpSeconds; /* the size of the next discontinuity in second */
    uint64_t timeOfNextJump; /* The PTP time of the next discontinuity */
    const char *TZName; /** Time zone arbiviation */
};

struct service_opt {
    bool useRxTwoSteps;
    bool useTxTwoSteps;
    struct ifClk_t clockInfo;
    prot type;
    const char *ifName;
};

struct client_opt {
    bool useTwoSteps;
    bool useCSPTPstatus;
    bool useAltTimeScale;
    uint8_t domainNumber;
    prot type;
    const char *ifName;
    const char *ip;
};

enum cmd_ret {
    CMD_ERR = -1, /**> Exit with error */
    CMD_OK = 0,   /**> Pass */
    CMD_MSG = 1,  /**> Exit after a grace message */
};

/**
 * Parse command line
 * @param[in] argc main pass number of arguments passed
 * @param[in] argcv main pass array of strings
 * @param[in, out] options object
 * @param[in] records additional records to more options
 * @return enum cmd_ret state
 */
enum cmd_ret cmd_base(int argc, char *argv[], popt *poptions, pcrec records);

/**
 * Parse command line for service
 * @param[in] argc main pass number of arguments passed
 * @param[in] argcv main pass array of strings
 * @param[in, out] options structure for service
 * @return enum cmd_ret state
 */
enum cmd_ret cmd_service(int argc, char *argv[], struct service_opt *options);

/**
 * Parse command line for client
 * @param[in] argc main pass number of arguments passed
 * @param[in] argcv main pass array of strings
 * @param[in, out] options structure for client
 * @return enum cmd_ret state
 */
enum cmd_ret cmd_client(int argc, char *argv[], struct client_opt *options);

#define CMD_CALL(func) \
    switch(cmd_##func(argc, argv, &options)) {\
    case CMD_ERR: return EXIT_FAILURE;\
    case CMD_MSG: return EXIT_SUCCESS;\
    case CMD_OK: break;\
    }

#define CMD_OERR(format, ...)\
    fprintf(stderr, format, ##__VA_ARGS__)
#define CMD_OUT(format, ...)\
    printf(format, ##__VA_ARGS__)

static inline const char *log2str(int64_t value)
{
    switch(value) {
        case caseItem(LOG_EMERG);
        case caseItem(LOG_ALERT);
        case caseItem(LOG_CRIT);
        case caseItem(LOG_ERR);
        case caseItem(LOG_WARNING);
        case caseItem(LOG_NOTICE);
        case caseItem(LOG_INFO);
        case caseItem(LOG_DEBUG);
    }
    return NULL;
}

#endif /* __CSPTP_CMDL_H_ */
