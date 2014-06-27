/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "sys_timer.h"
#include "kernel_config.h"
#include "timer.h"
#include "dbg.h"
#include "core/core_kernel.h"
#include "sys_calls.h"
#include "sys_call.h"
#include <string.h>
#if (SYS_TIMER_SOFT_RTC == 0)
#include "rtc.h"
#endif //SYS_TIMER_SOFT_RTC

#define TIMER_ONE_SECOND                                                                                    1000000
#define TIMER_FREE_RUN                                                                                        (TIMER_ONE_SECOND * 2)

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

    do {
        to_shoot = NULL;
        CRITICAL_ENTER;
        if (__KERNEL->timers_count)
        {
            if (__KERNEL->timers[0]->time.sec < get_uptime() || ((__KERNEL->timers[0]->time.sec == get_uptime()) && (__KERNEL->timers[0]->time.usec <= __KERNEL->uptime_usec)))
                to_shoot = pop_timer();
            else if (__KERNEL->timers[0]->time.sec == get_uptime())
            {
                __KERNEL->uptime_usec += timer_elapsed(SYS_TIMER_HPET);
                timer_stop(SYS_TIMER_HPET);
                __KERNEL->hpet_value = __KERNEL->timers[0]->time.usec - __KERNEL->uptime_usec;
                timer_start(SYS_TIMER_HPET, __KERNEL->hpet_value);
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

void hpet_on_isr(TIMER_CLASS timer)
{
    __KERNEL->uptime_usec += __KERNEL->hpet_value;
    __KERNEL->hpet_value = 0;
    timer_start(SYS_TIMER_HPET, TIMER_FREE_RUN);
    find_shoot_next();
}

#if (SYS_TIMER_SOFT_RTC)
void rtc_on_isr(TIMER_CLASS timer)
#else
void rtc_on_isr(RTC_CLASS rtc)
#endif //SYS_TIMER_SOFT_RTC
{
    __KERNEL->hpet_value = 0;
    timer_stop(SYS_TIMER_HPET);
    timer_start(SYS_TIMER_HPET, TIMER_FREE_RUN);
    ++__GLOBAL->uptime;
    __KERNEL->uptime_usec = 0;

    find_shoot_next();
}

void sys_timer_init()
{
#if (SYS_TIMER_SOFT_RTC)
    timer_enable(SYS_TIMER_SOFT_RTC, rtc_on_isr, SYS_TIMER_PRIORITY, 0);
    timer_start(SYS_TIMER_SOFT_RTC, 1000000);
#else
    rtc_enable_second_tick(SYS_TIMER_RTC, rtc_on_isr, SYS_TIMER_PRIORITY);
#endif //SYS_TIMER_SOFT_RTC
    timer_enable(SYS_TIMER_HPET, hpet_on_isr, SYS_TIMER_PRIORITY, TIMER_FLAG_ONE_PULSE_MODE);
    timer_start(SYS_TIMER_HPET, TIMER_FREE_RUN);
}

void svc_sys_timer_create(TIMER* timer)
{
    CHECK_CONTEXT(SUPERVISOR_CONTEXT | IRQ_CONTEXT);
    TIME uptime;
    uptime.sec = get_uptime();
    uptime.usec = __KERNEL->uptime_usec + timer_elapsed(SYS_TIMER_HPET);
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
        //bottom
        else if (time_compare(&((TIMER*)__KERNEL->timers_uncached->list.prev)->time, &timer->time) > 0)
            dlist_add_tail((DLIST**)&__KERNEL->timers_uncached, (DLIST*)timer);
        //in the middle
        else
        {
            DLIST_ENUM de;
            TIMER* cur;
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

void svc_sys_timer_destroy(TIMER* timer)
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
