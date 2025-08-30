/* SPDX-License-Identifier: GPL-3.0-or-later
   SPDX-FileCopyrightText: Copyright © 2025 Erez Geva <ErezGeva2@gmail.com> */

/** @file
 * @brief thread object
 *
 * @author Erez Geva <ErezGeva2@@gmail.com>
 * @copyright © 2025 Erez Geva
 *
 */

#include "src/thread.h"
#include "src/log.h"

static bool _join(pcthread self)
{
    int e;
    #ifdef HAVE_THREADS_H
    int res;
    #endif
    #ifdef __CSPTP_PTHREADS
    int *res;
    #endif
    if(UNLIKELY_COND(self == NULL))
        return false;
    if(!atomic_load_explicit(&self->_run, memory_order_relaxed))
        return true;
    #ifdef HAVE_THREADS_H
    e = thrd_join(self->_thread, &res);
    if(e != thrd_success) {
        log_err("thrd_join %s", thread_error(e));
        return false;
    }
    #endif /* HAVE_THREADS_H */
    #ifdef __CSPTP_PTHREADS
    e = pthread_join(self->_thread, (void **)&res);
    if(e != 0) {
        logp_err("pthread_join");
        return false;
    }
    #endif /* __CSPTP_PTHREADS */
    return true;
}
static void _free(pthread self)
{
    if(LIKELY_COND(self != NULL)) {
        _join(self); /* Ensure we are NOT running */
        free(self);
    }
}
static bool _isRun(pcthread self)
{
    return UNLIKELY_COND(self == NULL) ? false :
        atomic_load_explicit(&self->_run, memory_order_relaxed);
}
static bool _retVal(pcthread self)
{
    return UNLIKELY_COND(self == NULL) ? false :
        atomic_load_explicit(&self->_ret, memory_order_relaxed);
}
#ifdef HAVE_THREADS_H
static int _start(void *a)
#endif
#ifdef __CSPTP_PTHREADS
static void *_start(void *a)
#endif
{
    #ifdef HAVE_THREADS_H
    int r;
    #endif
    #ifdef __CSPTP_PTHREADS
    static int r;
    #endif
    bool ret = false;
    pthread self = (pthread)a;
    if(LIKELY_COND(self != NULL && self->_func != NULL)) {
        ret = self->_func(self->_cookie);
        atomic_store_explicit(&self->_run, false, memory_order_relaxed);
        atomic_store_explicit(&self->_ret, ret, memory_order_relaxed);
    }
    r = ret ? EXIT_SUCCESS : EXIT_FAILURE;
    #ifdef HAVE_THREADS_H
    return r;
    #endif
    #ifdef __CSPTP_PTHREADS
    return &r;
    #endif
}
pthread thread_create(const thread_f func, void *cookie)
{
    pthread ret = malloc(sizeof(struct thread_t));
    if(ret != NULL) {
        int e;
#define asgV(a) ret->_##a = a
        asgV(cookie);
        asgV(func);
#define asg(a) ret->a = _##a
        asg(free);
        asg(join);
        asg(isRun);
        asg(retVal);
        atomic_store_explicit(&ret->_ret, false, memory_order_relaxed);
        atomic_store_explicit(&ret->_run, true, memory_order_relaxed);
        #ifdef HAVE_THREADS_H
        e = thrd_create(&ret->_thread, _start, ret);
        if(e != thrd_success) {
            log_err("thrd_create %s", thread_error(e));
            goto err;
        }
        #endif /* HAVE_THREADS_H */
        #ifdef __CSPTP_PTHREADS
        e = pthread_create(&ret->_thread, NULL, _start, ret);
        if(e < 0) {
            logp_err("pthread_create");
            goto err;
        }
        #endif /* __CSPTP_PTHREADS */
    }
    return ret;
err:
    free(ret);
    return NULL;
}
