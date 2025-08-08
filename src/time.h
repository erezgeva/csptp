/* SPDX-License-Identifier: GPL-3.0-or-later
   SPDX-FileCopyrightText: Copyright © 2025 Erez Geva <ErezGeva2@gmail.com> */

/** @file
 * @brief timestamp object
 *
 * @author Erez Geva <ErezGeva2@@gmail.com>
 * @copyright © 2025 Erez Geva
 *
 */

#ifndef __CSPTP_TIME_H_
#define __CSPTP_TIME_H_

#include "src/common.h"

typedef struct ts_t *pts;
typedef const struct ts_t *pcts;
typedef struct Timestamp_t *ppts;
typedef const struct Timestamp_t *pcpts;
typedef struct timespec *ptsp;
typedef const struct timespec *pctsp;

struct Timestamp_t {
    struct UInteger48_t secondsField;
    uint32_t nanosecondsField;
} _PACKED__;

struct time_zone_t {
    int32_t offset; /* from UTC in seconds */
    char name[MAX_TZ_LEN];
};

struct ts_t {
    struct timespec _ts;

    /**
     * Free this timestamp object
     * @param[in, out] self timestamp object
     */
    void (*free)(pts self);

    /**
     * Set from timespec
     * @param[in, out] self timestamp object
     * @param[in] ts timespec object
     */
    void (*fromTimespec)(pts self, pctsp ts);

    /**
     * Convert to timespec
     * @param[in] self timestamp object
     * @param[out] ts timespec object
     * @note We do not check nanosecond range!
     */
    void (*toTimespec)(pcts self, ptsp ts);

    /**
     * Set from Timestamp_t
     * @param[in, out] self timestamp object
     * @param[in] ts pointer to Timestamp_t
     * @return true on success
     */
    bool (*fromTimestamp)(pts self, pcpts ts);

    /**
     * Convert to Timestamp_t
     * @param[in] self timestamp object
     * @param[in] ts pointer to Timestamp_t
     * @return true on success
     */
    bool (*toTimestamp)(pcts self, ppts ts);

    /**
     * Set timestamp
     * @param[in, out] self timestamp object
     * @param[in] ts timespec object
     */
    void (*setTs)(pts self, int64_t ts);

    /**
     * Get timestamp
     * @param[in] self timestamp object
     */
    int64_t (*getTs)(pcts self);

    /**
     * Copy value from another timestamp
     * @param[in] self timestamp object
     * @param[in] other timestamp object
     * @return true if timestamps are equal
     */
    void (*assign)(pts self, pcts other);

    /**
     * Timestamp are equal
     * @param[in] self timestamp object
     * @param[in] other timestamp object
     * @return true if timestamps are equal
     */
    bool (*eq)(pcts self, pcts other);

    /**
     * Timestamp is less to other timestamp
     * @param[in] self timestamp object
     * @param[in] other timestamp object
     * @return true if self timestamp is less then other
     */
    bool (*less)(pcts self, pcts other);

    /**
     * Timestamp is less or equal to other timestamp
     * @param[in] self timestamp object
     * @param[in] other timestamp object
     * @return true if self timestamp is less then other
     */
    bool (*lessEq)(pcts self, pcts other);

    /**
     * sleep for the duration of this timestamp value
     * @param[in] self timestamp object
     * @param[in] update the timestamp with remaining time
     */
    void (*sleep)(pcts self);

    /**
     * Add milliseconds
     * @param[in, out] self timestamp object
     * @param[in] milliseconds to add
     */
    void (*addMilliseconds)(pts self, int milliseconds);
};

/**
 * Allocate a new timestamp object
 * Return pointer to a new timestamp object or null
 */
pts ts_alloc();

/**
 * convert UInteger48_t to unsigned 64 bits integer
 * @param[in] num pointer to UInteger48_t
 * Return unsigned 64 bits integer value or zero
 */
uint64_t get_uint48(pcuint48 num);

/**
 * convert unsigned 64 bits integer to UInteger48_t
 * @param[in, out] pointer to UInteger48_t
 * @param[in] value to set
 * Return true if value is a valid 48 bits value
 */
bool set_uint48(puint48 num, uint64_t value);

/**
 * convert UInteger48_t from host to network order
 * @param[in, out] num pointer to UInteger48_t
 * Return true on success
 */
bool cpu_to_net48(puint48 num);

/**
 * convert UInteger48_t from network to host order
 * @param[in, out] num pointer to UInteger48_t
 * Return true on success
 */
bool net_to_cpu48(puint48 num);

/**
 * convert Timestamp_t from network to host order
 * @param[in, out] timestamp pointer
 * Return true on success
 */
bool net_to_cpu_ts(ppts timestamp);

/**
 * convert Timestamp_t from host to network order
 * @param[in, out] timestamp pointer
 * Return true on success
 */
bool cpu_to_net_ts(ppts timestamp);

/**
 * Get local time zones, both local and daylight if exist
 * Return pointer to time zones
 */
struct time_zone_t *getLocalTZ();

/**
 * Get system monotonic clock
 * @param[in, out] self timestamp object
 * TODO use PHC
 * @note we use it instead of the PHC
 */
static inline void getUtcClock(pts ts)
{
    clock_gettime(CLOCK_REALTIME, &ts->_ts);
}

#endif /* __CSPTP_TIME_H_ */
