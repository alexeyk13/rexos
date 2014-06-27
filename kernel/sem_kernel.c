/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "sem_kernel.h"
#include "mem_kernel.h"
#include "../userspace/core/sys_calls.h"
#include "../userspace/error.h"

static inline void svc_sem_create(SEM** sem)
{
    *sem = sys_alloc(sizeof(SEM));
    if (*sem != NULL)
    {
        (*sem)->value = 0;
        (*sem)->waiters = NULL;
        DO_MAGIC((*sem), MAGIC_SEM);
    }
    else
        error(ERROR_OUT_OF_SYSTEM_MEMORY);
}

void svc_sem_signal(SEM* sem)
{
    CHECK_MAGIC(sem, MAGIC_SEM);

    sem->value++;
    //release all waiters
    THREAD* thread;
    while (sem->value && sem->waiters)
    {
        thread = sem->waiters;
        dlist_remove_head((DLIST**)&sem->waiters);
        svc_thread_wakeup(thread);
        sem->value--;
    }
}

static inline void svc_sem_wait(SEM* sem, TIME* time)
{
    CHECK_MAGIC(sem, MAGIC_SEM);

    THREAD* thread = svc_thread_get_current();
    if (sem->value == 0)
    {
        //first - remove from active list
        //if called from IRQ context, thread_private.c will raise error
        svc_thread_sleep(time, THREAD_SYNC_SEM, sem);
        dlist_add_tail((DLIST**)&sem->waiters, (DLIST*)thread);
    }
}

void svc_sem_lock_release(SEM* sem, THREAD* thread)
{
    CHECK_CONTEXT(SUPERVISOR_CONTEXT | IRQ_CONTEXT);
    CHECK_MAGIC(sem, MAGIC_SEM);
    dlist_remove((DLIST**)&sem->waiters, (DLIST*)thread);
}

static inline void svc_sem_destroy(SEM* sem)
{
    THREAD* thread;
    while (sem->waiters)
    {
        thread = sem->waiters;
        dlist_remove_head((DLIST**)&sem->waiters);
        svc_thread_wakeup(thread);
        svc_thread_error(thread, ERROR_SYNC_OBJECT_DESTROYED);
    }
    sys_free(sem);
}

void svc_sem_handler(unsigned int num, unsigned int param1, unsigned int param2)
{
    CHECK_CONTEXT(SUPERVISOR_CONTEXT | IRQ_CONTEXT);
    CRITICAL_ENTER;
    switch (num)
    {
    case SVC_SEM_CREATE:
        svc_sem_create((SEM**)param1);
        break;
    case SVC_SEM_SIGNAL:
        svc_sem_signal((SEM*)param1);
        break;
    case SVC_SEM_WAIT:
        svc_sem_wait((SEM*)param1, (TIME*)param2);
        break;
    case SVC_SEM_DESTROY:
        svc_sem_destroy((SEM*)param1);
        break;
    default:
        error(ERROR_INVALID_SVC);
    }
    CRITICAL_LEAVE;
}
