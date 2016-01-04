/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2016, Alexey Kramarenko
    All rights reserved.
*/

#ifndef KSYSTIME_H
#define KSYSTIME_H

#include "../userspace/time.h"
#include "../userspace/dlist.h"
#include "../userspace/systime.h"
#include "../userspace/ipc.h"
#include "kernel_config.h"
#include "dbg.h"

typedef struct {
    DLIST list;
    SYSTIME time;
    void (*callback)(void*);
    void* param;
    bool active;
} KTIMER;

typedef struct {
    MAGIC;
    KTIMER timer;
    HANDLE owner;
    unsigned int param;
    HAL hal;
} SOFT_TIMER;

//called from process handler
void ksystime_timer_start_internal(KTIMER* timer, SYSTIME* time);
void ksystime_timer_stop_internal(KTIMER* timer);
void ksystime_timer_init_internal(KTIMER* timer, void (*callback)(void*), void* param);
void ksystime_get_uptime_internal(SYSTIME* res);

//called from svc handler
void ksystime_hpet_timeout();
void ksystime_second_pulse();
void ksystime_get_uptime(SYSTIME* res);
void ksystime_hpet_setup(const CB_SVC_TIMER* cb_ktimer, void* cb_ktimer_param);

void ksystime_soft_timer_create(SOFT_TIMER **timer, HANDLE param, HAL hal);
void ksystime_soft_timer_destroy(SOFT_TIMER* timer);
void ksystime_soft_timer_start(SOFT_TIMER* timer, SYSTIME* time);
void ksystime_soft_timer_stop(SOFT_TIMER* timer);

//called from startup
void ksystime_init();


#endif // KSYSTIME_H
