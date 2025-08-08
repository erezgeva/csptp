/* SPDX-License-Identifier: GPL-3.0-or-later
   SPDX-FileCopyrightText: Copyright © 2025 Erez Geva <ErezGeva2@gmail.com> */

/** @file
 * @brief logger
 *
 * @author Erez Geva <ErezGeva2@@gmail.com>
 * @copyright © 2025 Erez Geva
 *
 */

#include "src/log.h"
#include <stdarg.h>
#include <errno.h>

int log_level = LOG_DEBUG;
bool useSysLog = false;
bool useEcho = true;
char strMsg[1500];
char totMsg[2000];

bool setLog(const char *name, const struct log_options_t *opt)
{
    int level;
    if(UNLIKELY_COND(opt == NULL || name == NULL))
        return false;
    level = opt->log_level;
    if(level > LOG_DEBUG || level < LOG_EMERG) {
        fprintf(stderr, "Wrong logging level %d\n", level);
        return false;
    }
    log_level = level;
    useEcho = opt->useEcho;
    /* Do we change system log usage? */
    if(useSysLog != opt->useSysLog) {
        if(useSysLog)
            closelog();
        else
            openlog(name, LOG_PID, LOG_DAEMON);
        useSysLog = !useSysLog;
    }
    return true;
}
void doneLog()
{
    if(useSysLog)
        closelog();
}
void _log_msg(int level, bool useErrorno, const char *_file, int line,
    const char *func, const char *format, ...)
{
    int ret;
    va_list ap;
    const char *file;
    va_start(ap, format);
    ret = vsnprintf(strMsg, sizeof(strMsg), format, ap);
    va_end(ap);
    if(ret <= 0)
        return;
    strMsg[sizeof(strMsg) - 1] = 0;
    file = _basename(_file);
    if(useErrorno)
        ret = snprintf(totMsg, sizeof(totMsg), "[%d:%s:%d:%s] %s: %m", level, file,
                line, func, strMsg);
    else
        ret = snprintf(totMsg, sizeof(totMsg), "[%d:%s:%d:%s] %s", level, file, line,
                func, strMsg);
    if(ret <= 0)
        return;
    if(useSysLog)
        syslog(level, "%s", totMsg);
    if(useEcho)
        fprintf(level > LOG_WARNING ? stdout : stderr, "%s\n", totMsg);
}
