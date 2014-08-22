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
    disable_interrupts();
    dlist_remove((DLIST**)&event->waiters, (DLIST*)process);
    enable_interrupts();
}

static inline PROCESS* remove_top(EVENT* event)
{
    PROCESS* process = (PROCESS*)INVALID_HANDLE;
    disable_interrupts();
    if (event->waiters)
    {
        process = event->waiters;
        dlist_remove_head((DLIST**)&event->waiters);
    }
    enable_interrupts();
    if (process != INVALID_HANDLE)
        kprocess_wakeup(process);
    return process;
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

    while (remove_top(event) != INVALID_HANDLE) {}
}

void kevent_set(EVENT* event)
{
    CHECK_HANDLE(event, sizeof(EVENT));
    CHECK_MAGIC(event, MAGIC_EVENT);

    while (remove_top(event) != INVALID_HANDLE) {}
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
    //already set? do nothing.
    if (event->set)
        return;
    PROCESS* process = kprocess_get_current();

    kprocess_sleep(process, time, PROCESS_SYNC_EVENT, event);
    disable_interrupts();
    if (!event->set)
    {
        dlist_add_tail((DLIST**)&event->waiters, (DLIST*)process);
        process = INVALID_HANDLE;
    }
    enable_interrupts();
    //late set arrival in interrupt
    if (process != INVALID_HANDLE)
        kprocess_wakeup(process);
}

void kevent_destroy(EVENT* event)
{
    if ((HANDLE)event == INVALID_HANDLE)
        return;
    CHECK_HANDLE(event, sizeof(EVENT));
    CHECK_MAGIC(event, MAGIC_EVENT);
    CLEAR_MAGIC(event);

    while (remove_top(event) != INVALID_HANDLE) {}
    kfree(event);
}
