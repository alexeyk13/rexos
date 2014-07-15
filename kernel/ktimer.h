/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef KTIMER_H
#define KTIMER_H

#include "../userspace/lib/time.h"
#include "../userspace/lib/dlist.h"
#include "../userspace/timer.h"

typedef struct {
    DLIST list;
    TIME time;
    void (*callback)(void*);
    void* param;
}KTIMER;

//called from process handler
void ktimer_start(KTIMER* timer);
void ktimer_stop(KTIMER* timer);

//called from svc handler
void ktimer_hpet_timeout();
void ktimer_second_pulse();
void ktimer_get_uptime(TIME* res);
void ktimer_setup(const CB_SVC_TIMER* cb_ktimer, void* cb_ktimer_param);

//called from startup
void ktimer_init();


#endif // KTIMER_H
