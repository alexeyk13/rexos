/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef SEM_KERNEL_H
#define SEM_KERNEL_H

#include "thread_kernel.h"
#include "dbg.h"

typedef struct {
    MAGIC;
    int value;
    THREAD* waiters;
}SEM;

//called from thread_private.c on destroy or timeout
void svc_sem_lock_release(SEM* sem, THREAD* thread);

void svc_sem_handler(unsigned int num, unsigned int param1, unsigned int param2);


#endif // SEM_KERNEL_H
