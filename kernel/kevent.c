/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "kevent.h"
#include "kmalloc.h"
#include "../userspace/error.h"

void kevent_lock_release(EVENT* event, PROCESS* process)
{
    CHECK_HANDLE(event, sizeof(EVENT));
    CHECK_MAGIC(event, MAGIC_EVENT);
    dlist_remove((DLIST**)&event->waiters, (DLIST*)process);
}

void kevent_create(EVENT** event)
{
    *event = kmalloc(sizeof(EVENT));
    if (*event != NULL)
    {
        (*event)->set = false;
        (*event)->waiters = NULL;
        DO_MAGIC((*event), MAGIC_EVENT);
    }
    else
        kprocess_error_current(ERROR_OUT_OF_SYSTEM_MEMORY);
}

void kevent_pulse(EVENT* event)
{
    CHECK_HANDLE(event, sizeof(EVENT));
    CHECK_MAGIC(event, MAGIC_EVENT);

    //release all waiters
    PROCESS* process;
    while (event->waiters)
    {
        process = event->waiters;
        dlist_remove_head((DLIST**)&event->waiters);
        kprocess_wakeup(process);
    }
}

void kevent_set(EVENT* event)
{
    kevent_pulse(event);

    event->set = true;
}

void kevent_is_set(EVENT* event, bool* is_set)
{
    CHECK_HANDLE(event, sizeof(EVENT));
    CHECK_MAGIC(event, MAGIC_EVENT);

    *is_set = event->set;
}

void kevent_clear(EVENT* event)
{
    CHECK_HANDLE(event, sizeof(EVENT));
    CHECK_MAGIC(event, MAGIC_EVENT);
    event->set = false;
}

void kevent_wait(EVENT* event, TIME* time)
{
    CHECK_HANDLE(event, sizeof(EVENT));
    CHECK_MAGIC(event, MAGIC_EVENT);

    PROCESS* process = kprocess_get_current();
    if (!event->set)
    {
        kprocess_sleep_current(time, PROCESS_SYNC_EVENT, event);
        dlist_add_tail((DLIST**)&event->waiters, (DLIST*)process);
    }
}

void kevent_destroy(EVENT* event)
{
    CHECK_HANDLE(event, sizeof(EVENT));
    CHECK_MAGIC(event, MAGIC_EVENT);
    CLEAR_MAGIC(event);

    PROCESS* process;
    while (event->waiters)
    {
        process = event->waiters;
        dlist_remove_head((DLIST**)&event->waiters);
        kprocess_error(process, ERROR_SYNC_OBJECT_DESTROYED);
    }
    kfree(event);
}
