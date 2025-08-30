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
        #ifdef HAVE_THREADS_H
        mtx_destroy(&self->_mutex);
        #endif
        #ifdef __CSPTP_PTHREADS
        pthread_mutex_destroy(&self->_mutex);
        #endif
        free(self);
    }
}
static bool _unlock(pmutex self)
{
    #ifdef HAVE_THREADS_H
    int e;
    #endif
    if(UNLIKELY_COND(self == NULL))
        return false;
    #ifdef HAVE_THREADS_H
    e = mtx_unlock(&self->_mutex);
    if(e != thrd_success) {
        log_err("mtx_unlock %s", thread_error(e));
        return false;
    }
    #endif /* HAVE_THREADS_H */
    #ifdef __CSPTP_PTHREADS
    if(pthread_mutex_unlock(&self->_mutex) != 0) {
        logp_err("pthread_mutex_unlock");
        return false;
    }
    #endif /* __CSPTP_PTHREADS */
    return true;
}
static bool _lock(pmutex self)
{
    #ifdef HAVE_THREADS_H
    int e;
    #endif
    if(UNLIKELY_COND(self == NULL))
        return false;
    #ifdef HAVE_THREADS_H
    e = mtx_lock(&self->_mutex);
    if(e != thrd_success) {
        log_err("mtx_unlock %s", thread_error(e));
        return false;
    }
    #endif /* HAVE_THREADS_H */
    #ifdef __CSPTP_PTHREADS
    if(pthread_mutex_lock(&self->_mutex) != 0) {
        logp_err("pthread_mutex_lock");
        return false;
    }
    #endif /* __CSPTP_PTHREADS */
    return true;
}
pmutex mutex_alloc()
{
    pmutex ret = malloc(sizeof(struct mutex_t));
    if(ret != NULL) {
        #ifdef HAVE_THREADS_H
        int e = mtx_init(&ret->_mutex, mtx_plain);
        if(e != thrd_success) {
            log_err("mtx_init %s", thread_error(e));
            free(ret);
            return NULL;
        }
        #endif /* HAVE_THREADS_H */
        #ifdef __CSPTP_PTHREADS
        pthread_mutex_init(&ret->_mutex, NULL);
        #endif
#define asg(a) ret->a = _##a
        asg(free);
        asg(unlock);
        asg(lock);
    }
    return ret;
}
