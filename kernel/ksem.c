/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "ksem.h"
#include "kmalloc.h"
#include "../userspace/sys.h"
#include "../userspace/error.h"

void ksem_lock_release(SEM* sem, PROCESS* process)
{
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
        error(ERROR_OUT_OF_SYSTEM_MEMORY);
}

void ksem_signal(SEM* sem)
{
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
    CHECK_MAGIC(sem, MAGIC_SEM);

    PROCESS* process = kprocess_get_current();
    if (sem->value == 0)
    {
        kprocess_sleep(time, PROCESS_SYNC_SEM, sem);
        dlist_add_tail((DLIST**)&sem->waiters, (DLIST*)process);
    }
}

void ksem_destroy(SEM* sem)
{
    CHECK_MAGIC(sem, MAGIC_SEM);

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

