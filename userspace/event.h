/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef EVENT_H
#define EVENT_H

#include "lib/time.h"
#include "lib/types.h"
#include "cc_macro.h"
#include "sys.h"
#include "error.h"

/** \addtogroup event event
    event is a sync object. It's used, when thread(s) are waiting
    event from specific object.

    event can be in 2 states: active and inactive. When event becomes
    active, every event waiting functions returns immediatly. If event
    is inactive, event waiting put thread in waiting state.

    Because event_wait, event_wait_ms, event_wait_us can put current
    thread in waiting state, this functions can be called only from
    SYSTEM/USER context. Other functions, including event_set, event_pulse
    can be called from any context
    \{
 */


/**
    \brief creates event object.
    \retval event HANDLE on success. On failure (out of memory), error will be raised
*/
__STATIC_INLINE HANDLE event_create()
{
    HANDLE res;
    sys_call(SVC_EVENT_CREATE, (unsigned int)&res, 0, 0);
    return res;
}

/**
    \brief make event active, release all waiters, go inactive state
    \param event: event handle
    \retval none
*/
__STATIC_INLINE void event_pulse(HANDLE event)
{
    sys_call(SVC_EVENT_PULSE, (unsigned int)event, 0, 0);
}

/**
    \brief make event active, release all waiters, go inactive state
    \details This version must be called for IRQ context
    \param event: event handle
    \retval none
*/
__STATIC_INLINE void event_ipulse(HANDLE event)
{
    __GLOBAL->svc_irq(SVC_EVENT_PULSE, (unsigned int)event, 0, 0);
}

/**
    \brief make event active, release all waiters, stay in active state
    \param event: event handle
    \retval none
*/
__STATIC_INLINE void event_set(HANDLE event)
{
    sys_call(SVC_EVENT_SET, (unsigned int)event, 0, 0);
}

/**
    \brief make event active, release all waiters, stay in active state
    \details This version must be called for IRQ context
    \param event: event handle
    \retval none
*/
__STATIC_INLINE void event_iset(HANDLE event)
{
    __GLOBAL->svc_irq(SVC_EVENT_SET, (unsigned int)event, 0, 0);
}

/**
    \brief check is event is active
    \param event: event handle
    \retval true if active, false if not
*/
__STATIC_INLINE bool event_is_set(HANDLE event)
{
    bool res;
    sys_call(SVC_EVENT_IS_SET, (unsigned int)event, (unsigned int)&res, 0);
    return res;
}

/**
    \brief make event inactive
    \param event: event handle
    \retval none
*/
__STATIC_INLINE void event_clear(HANDLE event)
{
    sys_call(SVC_EVENT_CLEAR, (unsigned int)event, 0, 0);
}

/**
    \brief make event inactive
    \details This version must be called for IRQ context
    \param event: event handle
    \retval none
*/
__STATIC_INLINE void event_iclear(HANDLE event)
{
    __GLOBAL->svc_irq(SVC_EVENT_CLEAR, (unsigned int)event, 0, 0);
}

/**
    \brief wait for event
    \param event: event handle
    \param timeout: pointer to TIME structure
    \retval true on success, false on timeout
*/
__STATIC_INLINE bool event_wait(HANDLE event, TIME* timeout)
{
    sys_call(SVC_EVENT_WAIT, (unsigned int)event, (unsigned int)timeout, 0);
    return get_last_error() == ERROR_OK;
}

/**
    \brief wait for event
    \param event: event handle
    \param timeout_ms: timeout in milliseconds
    \retval true on success, false on timeout
*/
__STATIC_INLINE bool event_wait_ms(HANDLE event, unsigned int timeout_ms)
{
    TIME timeout;
    ms_to_time(timeout_ms, &timeout);
    return event_wait(event, &timeout);
}

/**
    \brief wait for event
    \param event: event handle
    \param timeout_ms: timeout in microseconds
    \retval true on success, false on timeout
*/
__STATIC_INLINE bool event_wait_us(HANDLE event, unsigned int timeout_us)
{
    TIME timeout;
    us_to_time(timeout_us, &timeout);
    return event_wait(event, &timeout);
}

/**
    \brief destroy event
    \param event: event handle
    \retval none
*/
__STATIC_INLINE void event_destroy(HANDLE event)
{
    sys_call(SVC_EVENT_DESTROY, (unsigned int)event, 0, 0);
}

/** \} */ // end of event group

#endif // EVENT_H
