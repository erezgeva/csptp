/* SPDX-License-Identifier: GPL-3.0-or-later
   SPDX-FileCopyrightText: Copyright © 2025 Erez Geva <ErezGeva2@gmail.com> */

/** @file
 * @brief command line parsing
 *
 * @author Erez Geva <ErezGeva2@@gmail.com>
 * @copyright © 2025 Erez Geva
 *
 */

#include "src/cmdl.h"
#include "src/cfg.h"

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

static const struct opt_rec_t base_options[] = {
    KEY_BOOL("skip_syslog", 'y', "skip logging to syslog", false),
    KEY_BOOL("term_echo", 'e', "Log to terminal", false),
    KEY_STR("ifName", 'i', "<interface> to use", "eth0", 0),
    KEY_ENUM("log_level", 'l', "<level> for logging", LOG_WARNING, log2str, 12, LOG_EMERG, LOG_DEBUG),
    KEY_STR("", 'f', "<file> read a configuration file", "", 0),
    KEY_LAST
};

static const char base_usage[] =
    "%s:\n"
    "    -h  this help\n"
    "    -v  show version\n";

static bool key_f(const void *s, const char *key, const char *val, void *ck)
{
    popt opt = (popt)ck;
    switch(opt->keyType(opt, key)) {
        case CFG_INV: /* key does not exist */
            break;
        case CFG_STR:
            /**
             * Strip quotation marks from string
             * the configuration do not know if key uses string values
             * Only the option object knows!
             */
            val = cfg_rmStrQuote(val);
        default:
            /**
             * Configuration do NOT replace command line options
             * OPT_STR_DUP - Duplicate string to a new allocation
             */
            return opt->parseKey(opt, key, val, OPT_STR_DUP);
    }
    return false;
}

static inline bool _setLog(const char *name, popt opt)
{
    optRecVal v;
    struct log_options_t lopt;
    lopt.useSysLog = GET_OPT_TRUE('y');
    lopt.useEcho = GET_OPT_FALSE('e');
    lopt.log_level = GET_OPT_INT('l', LOG_WARNING);
    return setLog(name, &lopt);
}

enum cmd_ret cmd_base(int argc, char *argv[], popt *poptions, pcrec rcs)
{
    int o;
    popt opt;
    enum cmd_ret ret;
    const char *name, *ops_str;
    if(argc < 1 || argv == NULL || poptions == NULL || rcs == NULL)
        return CMD_ERR;
    opt = opt_alloc();
    if(opt == NULL) {
        CMD_OERR("Fail allocating options object\n");
        return CMD_ERR;
    }
    ret = CMD_ERR;
    if(!opt->addRecords(opt, base_options)) {
        CMD_OERR("Fail adding base options\n");
        goto end;
    }
    if(!opt->addRecords(opt, rcs)) {
        CMD_OERR("Fail adding additional options\n");
        goto end;
    }
    #if 0 /* Check for duplicate keys! */
    if(!opt_duplicates(opt, ":hv?", NULL))
        goto end;
    #endif
    ops_str = opt->getopt(opt, ":hv?");
    if(ops_str == NULL) {
        CMD_OERR("Fail fetching options string\n");
        goto end;
    }
    name = _basename(argv[0]);
    opterr = 0; /* Reset previous error */
    optind = 1; /* ensure we start from first argument! */
    while((o = getopt(argc, argv, ops_str)) != -1) {
        switch(o) {
            case ':':
                CMD_OERR("option '%s' need value\n\n", argv[optind - 1]);
                CMD_OERR(base_usage, name);
                opt->printHelp(opt, stderr, 4);
                goto end;
            case '?':
                if(argv[optind - 1][1] != '?') {
                    CMD_OERR("Unkown option '%s'\n\n", argv[optind - 1]);
                    CMD_OERR(base_usage, name);
                    opt->printHelp(opt, stderr, 4);
                    goto end;
                }
            case 'h':
                CMD_OUT(base_usage, name);
                opt->printHelp(opt, stdout, 4);
                ret = CMD_MSG;
                goto end;
            case 'v':
                CMD_OUT("Version: %s\n", VERSION);
                ret = CMD_MSG;
                goto end;
            default:
                if(!opt->isSkey(opt, o)) {
                    CMD_OERR("Unkown option '%s'\n", argv[optind - 1]);
                    goto end;
                }
                /**
                 * Boolean options do not use command line arguments
                 * OPT_BOOL_DEF - Use inverse of default value for boolean value
                 */
                if(!opt->parseSkey(opt, o, optarg, OPT_BOOL_DEF)) {
                    CMD_OERR("Wrong %s '%s'\n", opt->getKey(opt, o), optarg);
                    goto end;
                }
                break;
        };
    }
    /* Set logging based on command line options */
    if(_setLog(name, opt)) {
        optRecVal v;
        char *cname = GET_OPT_STR('f');
        if(cname != NULL && *cname != 0) {
            pcfg c = cfg_alloc(NULL, key_f, opt);
            if(c->parseFile(c, cname)) {
                /* Set logging based on command line options and configuration file */
                if(_setLog(name, opt))
                    ret = CMD_OK;
            }
            c->free(c);
        } else
            ret = CMD_OK;
    }
end:
    if(ret == CMD_OK)
        *poptions = opt;
    else
        opt->free(opt);
    return ret;
}
