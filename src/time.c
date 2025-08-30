/* SPDX-License-Identifier: GPL-3.0-or-later
   SPDX-FileCopyrightText: Copyright © 2025 Erez Geva <ErezGeva2@gmail.com> */

/** @file
 * @brief timestamp object
 *
 * @author Erez Geva <ErezGeva2@@gmail.com>
 * @copyright © 2025 Erez Geva
 *
 */

#include "src/time.h"
#include "src/log.h"
#include "src/swap.h"

#include <inttypes.h>

const int clock_realtime_id = CLOCK_REALTIME;

/** Maximum value for unsigned integer 48 bits */
static const uint64_t UINT48_MAX = UINT64_C(0xffffffffffff);

static void t_free(pts self)
{
    free(self);
}
static void t_fromTimespec(pts self, pctsp ts)
{
    if(ts == NULL)
        log_err("missing timespec");
    else if(LIKELY_COND(self != NULL) && ts->tv_nsec < NSEC_PER_SEC &&
        ts->tv_nsec >= 0) {
        self->_ts.tv_sec = ts->tv_sec;
        self->_ts.tv_nsec = ts->tv_nsec;
    }
}
static void t_toTimespec(pcts self, ptsp ts)
{
    if(ts == NULL)
        log_err("missing timespec");
    else if(LIKELY_COND(self != NULL)) {
        ts->tv_sec = self->_ts.tv_sec;
        ts->tv_nsec = self->_ts.tv_nsec;
    }
}
static bool t_fromTimestamp(pts self, pcpts ts)
{
    if(ts == NULL)
        log_err("missing PTP Timestamp");
    else if(ts->nanosecondsField >= NSEC_PER_SEC)
        log_err("PTP Timestamp uses nanoseconds out of range");
    else if(LIKELY_COND(self != NULL)) {
        self->_ts.tv_sec = get_uint48(&ts->secondsField);
        self->_ts.tv_nsec = ts->nanosecondsField;
        return true;
    }
    return false;
}
static bool t_toTimestamp(pcts self, ppts ts)
{
    if(UNLIKELY_COND(self == NULL))
        return false;
    if(ts == NULL)
        log_err("missing pointer to PTP Timestamp");
    else if(self->_ts.tv_sec < 0)
        log_warning("PTP Timestamp do not support negitive time");
    else if(UNLIKELY_COND(self->_ts.tv_nsec >= NSEC_PER_SEC ||
            self->_ts.tv_nsec < 0))
        log_err("nanoseconds out of range");
    else if(UNLIKELY_COND(self->_ts.tv_sec > UINT48_MAX))
        log_warning("seconds are above PTP Timestamp seconds range");
    else if(UNLIKELY_COND(!set_uint48(&ts->secondsField, self->_ts.tv_sec)))
        log_err("fail set_uint48");
    else {
        ts->nanosecondsField = self->_ts.tv_nsec;
        return true;
    }
    return false;
}
static void t_setTs(pts self, int64_t ts)
{
    if(LIKELY_COND(self != NULL)) {
        lldiv_t d = lldiv(ts, NSEC_PER_SEC);
        if(d.rem < 0) { /* Ensure tv_nsec stays positive! */
            d.rem += NSEC_PER_SEC;
            d.quot--;
        }
        self->_ts.tv_sec = d.quot;
        self->_ts.tv_nsec = d.rem;
    }
}
static int64_t t_getTs(pcts self)
{
    return UNLIKELY_COND(self == NULL) ? 0 :
        self->_ts.tv_sec * NSEC_PER_SEC + self->_ts.tv_nsec;
}
static void t_assign(pts self, pcts other)
{
    if(other == NULL)
        log_err("other timestap is missing");
    else if(LIKELY_COND(self != NULL && other->_ts.tv_nsec < NSEC_PER_SEC &&
            other->_ts.tv_nsec >= 0)) {
        self->_ts.tv_sec = other->_ts.tv_sec;
        self->_ts.tv_nsec = other->_ts.tv_nsec;
    }
}
static bool t_eq(pcts self, pcts other)
{
    if(other == NULL)
        log_err("other timestap is missing");
    else if(LIKELY_COND(self != NULL))
        return self->_ts.tv_sec == other->_ts.tv_sec &&
            self->_ts.tv_nsec == other->_ts.tv_nsec;
    return false;
}
static bool t_less(pcts self, pcts other)
{
    if(other == NULL)
        log_err("other timestap is missing");
    else if(LIKELY_COND(self != NULL))
        return (self->_ts.tv_sec < other->_ts.tv_sec) ||
            (self->_ts.tv_sec == other->_ts.tv_sec &&
                self->_ts.tv_nsec < other->_ts.tv_nsec);
    return false;
}
static bool t_lessEq(pcts self, pcts other)
{
    if(other == NULL)
        log_err("other timestap is missing");
    else if(LIKELY_COND(self != NULL))
        return (self->_ts.tv_sec < other->_ts.tv_sec) ||
            (self->_ts.tv_sec == other->_ts.tv_sec &&
                self->_ts.tv_nsec <= other->_ts.tv_nsec);
    return false;
}
static void t_sleep(pcts self)
{
    if(LIKELY_COND(self != NULL) && self->_ts.tv_sec > 0) {
        #ifdef HAVE_THREADS_H
        switch(thrd_sleep(&self->_ts, NULL)) {
            case 0:
                break;
            case -1:
                log_info("thrd_sleep got signal");
                break;
            default:
                logp_err("thrd_sleep");
                break;
        }
        #endif
        #ifdef __CSPTP_PTHREADS
        if(nanosleep(&self->_ts, NULL) != 0)
            logp_err("nanosleep");
        #endif
    }
}
static void t_addMilliseconds(pts self, int milliseconds)
{
    if(LIKELY_COND(self != NULL)) {
        lldiv_t d = lldiv(milliseconds, MSEC_PER_SEC);
        self->_ts.tv_sec += d.quot;
        self->_ts.tv_nsec += d.rem * NSEC_PER_MSEC;
        if(self->_ts.tv_nsec >= NSEC_PER_SEC) {
            self->_ts.tv_nsec -= NSEC_PER_SEC;
            self->_ts.tv_sec++;
        } else if(self->_ts.tv_nsec < 0) {
            self->_ts.tv_nsec += NSEC_PER_SEC;
            self->_ts.tv_sec--;
        }
    }
}
pts ts_alloc()
{
    pts ret = malloc(sizeof(struct ts_t));
    if(ret != NULL) {
        ret->_ts.tv_sec = 0;
        ret->_ts.tv_nsec = 0;
#define asg(a) ret->a = t_##a
        asg(free);
        asg(fromTimespec);
        asg(toTimespec);
        asg(fromTimestamp);
        asg(toTimestamp);
        asg(setTs);
        asg(getTs);
        asg(assign);
        asg(eq);
        asg(less);
        asg(lessEq);
        asg(sleep);
        asg(addMilliseconds);
    } else
        log_err("memory allocation failed");
    return ret;
}

uint64_t get_uint48(pcuint48 num)
{
    return UNLIKELY_COND(num == NULL) ? 0 :
        num->_low | ((uint64_t)num->_high << 32);
}
bool set_uint48(puint48 num, uint64_t value)
{
    if(UNLIKELY_COND(num == NULL))
        return false;
    if(value > UINT48_MAX) {
        log_err("Value is too big for unsigned 48 bits 0x%lx", value);
        return false;
    }
    num->_low = value & UINT32_MAX;
    num->_high = (value >> 32) & UINT16_MAX;
    return true;
}
bool cpu_to_net48(puint48 num)
{
    if(UNLIKELY_COND(num == NULL))
        return false;
    num->_low = cpu_to_net32(num->_low);
    num->_high = cpu_to_net16(num->_high);
    return true;
}
bool net_to_cpu48(puint48 num)
{
    if(UNLIKELY_COND(num == NULL))
        return false;
    num->_low = net_to_cpu32(num->_low);
    num->_high = net_to_cpu16(num->_high);
    return true;
}
bool net_to_cpu_ts(ppts ts)
{
    if(UNLIKELY_COND(ts == NULL))
        return false;
    net_to_cpu48(&ts->secondsField);
    ts->nanosecondsField = net_to_cpu32(ts->nanosecondsField);
    return true;
}
bool cpu_to_net_ts(ppts ts)
{
    if(UNLIKELY_COND(ts == NULL))
        return false;
    cpu_to_net48(&ts->secondsField);
    ts->nanosecondsField = cpu_to_net32(ts->nanosecondsField);
    return true;
}
struct time_zone_t *getLocalTZ()
{
    static struct time_zone_t lTz[2];
    #ifdef HAVE_TM_GMTOFF
    static bool first = true;
    if(first) {
        memset(lTz, 0, sizeof(lTz));
        int i;
        time_t p4, n = time(NULL);
        struct tm *l = localtime(&n);
        i = l->tm_isdst;
        lTz[i].offset = l->tm_gmtoff;
        strncpy(lTz[i].name, l-> tm_zone, MAX_TZ_LEN);
        p4 = n + 86400 * 120; /* 4 month ahead */
        l = localtime(&p4);
        if(i == l->tm_isdst) {
            n -= 86400 * 120; /* 4 month back */
            l = localtime(&n);
            if(i == l->tm_isdst) {
                /* We probably do not have daylight */
                first = false;
                return lTz;
            }
        }
        i = l->tm_isdst;
        lTz[i].offset = l->tm_gmtoff;
        strncpy(lTz[i].name, l-> tm_zone, MAX_TZ_LEN);
        first = false;
    }
    #endif /* HAVE_TM_GMTOFF */
    return lTz;
}
