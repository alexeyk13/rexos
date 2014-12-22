/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef KTIMER_H
#define KTIMER_H

#include "../userspace/time.h"
#include "../userspace/dlist.h"
#include "../userspace/timer.h"
#include "kernel_config.h"
#include "dbg.h"

typedef struct {
    DLIST list;
    TIME time;
    void (*callback)(void*);
    void* param;
    bool active;
} KTIMER;

#if (KERNEL_SOFT_TIMERS)
typedef struct {
    MAGIC;
    KTIMER timer;
    HANDLE owner;
    unsigned int mode;
    TIME time;
} SOFT_TIMER;

#endif //KERNEL_SOFT_TIMERS


//called from process handler
void ktimer_start_internal(KTIMER* timer, TIME* time);
void ktimer_stop_internal(KTIMER* timer);
void ktimer_init_internal(KTIMER* timer, void (*callback)(void*), void* param);

//called from svc handler
void ktimer_hpet_timeout();
void ktimer_second_pulse();
void ktimer_get_uptime(TIME* res);
void ktimer_setup(const CB_SVC_TIMER* cb_ktimer, void* cb_ktimer_param);

#if (KERNEL_SOFT_TIMERS)
void ktimer_create(SOFT_TIMER **timer);
void ktimer_destroy(SOFT_TIMER* timer);
void ktimer_start(SOFT_TIMER* timer, TIME* time, unsigned int mode);
void ktimer_stop(SOFT_TIMER* timer);
#endif //KERNEL_SOFT_TIMERS


//called from startup
void ktimer_init();


#endif // KTIMER_H
