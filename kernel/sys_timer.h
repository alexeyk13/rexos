/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef SYS_TIMER_H
#define SYS_TIMER_H

#include "../lib/time.h"
#include "../userspace/dlist.h"

typedef void (*SYS_TIMER_HANDLER)(void*);

typedef struct {
    DLIST list;
    TIME time;
    SYS_TIMER_HANDLER callback;
    void* param;
}TIMER;

void sys_timer_init();

//can be called from SVC/IRQ
void svc_sys_timer_create(TIMER* timer);
void svc_sys_timer_destroy(TIMER* timer);

#endif // SYS_TIMER_H
