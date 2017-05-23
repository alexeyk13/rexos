/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef KSYSTIME_H
#define KSYSTIME_H

#include "../userspace/systime.h"
#include "../userspace/ipc.h"

typedef struct _KTIMER KTIMER;

//called from process handler
void ksystime_timer_start_internal(KTIMER* timer, SYSTIME* time);
void ksystime_timer_stop_internal(KTIMER* timer);
void ksystime_timer_init_internal(KTIMER* timer, void (*callback)(void*), void* param);
void ksystime_get_uptime_internal(SYSTIME* res);

//called from svc handler / exo drivers
void ksystime_hpet_timeout();
void ksystime_second_pulse();
void ksystime_get_uptime(SYSTIME* res);
void ksystime_hpet_setup(const CB_SVC_TIMER* cb_ktimer, void* cb_ktimer_param);

HANDLE ksystime_soft_timer_create(HANDLE process, HANDLE param, HAL hal);
void ksystime_soft_timer_destroy(HANDLE t);
void ksystime_soft_timer_start(HANDLE t, SYSTIME* time);
void ksystime_soft_timer_start_ms(HANDLE t, unsigned int ms);
void ksystime_soft_timer_start_us(HANDLE t, unsigned int us);
void ksystime_soft_timer_stop(HANDLE t);

//called from startup
void ksystime_init();


#endif // KSYSTIME_H
