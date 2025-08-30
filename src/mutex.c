/* SPDX-License-Identifier: GPL-3.0-or-later
   SPDX-FileCopyrightText: Copyright © 2025 Erez Geva <ErezGeva2@gmail.com> */

/** @file
 * @brief Mutex
 *
 * @author Erez Geva <ErezGeva2@@gmail.com>
 * @copyright © 2025 Erez Geva
 *
 */

#include "src/mutex.h"
#include "src/log.h"

static void _free(pmutex self)
{
    if(LIKELY_COND(self != NULL)) {
        mtx_destroy(&self->_mutex);
        free(self);
    }
}
static bool _unlock(pmutex self)
{
    if(LIKELY_COND(self != NULL)) {
        int e = mtx_unlock(&self->_mutex);
        if(e == thrd_success)
            return true;
        else
            log_err("mtx_unlock %s", thread_error(e));
    }
    return false;
}
static bool _lock(pmutex self)
{
    if(LIKELY_COND(self != NULL)) {
        int e = mtx_lock(&self->_mutex);
        if(e == thrd_success)
            return true;
        else
            log_err("mtx_unlock %s", thread_error(e));
    }
    return false;
}
pmutex mutex_alloc()
{
    pmutex ret = malloc(sizeof(struct mutex_t));
    if(ret != NULL) {
        int e = mtx_init(&ret->_mutex, mtx_plain);
        if(e != thrd_success) {
            log_err("mtx_init %s", thread_error(e));
            free(ret);
            return NULL;
        }
#define asg(a) ret->a = _##a
        asg(free);
        asg(unlock);
        asg(lock);
    }
    return ret;
}
