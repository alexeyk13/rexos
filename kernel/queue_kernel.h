/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef QUEUE_KERNEL_H
#define QUEUE_KERNEL_H

#include "thread_kernel.h"
#include "dbg.h"
#include "../userspace/dlist.h"

typedef struct {
    MAGIC;
    void* mem_block;
    DLIST* free_blocks;
    DLIST* filled_blocks;
    THREAD* push_waiters;
    THREAD* pull_waiters;
}QUEUE;

//called from thread_private.c on destroy or timeout
void svc_queue_lock_release(QUEUE* queue, THREAD* thread);

unsigned int svc_queue_handler(unsigned int num, unsigned int param1, unsigned int param2, unsigned int param3);

#endif // QUEUE_KERNEL_H
