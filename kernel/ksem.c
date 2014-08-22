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
    disable_interrupts();
    dlist_remove((DLIST**)&sem->waiters, (DLIST*)process);
    enable_interrupts();
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

PROCESS* remove_top(SEM* sem)
{
    PROCESS* process = (PROCESS*)INVALID_HANDLE;
    disable_interrupts();
    if (sem->waiters)
    {
        process = sem->waiters;
        dlist_remove_head((DLIST**)&sem->waiters);
        sem->value--;
    }
    enable_interrupts();
    if (process != INVALID_HANDLE)
        kprocess_wakeup(process);
    return process;
}

void ksem_signal(SEM* sem)
{
    int val;
    PROCESS* process;
    CHECK_HANDLE(sem, sizeof(SEM));
    CHECK_MAGIC(sem, MAGIC_SEM);

    disable_interrupts();
    val = ++sem->value;
    enable_interrupts();
    //release all waiters
    for (; val && (process = remove_top(sem) != INVALID_HANDLE); val--) {}
}

void ksem_wait(SEM* sem, TIME* time)
{
    bool val = false;
    CHECK_HANDLE(sem, sizeof(SEM));
    CHECK_MAGIC(sem, MAGIC_SEM);
    PROCESS* process = kprocess_get_current();
    CHECK_ADDRESS(process, time, sizeof(TIME));
    disable_interrupts();
    if (sem->value)
    {
        --sem->value;
        val = true;
    }
    enable_interrupts();
    if (!val)
    {
        kprocess_sleep(process, time, PROCESS_SYNC_SEM, sem);
        disable_interrupts();
        dlist_add_tail((DLIST**)&sem->waiters, (DLIST*)process);
        enable_interrupts();
    }
}

void ksem_destroy(SEM* sem)
{
    PROCESS* process;
    if ((HANDLE)sem == INVALID_HANDLE)
        return;
    CHECK_HANDLE(sem, sizeof(SEM));
    CHECK_MAGIC(sem, MAGIC_SEM);
    CLEAR_MAGIC(sem);

    while ((process = remove_top(sem)) != INVALID_HANDLE) {}
    kfree(sem);
}

