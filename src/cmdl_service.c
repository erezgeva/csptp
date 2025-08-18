/* SPDX-License-Identifier: GPL-3.0-or-later
   SPDX-FileCopyrightText: Copyright © 2025 Erez Geva <ErezGeva2@gmail.com> */

/** @file
 * @brief command line parsing for service
 *
 * @author Erez Geva <ErezGeva2@@gmail.com>
 * @copyright © 2025 Erez Geva
 *
 */

#include "src/cmdl.h"

static const struct opt_rec_t service_options[] = {
    KEY_INT("oneStepRx", 'r', "1 receive one-step PTP messages only\n2 receive two-steps PTP messages only", 2, 1, 2),
    KEY_INT("oneStepTx", 't', "1 transmit one-step PTP messages only\n2 transmit two-steps PTP messages only", 2, 1, 2),
    KEY_INT("priority1", 0, NULL, 128, 0, 255),
    KEY_INT("priority2", 0, NULL, 128, 0, 255),
    /* ClockQuality */
    KEY_INT("clockClass", 0, NULL, 248, 0, 255),
    KEY_INT("clockAccuracy", 0, NULL, 0xfe, 0, 0xfe),
    KEY_INT("offsetScaledLogVariance", 0, NULL, 0xffff, 0, UINT16_MAX),
    KEY_BOOL("ipv6", '6', "Use IPv6 (defualt IPv4)", false),
    KEY_LAST
};

enum cmd_ret cmd_service(int argc, char *argv[], struct service_opt *o)
{
    popt opt;
    optRecVal v;
    enum cmd_ret ret;
    if(o == NULL || argc == 0 || argv == NULL)
        return CMD_ERR;
    ret = cmd_base(argc, argv, &opt, service_options);
    if(ret != CMD_OK)
        return ret;
    o->ifName = GET_OPT_STR('i');
    o->useRxTwoSteps = GET_OPT_INT('r', 2) == 2;
    o->useTxTwoSteps = GET_OPT_INT('t', 2) == 2;
    o->type = GET_OPT_FALSE('6') ? UDP_IPv6 : UDP_IPv4;
    opt->free(opt);
    return CMD_OK;
}
