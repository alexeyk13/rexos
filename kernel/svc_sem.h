/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef SVC_SEM_H
#define SVC_SEM_H

#include "svc_process.h"
#include "dbg.h"

typedef struct {
    MAGIC;
    int value;
    PROCESS* waiters;
}SEM;

//called from process_private.c on destroy or timeout
void svc_sem_lock_release(SEM* sem, PROCESS* process);

//called from svc
void svc_sem_create(SEM** sem);
void svc_sem_signal(SEM* sem);
void svc_sem_wait(SEM* sem, TIME* time);
void svc_sem_destroy(SEM* sem);

#endif // SEM_KERNEL_H
