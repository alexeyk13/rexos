/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "ksem.h"
#include "kmalloc.h"
#include "../userspace/svc.h"
#include "../userspace/error.h"

void ksem_lock_release(SEM* sem, PROCESS* process)
{
    CHECK_HANDLE(sem, sizeof(SEM));
    CHECK_MAGIC(sem, MAGIC_SEM);
    dlist_remove((DLIST**)&sem->waiters, (DLIST*)process);
}

void ksem_create(SEM** sem)
{
    *sem = kmalloc(sizeof(SEM));
    if (*sem != NULL)
    {
        (*sem)->value = 0;
        (*sem)->waiters = NULL;
        DO_MAGIC((*sem), MAGIC_SEM);
    }
    else
        kprocess_error_current(ERROR_OUT_OF_SYSTEM_MEMORY);
}

void ksem_signal(SEM* sem)
{
    CHECK_HANDLE(sem, sizeof(SEM));
    CHECK_MAGIC(sem, MAGIC_SEM);

    sem->value++;
    //release all waiters
    PROCESS* process;
    while (sem->value && sem->waiters)
    {
        process = sem->waiters;
        dlist_remove_head((DLIST**)&sem->waiters);
        kprocess_wakeup(process);
        sem->value--;
    }
}

void ksem_wait(SEM* sem, TIME* time)
{
    CHECK_HANDLE(sem, sizeof(SEM));
    CHECK_MAGIC(sem, MAGIC_SEM);
    PROCESS* process = kprocess_get_current();
    CHECK_ADDRESS(process, time, sizeof(TIME));
    if (sem->value == 0)
    {
        kprocess_sleep_current(time, PROCESS_SYNC_SEM, sem);
        dlist_add_tail((DLIST**)&sem->waiters, (DLIST*)process);
    }
}

void ksem_destroy(SEM* sem)
{
    if ((HANDLE)sem == INVALID_HANDLE)
        return;
    CHECK_HANDLE(sem, sizeof(SEM));
    CHECK_MAGIC(sem, MAGIC_SEM);
    CLEAR_MAGIC(sem);

    PROCESS* process;
    while (sem->waiters)
    {
        process = sem->waiters;
        dlist_remove_head((DLIST**)&sem->waiters);
        kprocess_wakeup(process);
        kprocess_error(process, ERROR_SYNC_OBJECT_DESTROYED);
    }
    kfree(sem);
}

