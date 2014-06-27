/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "event_kernel.h"
#include "mem_kernel.h"
#include "../userspace/core/sys_calls.h"
#include "../userspace/error.h"

static inline void svc_event_create(EVENT** event)
{
    *event = sys_alloc(sizeof(EVENT));
    if (*event != NULL)
    {
        (*event)->set = false;
        (*event)->waiters = NULL;
        DO_MAGIC((*event), MAGIC_EVENT);
    }
    else
        error(ERROR_OUT_OF_SYSTEM_MEMORY);
}

void svc_event_pulse(EVENT* event)
{
    CHECK_MAGIC(event, MAGIC_EVENT);

    //release all waiters
    THREAD* thread;
    while (event->waiters)
    {
        thread = event->waiters;
        dlist_remove_head((DLIST**)&event->waiters);
        svc_thread_wakeup(thread);
    }
}

static inline void svc_event_set(EVENT* event)
{
    svc_event_pulse(event);
    event->set = true;
}

static inline void svc_event_is_set(EVENT* event, bool* is_set)
{
    *is_set = event->set;
}

static inline void svc_event_clear(EVENT* event)
{
    CHECK_MAGIC(event, MAGIC_EVENT);
    event->set = false;
}

static inline void svc_event_wait(EVENT* event, TIME* time)
{
    CHECK_MAGIC(event, MAGIC_EVENT);

    THREAD* thread = svc_thread_get_current();
    if (!event->set)
    {
        //first - remove from active list
        //if called from IRQ context, thread_private.c will raise error
        svc_thread_sleep(time, THREAD_SYNC_EVENT, event);
        dlist_add_tail((DLIST**)&event->waiters, (DLIST*)thread);
    }
}

void svc_event_lock_release(EVENT* event, THREAD* thread)
{
    CHECK_CONTEXT(SUPERVISOR_CONTEXT | IRQ_CONTEXT);
    CHECK_MAGIC(event, MAGIC_EVENT);
    dlist_remove((DLIST**)&event->waiters, (DLIST*)thread);
}

static inline void svc_event_destroy(EVENT* event)
{
    THREAD* thread;
    while (event->waiters)
    {
        thread = event->waiters;
        dlist_remove_head((DLIST**)&event->waiters);
        svc_thread_error(thread, ERROR_SYNC_OBJECT_DESTROYED);
    }
    sys_free(event);
}

void svc_event_handler(unsigned int num, unsigned int param1, unsigned int param2)
{
    CHECK_CONTEXT(SUPERVISOR_CONTEXT | IRQ_CONTEXT);
    CRITICAL_ENTER;
    switch (num)
    {
    case SVC_EVENT_CREATE:
        svc_event_create((EVENT**)param1);
        break;
    case SVC_EVENT_PULSE:
        svc_event_pulse((EVENT*)param1);
        break;
    case SVC_EVENT_SET:
        svc_event_set((EVENT*)param1);
        break;
    case SVC_EVENT_IS_SET:
        svc_event_is_set((EVENT*)param1, (bool*)param2);
        break;
    case SVC_EVENT_CLEAR:
        svc_event_clear((EVENT*)param1);
        break;
    case SVC_EVENT_WAIT:
        svc_event_wait((EVENT*)param1, (TIME*)param2);
        break;
    case SVC_EVENT_DESTROY:
        svc_event_destroy((EVENT*)param1);
        break;
    default:
        error(ERROR_INVALID_SVC);
    }
    CRITICAL_LEAVE;
}
