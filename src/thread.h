/* SPDX-License-Identifier: GPL-3.0-or-later
   SPDX-FileCopyrightText: Copyright © 2025 Erez Geva <ErezGeva2@gmail.com> */

/** @file
 * @brief thread object
 *
 * @author Erez Geva <ErezGeva2@@gmail.com>
 * @copyright © 2025 Erez Geva
 *
 */

#ifndef __CSPTP_THREAD_H_
#define __CSPTP_THREAD_H_

#include "src/common.h"

#include <stdatomic.h>

typedef struct thread_t *pthread;
typedef const struct thread_t *pcthread;
typedef bool (*thread_f)(void *cookie);

struct thread_t {
    thrd_t _thread; /**> C thread */
    thread_f _func;
    void *_cookie;
    atomic_bool _run;
    atomic_bool _ret;

    /**
     * Free this thread object
     * @param[in, out] self thread object
     */
    void (*free)(pthread self);

    /**
     * Wait for thread to exit
     * @param[in, out] self thread object
     * @return true once thread exit
     */
    bool (*join)(pcthread self);

    /**
     * Query if thread already exit
     * @param[in, out] self thread object
     * @return true if thread is running
     */
    bool (*isRun)(pcthread self);

    /**
     * Query thread exit result
     * @param[in, out] self thread object
     * @return thread exit result or false if thread is running
     */
    bool (*retVal)(pcthread self);
};

/**
 * Create a new thread with a new thread object
 * @param[in] function to call when thread start
 * @param[in] cookie to pass to fuction
 * @return pointer to a new thread object or null
 */
pthread thread_create(const thread_f function, void *cookie);

#endif /* __CSPTP_THREAD_H_ */
