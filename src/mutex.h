/* SPDX-License-Identifier: GPL-3.0-or-later
   SPDX-FileCopyrightText: Copyright © 2025 Erez Geva <ErezGeva2@gmail.com> */

/** @file
 * @brief Mutex
 *
 * @author Erez Geva <ErezGeva2@@gmail.com>
 * @copyright © 2025 Erez Geva
 *
 */

#ifndef __CSPTP_MUTEX_H_
#define __CSPTP_MUTEX_H_

#include "src/common.h"

typedef struct mutex_t *pmutex;

struct mutex_t {
    mtx_t _mutex; /**> C mutex */

    /**
     * Free this mutex object
     * @param[in, out] self mutex object
     */
    void (*free)(pmutex self);

    /**
     * Unlock
     * @param[in, out] self mutex object
     * @return true fot success
     */
    bool (*unlock)(pmutex self);

    /**
     * Lock
     * @param[in, out] self mutex object
     * @return true fot success
     */
    bool (*lock)(pmutex self);
};

/**
 * Allocate a mutex object
 * @return pointer to a new mutex object or null
 */
pmutex mutex_alloc();

#endif /* __CSPTP_MUTEX_H_ */
