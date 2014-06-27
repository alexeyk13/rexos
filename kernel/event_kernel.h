/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef EVENT_KERNEL_H
#define EVENT_KERNEL_H

#include "thread_kernel.h"
#include "dbg.h"

typedef struct {
    MAGIC;
    bool set;
    THREAD* waiters;
}EVENT;

//called from thread_private.c on destroy or timeout
void svc_event_lock_release(EVENT* event, THREAD* thread);

void svc_event_handler(unsigned int num, unsigned int param1, unsigned int param2);

#endif // EVENT_KERNEL_H
