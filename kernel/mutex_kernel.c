/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "mutex_kernel.h"
#include "mem_kernel.h"
#include "../userspace/core/sys_calls.h"
#include "../userspace/error.h"

static inline void svc_mutex_create(MUTEX** mutex)
{
    *mutex = sys_alloc(sizeof(MUTEX));
    if (*mutex != NULL)
    {
        (*mutex)->owner = NULL;
        (*mutex)->waiters = NULL;
        DO_MAGIC((*mutex), MAGIC_MUTEX);
    }
    else
        error(ERROR_OUT_OF_SYSTEM_MEMORY);
}

unsigned int svc_mutex_calculate_owner_priority(THREAD* thread)
{
    unsigned int priority = thread->base_priority;
    DLIST_ENUM owned_mutexes, thread_waiters;
    MUTEX* current_mutex;
    THREAD* current_thread;
    dlist_enum_start(&thread->owned_mutexes, &owned_mutexes);
    while (dlist_enum(&owned_mutexes, (DLIST**)&current_mutex))
    {
        dlist_enum_start((DLIST**)&current_mutex->waiters, &thread_waiters);
        while (dlist_enum(&thread_waiters, (DLIST**)&current_thread))
            if (current_thread->current_priority < priority)
                priority = current_thread->current_priority;
    }
    return priority;
}

static inline void svc_mutex_lock(MUTEX* mutex, TIME* time)
{
    CHECK_MAGIC(mutex, MAGIC_MUTEX);
    THREAD* thread = svc_thread_get_current();
    if (mutex->owner != NULL)
    {
        ASSERT(mutex->owner != thread);
        //first - remove from active list
        svc_thread_sleep(time, THREAD_SYNC_MUTEX, mutex);
        //add to mutex watiers list
        dlist_add_tail((DLIST**)&mutex->waiters, (DLIST*)thread);
    }
    //we are first. just lock and add to owned
    else
    {
        mutex->owner = thread;
        dlist_add_tail((DLIST**)&mutex->owner->owned_mutexes, (DLIST*)mutex);
    }
}

void svc_mutex_lock_release(MUTEX* mutex, THREAD* thread)
{
    //it's the only mutex call, that can be made in irq context - on sys_timer timeout call from thread_private.c
    CHECK_CONTEXT(SUPERVISOR_CONTEXT | IRQ_CONTEXT);
    CHECK_MAGIC(mutex, MAGIC_MUTEX);
    //release mutex owner
    if (thread == mutex->owner)
    {
        //thread now is not owning mutex, remove it from owned list and calculate new priority (he is still can own nested mutexes)
        dlist_remove((DLIST**)&thread->owned_mutexes, (DLIST*)mutex);
        svc_thread_set_current_priority(thread, svc_mutex_calculate_owner_priority(thread));

        mutex->owner = mutex->waiters;
        if (mutex->owner)
        {
            dlist_remove_head((DLIST**)&mutex->waiters);
            dlist_add_tail((DLIST**)&mutex->owner->owned_mutexes, (DLIST*)mutex);
            //owner can still depends on some waiters
            svc_thread_set_current_priority(mutex->owner, svc_mutex_calculate_owner_priority(mutex->owner));
            svc_thread_wakeup(mutex->owner);
        }
    }
    //remove item from waiters list
    else
    {
        dlist_remove((DLIST**)&mutex->waiters, (DLIST*)thread);
        //this can affect on owner priority
        svc_thread_set_current_priority(mutex->owner, svc_mutex_calculate_owner_priority(mutex->owner));
        //it's up to caller to decide, wake up thread (timeout, mutex destroy) or not (thread terminate) owned process
    }
}

static inline void svc_mutex_unlock(MUTEX* mutex)
{
    THREAD* thread = svc_thread_get_current();
    ASSERT(mutex->owner == thread);
    svc_mutex_lock_release(mutex, thread);
}

static inline void svc_mutex_destroy(MUTEX* mutex)
{
    THREAD* thread;
    while (mutex->waiters)
    {
        thread = mutex->waiters;
        svc_mutex_lock_release(mutex, mutex->waiters);
        svc_thread_wakeup(thread);
        svc_thread_error(thread, ERROR_SYNC_OBJECT_DESTROYED);
    }
    if (mutex->owner)
        svc_mutex_lock_release(mutex, mutex->owner);
    sys_free(mutex);
}

void svc_mutex_handler(unsigned int num, unsigned int param1, unsigned int param2)
{
    CHECK_CONTEXT(SUPERVISOR_CONTEXT);
    CRITICAL_ENTER;
    switch (num)
    {
    case SVC_MUTEX_CREATE:
        svc_mutex_create((MUTEX**)param1);
        break;
    case SVC_MUTEX_LOCK:
        svc_mutex_lock((MUTEX*)param1, (TIME*)param2);
        break;
    case SVC_MUTEX_UNLOCK:
        svc_mutex_unlock((MUTEX*)param1);
        break;
    case SVC_MUTEX_DESTROY:
        svc_mutex_destroy((MUTEX*)param1);
        break;
    default:
        error(ERROR_INVALID_SVC);
    }
    CRITICAL_LEAVE;
}
