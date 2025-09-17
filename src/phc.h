/* SPDX-License-Identifier: GPL-3.0-or-later
   SPDX-FileCopyrightText: Copyright © 2025 Erez Geva <ErezGeva2@gmail.com> */

/** @file
 * @brief PTP Hardware clock objects
 *
 * @author Erez Geva <ErezGeva2@@gmail.com>
 * @copyright © 2025 Erez Geva
 *
 */

#ifndef __CSPTP_PHC_H_
#define __CSPTP_PHC_H_

#include "src/time.h"

typedef struct phc_t *pphc;
typedef const struct phc_t *pcphc;

struct phc_t {
    int _fd;
    int _ptpIndex;
    char *_device;
    clockid_t _clkId;

    /**
     * Free this phc object
     * @param[in, out] self phc object
     */
    void (*free)(pphc self);

    /**
     * Init using device name
     * @param[in, out] self phc object
     * @param[in] device name
     * @param[in] readonly state to open PHC
     * @return true on success
     */
    bool (*initDev)(pphc self, const char *device, bool readonly);

    /**
     * Init using PHC index
     * @param[in, out] self phc object
     * @param[in] ptpIndex PTP Hardware clock index
     * @param[in] readonly state to open PHC
     * @return true on success
     */
    bool (*initIndex)(pphc self, int ptpIndex, bool readonly);

    /**
     * Get clock time
     * @param[in] self phc object
     * @param[in, out] timestamp to store time
     * @return true on success
     */
    bool (*getTime)(pcphc self, pts timestamp);

    /**
     * Set clock time
     * @param[in, out] self phc object
     * @param[in] timestamp to set
     * @return true on success
     */
    bool (*setTime)(pphc self, pcts timestamp);

    /**
     * Adjust clock with offset
     * @param[in, out] self phc object
     * @param[in] offset timestamp
     * @return true on success
     */
    bool (*offsetClock)(pphc self, pcts offset);

    /**
     * Set clock phase offset
     * @param[in, out] self phc object
     * @param[in] offset in nanoseconeds
     * @return true on success
     */
    bool (*setPhase)(pphc self, pcts offset);

    /**
     * Get clock adjustment frequancy
     * @param[in] self phc object
     * @param[in] frequancy in ppb
     * @return true on success
     */
    bool (*getFreq)(pcphc self, double *frequancy);

    /**
     * Set clock adjustment frequancy
     * @param[in, out] self phc object
     * @param[in] frequancy in ppb
     * @return true on success
     */
    bool (*setFreq)(pphc self, double frequancy);

    /**
     * Get file descriptor
     * @param[in] self phc object
     * @return file descriptor or -1
     */
    int (*fileno)(pcphc self);

    /**
     * Get file descriptor
     * @param[in] self phc object
     * @return clock ID or -1
     */
    clockid_t (*clkId)(pcphc self);

    /**
     * Get PHC device index
     * @param[in] self phc object
     * @return PHC device index or -1
     */
    int (*ptpIndex)(pcphc self);

    /**
     * Get device name
     * @param[in] self phc object
     * @return device name or null
     */
    const char *(*device)(pcphc self);
};

/**
 * Allocate a phc object
 * @return pointer to a new phc object or null
 */
pphc phc_alloc();

#endif /* __CSPTP_PHC_H_ */
