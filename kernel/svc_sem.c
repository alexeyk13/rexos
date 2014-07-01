/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "svc_sem.h"
#include "svc_malloc.h"
#include "../userspace/core/sys_calls.h"
#include "../userspace/error.h"

void svc_sem_lock_release(SEM* sem, PROCESS* process)
{
    CHECK_MAGIC(sem, MAGIC_SEM);
    dlist_remove((DLIST**)&sem->waiters, (DLIST*)process);
}

void svc_sem_create(SEM** sem)
{
    *sem = kmalloc(sizeof(SEM));
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
    PROCESS* process;
    while (sem->value && sem->waiters)
    {
        process = sem->waiters;
        dlist_remove_head((DLIST**)&sem->waiters);
        svc_process_wakeup(process);
        sem->value--;
    }
}

void svc_sem_wait(SEM* sem, TIME* time)
{
    CHECK_MAGIC(sem, MAGIC_SEM);

    PROCESS* process = svc_process_get_current();
    if (sem->value == 0)
    {
        svc_process_sleep(time, PROCESS_SYNC_SEM, sem);
        dlist_add_tail((DLIST**)&sem->waiters, (DLIST*)process);
    }
}

void svc_sem_destroy(SEM* sem)
{
    CHECK_MAGIC(sem, MAGIC_SEM);

    PROCESS* process;
    while (sem->waiters)
    {
        process = sem->waiters;
        dlist_remove_head((DLIST**)&sem->waiters);
        svc_process_wakeup(process);
        svc_process_error(process, ERROR_SYNC_OBJECT_DESTROYED);
    }
    kfree(sem);
}

