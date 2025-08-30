/* SPDX-License-Identifier: GPL-3.0-or-later
   SPDX-FileCopyrightText: Copyright © 2025 Erez Geva <ErezGeva2@gmail.com> */

/** @file
 * @brief logger
 *
 * @author Erez Geva <ErezGeva2@@gmail.com>
 * @copyright © 2025 Erez Geva
 *
 */

#ifndef __CSPTP_LOG_H_
#define __CSPTP_LOG_H_

#include "src/common.h"

#ifdef HAVE_SYSLOG_H
#include <syslog.h>
#else
#define LOG_EMERG    (0)
#define LOG_ALERT    (1)
#define LOG_CRIT     (2)
#define LOG_ERR      (3)
#define LOG_WARNING  (4)
#define LOG_NOTICE   (5)
#define LOG_INFO     (6)
#define LOG_DEBUG    (7)
#endif /* HAVE_SYSLOG_H */

struct log_options_t {
    int log_level; /**> Logging level */
    bool useSysLog; /**> Use system log */
    bool useEcho; /**> Print messages to standard output (terminal) */
};

extern int log_level;

/** Internal basename function */
static inline const char *_basename(const char *path)
{
    /* basename */
    const char *ret = strrchr(path, '/');
    if(ret == NULL)
        ret = path;
    else
        ret++;
    return ret;
}

/** Threads header errors */
#ifdef HAVE_THREADS_H
static inline const char *thread_error(int err)
{
    switch(err) {
        case thrd_success:
            return "success";
        case thrd_nomem:
            return "no memory";
        case thrd_timedout:
            return "timed-out";
        case thrd_busy:
            return "busy";
        case thrd_error:
            return "error";
        default:
            return "unkown";
    }
}
#endif /* HAVE_THREADS_H */

/**
 * set logging setting
 * @param[in] name od the service
 * @param[in] options to set logger
 * @return true on success
 */
bool setLog(const char *name, const struct log_options_t *options);

/**
 * Stop logging
 */
void doneLog();

/**
 * internal logging function
 * @param[in] level of this message
 * @param[in] useErrorno use of errorno
 * @param[in] file name of source code, where logging message comes from
 * @param[in] line in source code
 * @param[in] function name
 * @param[in] format of message
 * @param[in] ... aditional parametes
 */
void _log_msg(int level, bool useErrorno, const char *file, int line,
    const char *function, const char *format, ...);

#define log_msg(level, useErrorno, ...)\
    do {\
        if(level <= log_level)\
            _log_msg(level, useErrorno, __FILE__, __LINE__, __func__, __VA_ARGS__);\
    } while(false)

#if 0
/* A panic condition was reported to all processes. */
#define log_emerg(...) log_msg(LOG_EMERG, false, __VA_ARGS__)
#define logp_emerg(...) log_msg(LOG_EMERG, true, __VA_ARGS__)
/* A condition that should be corrected immediately. */
#define log_alert(...) log_msg(LOG_ALERT, false, __VA_ARGS__)
#define logp_alert(...) log_msg(LOG_ALERT, true, __VA_ARGS__)
/* A critical condition. */
#define log_crit(...) log_msg(LOG_CRIT, false, __VA_ARGS__)
#define logp_crit(...) log_msg(LOG_CRIT, true, __VA_ARGS__)
#endif
/* An error message. */
#define log_err(...) log_msg(LOG_ERR, false, __VA_ARGS__)
#define logp_err(...) log_msg(LOG_ERR, true, __VA_ARGS__)
/* A warning message. */
#define log_warning(...) log_msg(LOG_WARNING, false, __VA_ARGS__)
#define logp_warning(...) log_msg(LOG_WARNING, true, __VA_ARGS__)
/* A condition requiring special handling. */
#define log_notice(...) log_msg(LOG_NOTICE, false, __VA_ARGS__)
#define logp_notice(...) log_msg(LOG_NOTICE, true, __VA_ARGS__)
/* A general information message. */
#define log_info(...) log_msg(LOG_INFO, false, __VA_ARGS__)
#define logp_info(...) log_msg(LOG_INFO, true, __VA_ARGS__)
/* A message useful for debugging programs. */
#define log_debug(...) log_msg(LOG_DEBUG, false, __VA_ARGS__)
#define logp_debug(...) log_msg(LOG_DEBUG, true, __VA_ARGS__)

#endif /* __CSPTP_LOG_H_ */
