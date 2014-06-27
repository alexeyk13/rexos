/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef SEM_H
#define SEM_H

/*
        SEM
  */

/** \addtogroup SEM SEM
    SEM is a sync object. It's used, for signalling sync condition

    SEM is quite simple. SEM_signal increments counter, any
    SEM_wait*, decrements. If counter reached zero, thread will be putted
    in waiting state until next SEM_signal.

    Because SEM_wait, SEM_wait_ms, SEM_wait_us can put current
    thread in waiting state, this functions can be called only from
    SYSTEM/USER context. Other functions, including SEM_signal
    can be called from any context
    \{
 */

#include "core/core.h"
#include "core/sys_calls.h"
#include "error.h"

/**
    \brief creates SEM object.
    \retval SEM HANDLE on success. On failure (out of memory), error will be raised
*/
__STATIC_INLINE HANDLE sem_create()
{
    sys_call(SVC_SEM_CREATE, (unsigned int)&res, 0, 0);
    return res;
}

/**
    \brief increments counter
    \param sem: SEM handle
    \retval none
*/
__STATIC_INLINE void sem_signal(HANDLE sem)
{
    sys_call(SVC_SEM_SIGNAL, (unsigned int)sem, 0, 0);
}

/**
    \brief wait for SEM signal
    \param sem: SEM handle
    \param timeout: pointer to TIME structure
    \retval true on success, false on timeout
*/
__STATIC_INLINE bool sem_wait(HANDLE sem, TIME* timeout)
{
    sys_call(SVC_SEM_WAIT, (unsigned int)sem, (unsigned int)timeout, 0);
    return get_last_error() == ERROR_OK;
}

/**
    \brief wait for SEM signal
    \param sem: SEM handle
    \param timeout_ms: timeout in milliseconds
    \retval true on success, false on timeout
*/
__STATIC_INLINE bool sem_wait_ms(HANDLE sem, unsigned int timeout_ms)
{
    TIME timeout;
    ms_to_time(timeout_ms, &timeout);
    return sem_wait(sem, &timeout);
}

/**
    \brief wait for SEM signal
    \param sem: SEM handle
    \param timeout_us: timeout in microseconds
    \retval true on success, false on timeout
*/
__STATIC_INLINE bool sem_wait_us(HANDLE sem, unsigned int timeout_us)
{
    TIME timeout;
    us_to_time(timeout_us, &timeout);
    return sem_wait(sem, &timeout);
}

/**
    \brief destroys SEM
    \param sem: SEM handle
    \retval none
*/
__STATIC_INLINE void sem_destroy(HANDLE sem)
{
    sys_call(SVC_SEM_DESTROY, (unsigned int)sem, 0, 0);
}

/** \} */ // end of sem group

#endif // SEM_H
