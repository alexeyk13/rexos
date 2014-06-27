/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "svc_timer.h"
#include "kernel_config.h"
#include "dbg.h"
#include "kernel.h"
#include "../userspace/core/sys_calls.h"
#include <string.h>

#define FREE_RUN                                        2000000

void svc_timer_get_uptime(TIME* res)
{
    res->sec = __KERNEL->uptime.sec;
    res->usec = __KERNEL->uptime.usec + __KERNEL->cb_svc_timer.elapsed();
    while (res->usec >= 1000000)
    {
        res->sec++;
        res->usec -= 1000000;
    }
}

static inline void find_shoot_next()
{
    TIMER* to_shoot;
    TIME uptime;

    do {
        to_shoot = NULL;
        CRITICAL_ENTER;
        if (__KERNEL->timers)
        {
            //ignore seconds adjustment
            uptime.sec = __KERNEL->uptime.sec;
            uptime.usec = __KERNEL->uptime.usec + __KERNEL->cb_svc_timer.elapsed();
            if (time_compare(&__KERNEL->timers->time, &uptime) >= 0)
            {
                to_shoot = __KERNEL->timers;
                dlist_remove_head((DLIST**)&__KERNEL->timers);
            }
            //add to this second events
            else if (__KERNEL->timers->time.sec == uptime.sec)
            {
                __KERNEL->uptime.usec += __KERNEL->cb_svc_timer.elapsed();
                __KERNEL->cb_svc_timer.stop();
                __KERNEL->hpet_value = __KERNEL->timers->time.usec - __KERNEL->uptime.usec;
                __KERNEL->cb_svc_timer.start(__KERNEL->hpet_value);
            }
        }
        CRITICAL_LEAVE;
        if (to_shoot)
        {
            __KERNEL->timer_inside_isr = true;
            to_shoot->callback(to_shoot->param);
            __KERNEL->timer_inside_isr = false;
        }
    } while (to_shoot);
}

static inline void svc_timer_second_pulse()
{
    __KERNEL->hpet_value = 0;
    __KERNEL->cb_svc_timer.stop();
    __KERNEL->cb_svc_timer.start(FREE_RUN);
    ++__KERNEL->uptime.sec;
    __KERNEL->uptime.usec = 0;

    find_shoot_next();
}

void svc_timer_hpet_timeout()
{
    __KERNEL->uptime.usec += __KERNEL->hpet_value;
    __KERNEL->hpet_value = 0;
    __KERNEL->cb_svc_timer.start(FREE_RUN);

    find_shoot_next();
}

void svc_timer_init(CB_SVC_TIMER* cb_svc_timer)
{
    memcpy(&__KERNEL->cb_svc_timer, cb_svc_timer, sizeof(CB_SVC_TIMER));
    __KERNEL->cb_svc_timer.start(FREE_RUN);
}

void svc_timer_start(TIMER* timer)
{
    TIME uptime;
    DLIST_ENUM de;
    TIMER* cur;
    bool found = false;
    CHECK_CONTEXT(SUPERVISOR_CONTEXT | IRQ_CONTEXT);
    svc_timer_get_uptime(&uptime);
    time_add(&uptime, &timer->time, &timer->time);
    CRITICAL_ENTER;
    dlist_enum_start((DLIST**)&__KERNEL->timers, &de);
    while (dlist_enum(&de, (DLIST**)&cur))
        if (time_compare(&cur->time, &timer->time) < 0)
        {
            dlist_add_before((DLIST**)&__KERNEL->timers, (DLIST*)cur, (DLIST*)timer);
            found = true;
            break;
        }
    if (!found)
        dlist_add_tail((DLIST**)&__KERNEL->timers, (DLIST*)timer);
    CRITICAL_LEAVE;
    if (!__KERNEL->timer_inside_isr)
        find_shoot_next();
}

void svc_timer_stop(TIMER* timer)
{
    CHECK_CONTEXT(SUPERVISOR_CONTEXT | IRQ_CONTEXT);
    CRITICAL_ENTER;
    dlist_remove((DLIST**)&__KERNEL->timers, (DLIST*)timer);
    CRITICAL_LEAVE;
}

void svc_timer_handler(unsigned int num, unsigned int param1, unsigned int param2)
{
    switch (num)
    {
    case SVC_TIMER_HPET_TIMEOUT:
        svc_timer_hpet_timeout();
        break;
    case SVC_TIMER_SECOND_PULSE:
        svc_timer_second_pulse();
        break;
    case SVC_TIMER_GET_UPTIME:
        svc_timer_get_uptime((TIME*)param1);
        break;
    case SVC_TIMER_INIT:
        svc_timer_init((CB_SVC_TIMER*)param1);
        break;
    default:
        error(ERROR_INVALID_SVC);
    }
}
