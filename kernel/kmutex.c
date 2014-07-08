/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "kmutex.h"
#include "kmalloc.h"
#include "../userspace/error.h"

unsigned int kmutex_calculate_owner_priority(PROCESS *process)
{
    unsigned int priority = process->base_priority;
    DLIST_ENUM owned_mutexes, process_waiters;
    MUTEX* current_mutex;
    PROCESS* current_process;
    dlist_enum_start(&process->owned_mutexes, &owned_mutexes);
    while (dlist_enum(&owned_mutexes, (DLIST**)&current_mutex))
    {
        dlist_enum_start((DLIST**)&current_mutex->waiters, &process_waiters);
        while (dlist_enum(&process_waiters, (DLIST**)&current_process))
            if (current_process->current_priority < priority)
                priority = current_process->current_priority;
    }
    return priority;
}

void kmutex_lock_release(MUTEX* mutex, PROCESS* process)
{
    CHECK_MAGIC(mutex, MAGIC_MUTEX);
    //release mutex owner
    if (process == mutex->owner)
    {
        //process now is not owning mutex, remove it from owned list and calculate new priority (he is still can own nested mutexes)
        dlist_remove((DLIST**)&process->owned_mutexes, (DLIST*)mutex);
        kprocess_set_current_priority(process, kmutex_calculate_owner_priority(process));

        mutex->owner = mutex->waiters;
        if (mutex->owner)
        {
            dlist_remove_head((DLIST**)&mutex->waiters);
            dlist_add_tail((DLIST**)&mutex->owner->owned_mutexes, (DLIST*)mutex);
            //owner can still depends on some waiters
            kprocess_set_current_priority(mutex->owner, kmutex_calculate_owner_priority(mutex->owner));
            kprocess_wakeup(mutex->owner);
        }
    }
    //remove item from waiters list
    else
    {
        dlist_remove((DLIST**)&mutex->waiters, (DLIST*)process);
        //this can affect on owner priority
        kprocess_set_current_priority(mutex->owner, kmutex_calculate_owner_priority(mutex->owner));
        //it's up to caller to decide, wake up process (timeout, mutex destroy) or not (process terminate) owned process
    }
}

void kmutex_create(MUTEX** mutex)
{
    *mutex = kmalloc(sizeof(MUTEX));
    if (*mutex != NULL)
    {
        (*mutex)->owner = NULL;
        (*mutex)->waiters = NULL;
        DO_MAGIC((*mutex), MAGIC_MUTEX);
    }
    else
        kprocess_error_current(ERROR_OUT_OF_SYSTEM_MEMORY);
}

void kmutex_lock(MUTEX* mutex, TIME* time)
{
    CHECK_MAGIC(mutex, MAGIC_MUTEX);
    PROCESS* process = kprocess_get_current();
    if (mutex->owner != NULL)
    {
        ASSERT(mutex->owner != process);
        //first - remove from active list
        kprocess_sleep_current(time, PROCESS_SYNC_MUTEX, mutex);
        //add to mutex watiers list
        dlist_add_tail((DLIST**)&mutex->waiters, (DLIST*)process);
    }
    //we are first. just lock and add to owned
    else
    {
        mutex->owner = process;
        dlist_add_tail((DLIST**)&mutex->owner->owned_mutexes, (DLIST*)mutex);
    }
}

void kmutex_unlock(MUTEX* mutex)
{
    CHECK_MAGIC(mutex, MAGIC_MUTEX);

    PROCESS* process = kprocess_get_current();
    ASSERT(mutex->owner == process);
    kmutex_lock_release(mutex, process);
}

void kmutex_destroy(MUTEX* mutex)
{
    CHECK_MAGIC(mutex, MAGIC_MUTEX);

    PROCESS* process;
    while (mutex->waiters)
    {
        process = mutex->waiters;
        kmutex_lock_release(mutex, mutex->waiters);
        kprocess_wakeup(process);
        kprocess_error(process, ERROR_SYNC_OBJECT_DESTROYED);
    }
    if (mutex->owner)
        kmutex_lock_release(mutex, mutex->owner);
    kfree(mutex);
}
