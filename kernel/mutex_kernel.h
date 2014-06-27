/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef MUTEX_KERNEL_H
#define MUTEX_KERNEL_H

#include "../userspace/dlist.h"
#include "thread_kernel.h"
#include "dbg.h"

typedef struct {
    DLIST list;
    MAGIC;
    //only one item
    THREAD* owner;
    //list
    THREAD* waiters;
}MUTEX;

//update owner priority according lowest priority of owner's all waiters of all mutexes
//can be called from thread_private.c on base priority update
//also called internally on mutex unlock
unsigned int svc_mutex_calculate_owner_priority(THREAD* thread);
//release lock, acquired by mutex. Called from thread_private to release
//locked object - by timeout or thread termination. also can be called on normal release
void svc_mutex_lock_release(MUTEX* mutex, THREAD* thread);

void svc_mutex_handler(unsigned int num, unsigned int param1, unsigned int param2);

#endif // MUTEX_KERNEL_H
