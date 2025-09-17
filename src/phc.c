/* SPDX-License-Identifier: GPL-3.0-or-later
   SPDX-FileCopyrightText: Copyright © 2025 Erez Geva <ErezGeva2@gmail.com> */

/** @file
 * @brief PTP Hardware clock objects
 *
 * @author Erez Geva <ErezGeva2@@gmail.com>
 * @copyright © 2025 Erez Geva
 *
 */

#define _GNU_SOURCE /* For clock_adjtime() */

#include "src/phc.h"
#include "src/log.h"

#include <limits.h>
#ifdef _POSIX_SOURCE
#include <unistd.h>   /* POSIX */
#include <fcntl.h>    /* POSIX */
#include <sys/stat.h> /* POSIX */
#endif /*_POSIX_SOURCE*/

#define CLOCK_INVALID (-1)
#define NO_SUCH_PTP (-1)

#define PTP_DEV "/dev/ptp"

// parts per billion (ppb) = 10^9
// to scale parts per billion (ppm) = 10^6 * 2^16
#define PPB_TO_SCALE_PPM  (65.536) // (2^16 / 1000)

// From kernel include/linux/posix-timers.h
//             include/linux/posix-timers_types.h
static inline clockid_t fd_to_clockid(int fd)
{
    return (~((unsigned int)fd) << 3) | 3;
}
static inline char *isCharFile(const char *file, char *b)
{
    struct stat sb;
    char *a = realpath(file, b);
    if(a == NULL)
        return NULL; /* File does not exist */
    if(stat(file, &sb) != 0) {
        logp_err("stat");
        return NULL;
    }
    return (sb.st_mode & S_IFMT) == S_IFCHR ? a : NULL;
}
static inline bool init(pphc self, const char *dev, bool rd)
{
    struct timespec ts;
    int fd = open(dev, rd ? O_RDONLY : O_RDWR);
    if(fd < 0) {
        logp_err("opening %s", dev);
        return false;
    }
    clockid_t cid = fd_to_clockid(fd);
    if(cid == CLOCK_INVALID) {
        log_err("failed to read clock id");
        goto err;
    }
    if(clock_gettime(cid, &ts) != 0) {
        logp_err("clock_gettime");
        goto err;
    }
    self->_fd = fd;
    self->_clkId = cid;
    self->_device = strdup(dev);
    return true;
err:
    close(fd);
    return false;
}
static inline bool initDone(pphc self, const char *dev)
{
    int d;
    if(sscanf(dev, PTP_DEV "%d", &d) == 1)
        self->_ptpIndex = d;
    return true;
}
static void _free(pphc self)
{
    if(LIKELY_COND(self != NULL)) {
        if(self->_fd >= 0)
            close(self->_fd);
        free(self->_device);
        free(self);
    }
}
static bool _initDev(pphc self, const char *dev, bool rd)
{
    char p[PATH_MAX], *a;
    if(UNLIKELY_COND(self == NULL) || dev == NULL)
        return false;
    if(self->_fd >= 0) {
        log_warning("Already Initialized");
        return false;
    }
    if((a = isCharFile(dev, p)) != NULL) {
        if(init(self, a, rd))
            return initDone(self, a);
    } else if(strchr(dev, '/') == NULL) {
        char p0[PATH_MAX];
        if(snprintf(p0, sizeof(p0), "/dev/%s", dev) > 0 &&
            (a = isCharFile(p0, p)) != NULL && init(self, a, rd))
            return initDone(self, a);
    }
    return false;
}
static bool _initIndex(pphc self, int idx, bool rd)
{
    char p[PATH_MAX];
    if(UNLIKELY_COND(self == NULL) || idx < 0)
        return false;
    if(self->_fd >= 0) {
        log_warning("Already Initialized");
        return false;
    }
    if(snprintf(p, sizeof(p), PTP_DEV "%d", idx) > 0 && init(self, p, rd)) {
        self->_ptpIndex = idx;
        return true;
    }
    return false;
}
static bool _getTime(pcphc self, pts ts)
{
    if(UNLIKELY_COND(self == NULL) || self->_fd < 0 || ts == NULL)
        return false;
    if(clock_gettime(self->_clkId, &ts->_ts) != 0) {
        logp_err("clock_gettime");
        return false;
    }
    return true;
}
static bool _setTime(pphc self, pcts ts)
{
    if(UNLIKELY_COND(self == NULL) || self->_fd < 0 || ts == NULL)
        return false;
    if(clock_settime(self->_clkId, &ts->_ts) != 0) {
        logp_err("clock_settime");
        return false;
    }
    return true;
}
static bool _offsetClock(pphc self, pcts ts)
{
    struct timex tmx;
    if(UNLIKELY_COND(self == NULL) || self->_fd < 0 || ts == NULL)
        return false;
    memset(&tmx, 0, sizeof(struct timex));
    tmx.modes = ADJ_SETOFFSET | ADJ_NANO;
    tmx.time.tv_sec = ts->_ts.tv_sec;
    /* using ADJ_NANO means it holds nanoseconds */
    tmx.time.tv_usec = ts->_ts.tv_nsec;
    if(clock_adjtime(self->_clkId, &tmx) != 0) {
        logp_err("ADJ_SETOFFSET");
        return false;
    }
    return true;
}
static bool _setPhase(pphc self, pcts ts)
{
    struct timex tmx;
    if(UNLIKELY_COND(self == NULL) || self->_fd < 0 || ts == NULL)
        return false;
    memset(&tmx, 0, sizeof(struct timex));
    /* ADJ_NANO: Use nanoseconds instead of microseconds */
    tmx.modes = ADJ_OFFSET | ADJ_NANO;
    tmx.offset = ts->getTs(ts);
    if(clock_adjtime(self->_clkId, &tmx) != 0) {
        logp_err("ADJ_FREQUENCY");
        return false;
    }
    return true;
}
static bool _getFreq(pcphc self, double *f)
{
    struct timex tmx;
    if(UNLIKELY_COND(self == NULL) || self->_fd < 0 || f == NULL)
        return false;
    memset(&tmx, 0, sizeof(struct timex));
    if(clock_adjtime(self->_clkId, &tmx) != 0) {
        logp_err("clock_adjtime");
        return false;
    }
    *f = (double)tmx.freq / PPB_TO_SCALE_PPM;
    return true;
}
static bool _setFreq(pphc self, double f)
{
    struct timex tmx;
    if(UNLIKELY_COND(self == NULL) || self->_fd < 0)
        return false;
    memset(&tmx, 0, sizeof(struct timex));
    tmx.modes = ADJ_FREQUENCY;
    tmx.freq = (long)(f * PPB_TO_SCALE_PPM);
    if(clock_adjtime(self->_clkId, &tmx) != 0) {
        logp_err("ADJ_FREQUENCY");
        return false;
    }
    return true;
}
static int _fileno(pcphc self)
{
    return LIKELY_COND(self != NULL) ? self->_fd : -1;
}
static clockid_t _clkId(pcphc self)
{
    return LIKELY_COND(self != NULL) ? self->_clkId : CLOCK_INVALID;
}
static int _ptpIndex(pcphc self)
{
    return LIKELY_COND(self != NULL) ? self->_ptpIndex : NO_SUCH_PTP;
}
static const char *_device(pcphc self)
{
    return LIKELY_COND(self != NULL) ? self->_device : NULL;
}
pphc phc_alloc()
{
    pphc ret = malloc(sizeof(struct phc_t));
    if(ret != NULL) {
        ret->_fd = -1;
        ret->_ptpIndex = NO_SUCH_PTP;
        ret->_clkId = CLOCK_INVALID;
        ret->_device = NULL;
#define asg(a) ret->a = _##a
        asg(free);
        asg(initDev);
        asg(initIndex);
        asg(getTime);
        asg(setTime);
        asg(offsetClock);
        asg(setPhase);
        asg(getFreq);
        asg(setFreq);
        asg(fileno);
        asg(clkId);
        asg(ptpIndex);
        asg(device);
    } else
        log_err("memory allocation failed");
    return ret;
}
