/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef SVC_TIMER_H
#define SVC_TIMER_H

#include "../lib/time.h"
#include "../userspace/dlist.h"
#include "../userspace/timer.h"

typedef struct {
    DLIST list;
    TIME time;
    void (*callback)(void*);
    void* param;
}TIMER;

//called from process handler
void svc_timer_start(TIMER* timer);
void svc_timer_stop(TIMER* timer);

//called from svc handler
void svc_timer_hpet_timeout();
void svc_timer_second_pulse();
void svc_timer_get_uptime(TIME* res);
void svc_timer_setup(const CB_SVC_TIMER* cb_svc_timer);

//called from startup
void svc_timer_init();


#endif // SVC_TIMER_H
