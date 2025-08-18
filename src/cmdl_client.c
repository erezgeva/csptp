/* SPDX-License-Identifier: GPL-3.0-or-later
   SPDX-FileCopyrightText: Copyright © 2025 Erez Geva <ErezGeva2@gmail.com> */

/** @file
 * @brief command line parsing for client
 *
 * @author Erez Geva <ErezGeva2@@gmail.com>
 * @copyright © 2025 Erez Geva
 *
 */

#include "src/cmdl.h"

static const struct opt_rec_t client_options[] = {
    KEY_BOOL("oneStep", 't', "Use one-step PTP messages", false),
    KEY_BOOL("reqStatTLV", 's', "requests CSPTP status TLV", false),
    KEY_BOOL("reqAltTLV", 'a', "requests alternate timescale TLV", false),
    KEY_STR("serviceAddress", 'd', "<address> IP address or host name of service", "", 0),
    KEY_BOOL("ipv4", '4', "Force IPv4 service", false),
    KEY_BOOL("ipv6", '6', "Force IPv6 service", false),
    KEY_INT("domainNumber", 'n', "<domain number> domainNumber", 128, 128, 239),
    KEY_LAST
};


enum cmd_ret cmd_client(int argc, char *argv[], struct client_opt *o)
{
    popt opt;
    optRecVal v;
    bool h4, h6;
    enum cmd_ret ret;
    if(o == NULL || argc == 0 || argv == NULL)
        return CMD_ERR;
    ret = cmd_base(argc, argv, &opt, client_options);
    if(ret != CMD_OK)
        return ret;
    h4 = GET_OPT_FALSE('4');
    h6 = GET_OPT_FALSE('6');
    if(h4 && h6) {
        CMD_OERR("options '-4' and '-6' can not be used together\n");
        opt->free(opt);
        return CMD_ERR;
    }
    if(h4)
        o->type = UDP_IPv4;
    else if(h6)
        o->type = UDP_IPv6;
    else
        o->type = Invalid_PROTO;
    o->ifName = GET_OPT_STR('i');
    o->useTwoSteps = GET_OPT_TRUE('t');
    o->useCSPTPstatus = GET_OPT_FALSE('s');
    o->useAltTimeScale = GET_OPT_FALSE('a');
    o->ip = GET_OPT_STR('d');
    o->domainNumber = GET_OPT_INT('n', 128);
    opt->free(opt);
    return CMD_OK;
}
