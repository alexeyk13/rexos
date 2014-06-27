/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef SVC_TIMER_H
#define SVC_TIMER_H

#include "../lib/time.h"
#include "../userspace/dlist.h"

typedef void (*TIMER_HANDLER)(void*);

typedef struct {
    DLIST list;
    TIME time;
    TIMER_HANDLER callback;
    void* param;
}TIMER;

void svc_timer_start(TIMER* timer);
void svc_timer_stop(TIMER* timer);

//called from svc handler
void svc_timer_handler(unsigned int num, unsigned int param1, unsigned int param2);

#endif // SVC_TIMER_H
