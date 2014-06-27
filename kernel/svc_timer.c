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

static inline TIMER* pop_timer()
{
    TIMER* cur = __KERNEL->timers[0];
    memmove(__KERNEL->timers + 0, __KERNEL->timers + 1, (--__KERNEL->timers_count) * sizeof(void*));
    if (__KERNEL->timers_count >= SYS_TIMER_CACHE_SIZE)
    {
        __KERNEL->timers[SYS_TIMER_CACHE_SIZE - 1] = __KERNEL->timers_uncached;
        dlist_remove_head((DLIST**)&__KERNEL->timers_uncached);
    }
    return cur;
}

static inline void find_shoot_next()
{
    TIMER* to_shoot;
    TIME uptime;

    do {
        to_shoot = NULL;
        CRITICAL_ENTER;
        if (__KERNEL->timers_count)
        {
            //ignore seconds adjustment
            uptime.sec = __KERNEL->uptime.sec;
            uptime.usec = __KERNEL->uptime.usec + __KERNEL->cb_svc_timer.elapsed();
            if (time_compare(&__KERNEL->timers[0]->time, &uptime) >= 0)
                to_shoot = pop_timer();
            //add to this second events
            else if (__KERNEL->timers[0]->time.sec == uptime.sec)
            {
                __KERNEL->uptime.usec += __KERNEL->cb_svc_timer.elapsed();
                __KERNEL->cb_svc_timer.stop();
                __KERNEL->hpet_value = __KERNEL->timers[0]->time.usec - __KERNEL->uptime.usec;
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
    //refactor me
    __KERNEL->cb_svc_timer.start(FREE_RUN);
}

void svc_timer_start(TIMER* timer)
{
    CHECK_CONTEXT(SUPERVISOR_CONTEXT | IRQ_CONTEXT);
    TIME uptime;
    DLIST_ENUM de;
    TIMER* cur;
    bool found = false;
    svc_timer_get_uptime(&uptime);
    time_add(&uptime, &timer->time, &timer->time);
    CRITICAL_ENTER;
    //adjust time, according current uptime
    int list_size = __KERNEL->timers_count;
    if (list_size > SYS_TIMER_CACHE_SIZE)
        list_size = SYS_TIMER_CACHE_SIZE;
    //insert timer into queue
    int pos = 0;
    if (__KERNEL->timers_count)
    {
        int first = 0;
        int last = list_size - 1;
        int mid;
        while (first < last)
        {
            mid = (first + last) >> 1;
            if (time_compare(&__KERNEL->timers[mid]->time, &timer->time) >= 0)
                first = mid + 1;
            else
                last = mid;
        }
        pos = first;
        if (time_compare(&__KERNEL->timers[pos]->time, &timer->time) >= 0)
            ++pos;
    }
    //we have space in cache?
    if (pos < SYS_TIMER_CACHE_SIZE)
    {
        //last is going out ouf cache
        if (__KERNEL->timers_count >= SYS_TIMER_CACHE_SIZE)
            dlist_add_head((DLIST**)&__KERNEL->timers_uncached, (DLIST*)timer);
        memmove(__KERNEL->timers + pos + 1, __KERNEL->timers + pos, (list_size - pos) * sizeof(void*));
        __KERNEL->timers[pos] = timer;

    }
    //find and allocate timer on uncached list
    else
    {
        //top
        if (__KERNEL->timers_uncached == NULL || time_compare(&__KERNEL->timers_uncached->time, &timer->time) < 0)
            dlist_add_head((DLIST**)&__KERNEL->timers_uncached, (DLIST*)timer);
        //in the middle
        else
        {
            dlist_enum_start((DLIST**)&__KERNEL->timers_uncached, &de);
            while (dlist_enum(&de, (DLIST**)&cur))
                if (time_compare(&cur->time, &timer->time) < 0)
                {
                    dlist_add_before((DLIST**)&__KERNEL->timers_uncached, (DLIST*)cur, (DLIST*)timer);
                    break;
                }
        }
    }
    ++__KERNEL->timers_count;
    CRITICAL_LEAVE;

    if (!__KERNEL->timer_inside_isr)
        find_shoot_next();
}

void svc_timer_stop(TIMER* timer)
{
    CHECK_CONTEXT(SUPERVISOR_CONTEXT | IRQ_CONTEXT);
    CRITICAL_ENTER;
    int list_size = __KERNEL->timers_count;
    if (list_size > SYS_TIMER_CACHE_SIZE)
        list_size = SYS_TIMER_CACHE_SIZE;

    int pos = 0;
    int first = 0;
    int last = list_size - 1;
    int mid;
    while (first < last)
    {
        mid = (first + last) >> 1;
        if (time_compare(&__KERNEL->timers[mid]->time, &timer->time) > 0)
            first = mid + 1;
        else
            last = mid;
    }
    pos = first;
    if (time_compare(&__KERNEL->timers[pos]->time, &timer->time) > 0)
        ++pos;

    //timer in cache?
    if (pos < SYS_TIMER_CACHE_SIZE)
    {
        memmove(__KERNEL->timers + pos, __KERNEL->timers + pos + 1, (list_size - pos - 1) * sizeof(void*));
        if (__KERNEL->timers_count >= SYS_TIMER_CACHE_SIZE)
        {
            __KERNEL->timers[SYS_TIMER_CACHE_SIZE - 1] = __KERNEL->timers_uncached;
            dlist_remove_head((DLIST**)&__KERNEL->timers_uncached);
        }
    }
    //timer in uncached area
    else
    {
        DLIST_ENUM de;
        TIMER* cur;
        dlist_enum_start((DLIST**)&__KERNEL->timers_uncached, &de);
        while (dlist_enum(&de, (DLIST**)&cur))
            if (cur == timer)
            {
                dlist_remove((DLIST**)&__KERNEL->timers_uncached, (DLIST*)&cur);
                break;
            }
    }
    --__KERNEL->timers_count;
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
