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
    if(LIKELY_COND(self != NULL)) {
        int e, res;
        if(!atomic_load_explicit(&self->_run, memory_order_relaxed))
            return true;
        e = thrd_join(self->_thread, &res);
        if(e == thrd_success)
            return true;
        log_err("thrd_join %s", thread_error(e));
    }
    return false;
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
static int _start(void *a)
{
    bool ret = false;
    pthread self = (pthread)a;
    if(LIKELY_COND(self != NULL && self->_func != NULL)) {
        ret = self->_func(self->_cookie);
        atomic_store_explicit(&self->_run, false, memory_order_relaxed);
        atomic_store_explicit(&self->_ret, ret, memory_order_relaxed);
    }
    return ret ? EXIT_SUCCESS : EXIT_FAILURE;
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
        e = thrd_create(&ret->_thread, _start, ret);
        if(e != thrd_success) {
            free(ret);
            log_err("thrd_create %s", thread_error(e));
            return NULL;
        }
    }
    return ret;
}
