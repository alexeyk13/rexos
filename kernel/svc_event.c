/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "svc_event.h"
#include "svc_malloc.h"
#include "../userspace/error.h"

void svc_event_lock_release(EVENT* event, PROCESS* process)
{
    CHECK_MAGIC(event, MAGIC_EVENT);
    dlist_remove((DLIST**)&event->waiters, (DLIST*)process);
}

void svc_event_create(EVENT** event)
{
    *event = kmalloc(sizeof(EVENT));
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
    PROCESS* process;
    while (event->waiters)
    {
        process = event->waiters;
        dlist_remove_head((DLIST**)&event->waiters);
        svc_process_wakeup(process);
    }
}

void svc_event_set(EVENT* event)
{
    svc_event_pulse(event);

    event->set = true;
}

void svc_event_is_set(EVENT* event, bool* is_set)
{
    CHECK_MAGIC(event, MAGIC_EVENT);

    *is_set = event->set;
}

void svc_event_clear(EVENT* event)
{
    CHECK_MAGIC(event, MAGIC_EVENT);
    event->set = false;
}

void svc_event_wait(EVENT* event, TIME* time)
{
    CHECK_MAGIC(event, MAGIC_EVENT);

    PROCESS* process = svc_process_get_current();
    if (!event->set)
    {
        svc_process_sleep(time, PROCESS_SYNC_EVENT, event);
        dlist_add_tail((DLIST**)&event->waiters, (DLIST*)process);
    }
}

void svc_event_destroy(EVENT* event)
{
    CHECK_MAGIC(event, MAGIC_EVENT);

    PROCESS* process;
    while (event->waiters)
    {
        process = event->waiters;
        dlist_remove_head((DLIST**)&event->waiters);
        svc_process_error(process, ERROR_SYNC_OBJECT_DESTROYED);
    }
    kfree(event);
}
