/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef MUTEX_H
#define MUTEX_H

/** \addtogroup mutex mutex
    mutex is a sync object. It's used for exclusive object access.

    First thread, requested mutex_lock() will get access for resource,
    while other threads, trying to call mutex_lock() will be putted in
    "waiting" state until owner thread release resource by mutex_unlock(),
    or, timeout condition is meet.

    caller, used timeout condition must check result of mutex_lock() call.

    in RTOS, if higher priority thread is trying to acquire mutex, owned lower
    priority thread, on mutex_lock(), mutex owner priority will be temporaily
    raised to caller's priority, to avoid deadlock. This condition is called
    mutex inheritance.

    Because thread can own many mutexes, effectivy priority is calculated by
    highest priority of all mutex-waiters of current thread. This condition is
    called nested mutex priority inheritance.

    After releasing mutex, thread priority is returned to base.

    Because mutex_lock can put current thread in waiting state, mutex
    locking/unlocking can be called only from SYSTEM/USER contex
    \{
 */

#include "sys.h"
#include "lib/time.h"
#include "error.h"

/**
    \brief creates mutex object.
    \retval mutex HANDLE on success. On failure (out of memory), error will be raised
*/
__STATIC_INLINE HANDLE mutex_create()
{
    HANDLE res;
    sys_call(SVC_MUTEX_CREATE, (unsigned int)&res, 0, 0);
    return res;
}

/**
    \brief try to lock mutex.
    \details If mutex is already locked, exception is raised and current thread terminated
    \param mutex: mutex handle
    \param timeout: pointer to TIME structure
    \retval true on success, false on timeout
*/
__STATIC_INLINE bool mutex_lock(HANDLE mutex, TIME* timeout)
{
    sys_call(SVC_MUTEX_LOCK, (unsigned int)mutex, (unsigned int)timeout, 0);
    return get_last_error() == ERROR_OK;
}

/**
    \brief try to lock mutex
    \details If mutex is already locked, exception is raised and current thread terminated
    \param mutex: mutex handle
    \param ms: time to try in milliseconds. Can be INFINITE
    \retval true on success, false on timeout
*/
__STATIC_INLINE bool mutex_lock_ms(HANDLE mutex, unsigned int timeout_ms)
{
    TIME timeout;
    ms_to_time(timeout_ms, &timeout);
    return mutex_lock(mutex, &timeout);
}

/**
    \brief try to lock mutex
    \details If mutex is already locked, exception is raised and current thread terminated
    \param us: time to try in microseconds. Can be INFINITE
    \retval true on success, false on timeout
*/
__STATIC_INLINE bool mutex_lock_us(HANDLE mutex, unsigned int timeout_us)
{
    TIME timeout;
    us_to_time(timeout_us, &timeout);
    return mutex_lock(mutex, &timeout);
}

/**
    \brief unlock mutex
    \details If mutex is not locked, exception is raised and current thread terminated
    \param mutex: mutex handle
    \retval none
*/
__STATIC_INLINE void mutex_unlock(HANDLE mutex)
{
    sys_call(SVC_MUTEX_UNLOCK, (unsigned int)mutex, 0, 0);
}

/**
    \brief destroy mutex
    \param mutex: mutex handle
    \retval none
*/
__STATIC_INLINE void mutex_destroy(HANDLE mutex)
{
    sys_call(SVC_MUTEX_DESTROY, (unsigned int)mutex, 0, 0);
}

/** \} */ // end of mutex group

#endif // MUTEX_H
