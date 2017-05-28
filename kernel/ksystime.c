/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "ksystime.h"
#include "kernel_config.h"
#include "dbg.h"
#include "kernel.h"
#include <string.h>
#include "../userspace/error.h"
#include "kstdlib.h"
#include "kipc.h"
#include "kprocess_private.h"

#define FREE_RUN                                        2000000

typedef struct {
    MAGIC;
    KTIMER timer;
    HANDLE owner;
    unsigned int param;
    HAL hal;
} SOFT_TIMER;

void hpet_start_stub(unsigned int value, void* param)
{
#if (KERNEL_DEBUG)
    printk("Warning: HPET start stub called\n");
#endif //KERNEL_DEBUG
}

void hpet_stop_stub(void* param)
{
#if (KERNEL_DEBUG)
    printk("Warning: HPET stop stub called\n");
#endif //KERNEL_DEBUG
}

unsigned int hpet_elapsed_stub(void* param)
{
#if (KERNEL_DEBUG)
    printk("Warning: HPET elapsed stub called\n");
#endif //KERNEL_DEBUG
    return 0;
}

void ksystime_get_uptime_internal(SYSTIME* res)
{
    res->sec = __KERNEL->uptime.sec;
    res->usec = __KERNEL->uptime.usec + __KERNEL->cb_ktimer.elapsed(__KERNEL->cb_ktimer_param);
    if (res->usec >= 1000000)
        res->usec = 999999;
}

void ksystime_get_uptime(SYSTIME* res)
{
    disable_interrupts();
    ksystime_get_uptime_internal(res);
    enable_interrupts();
}

static inline void find_shoot_next()
{
    volatile KTIMER* timers_to_shoot = NULL;
    volatile KTIMER* cur;
    SYSTIME uptime;

    disable_interrupts();
    while (__KERNEL->timers)
    {
        ksystime_get_uptime_internal(&uptime);
        if (systime_compare(&__KERNEL->timers->time, &uptime) >= 0)
        {
            cur = __KERNEL->timers;
            cur->active = false;
            dlist_remove_head((DLIST**)&__KERNEL->timers);
            dlist_add_tail((DLIST**)&timers_to_shoot, (DLIST*)cur);
        }
        //add to this second events
        else if (__KERNEL->timers->time.sec == uptime.sec)
        {
            __KERNEL->uptime.usec += __KERNEL->cb_ktimer.elapsed(__KERNEL->cb_ktimer_param);
            __KERNEL->cb_ktimer.stop(__KERNEL->cb_ktimer_param);
            __KERNEL->hpet_value = __KERNEL->timers->time.usec - __KERNEL->uptime.usec;
            __KERNEL->cb_ktimer.start(__KERNEL->hpet_value, __KERNEL->cb_ktimer_param);
            break;
        }
        else
            break;
    }
    enable_interrupts();
    while (timers_to_shoot)
    {
        cur = timers_to_shoot;
        dlist_remove_head((DLIST**)&timers_to_shoot);
        cur->callback(cur->param);
    }
}

void ksystime_second_pulse()
{
    disable_interrupts();
    ++__KERNEL->uptime.sec;
    __KERNEL->hpet_value = 0;
    __KERNEL->cb_ktimer.stop(__KERNEL->cb_ktimer_param);
    __KERNEL->cb_ktimer.start(FREE_RUN, __KERNEL->cb_ktimer_param);
    __KERNEL->uptime.usec = 0;
    enable_interrupts();

    find_shoot_next();
}

void ksystime_hpet_timeout()
{
#if (KERNEL_TIMER_DEBUG)
    if (__KERNEL->hpet_value == 0)
        printk("Warning: HPET timeout on FREE RUN mode: second pulse is inactive or HPET configured improperly\n");
#endif
    disable_interrupts();
    __KERNEL->uptime.usec += __KERNEL->hpet_value;
    __KERNEL->hpet_value = 0;
    __KERNEL->cb_ktimer.start(FREE_RUN, __KERNEL->cb_ktimer_param);
    enable_interrupts();

    find_shoot_next();
}

void ksystime_hpet_setup(const CB_SVC_TIMER *cb_ktimer, void *cb_ktimer_param)
{
    if (__KERNEL->cb_ktimer.start == hpet_start_stub)
    {
        __KERNEL->cb_ktimer.start = cb_ktimer->start;
        __KERNEL->cb_ktimer.stop = cb_ktimer->stop;
        __KERNEL->cb_ktimer.elapsed = cb_ktimer->elapsed;
        __KERNEL->cb_ktimer_param = cb_ktimer_param;
        __KERNEL->cb_ktimer.start(FREE_RUN, __KERNEL->cb_ktimer_param);
    }
    else
        error(ERROR_INVALID_SVC);
}

void ksystime_timer_start_internal(KTIMER* timer, SYSTIME *time)
{
    SYSTIME uptime;
    DLIST_ENUM de;
    KTIMER* cur;
    bool found = false;
    ksystime_get_uptime(&uptime);
    timer->time.sec = time->sec;
    timer->time.usec = time->usec;
    systime_add(&uptime, &timer->time, &timer->time);
    disable_interrupts();
    dlist_enum_start((DLIST**)&__KERNEL->timers, &de);
    while (dlist_enum(&de, (DLIST**)&cur))
        if (systime_compare(&cur->time, &timer->time) < 0)
        {
            dlist_add_before((DLIST**)&__KERNEL->timers, (DLIST*)cur, (DLIST*)timer);
            found = true;
            break;
        }
    if (!found)
        dlist_add_tail((DLIST**)&__KERNEL->timers, (DLIST*)timer);
    timer->active = true;
    enable_interrupts();
    find_shoot_next();
}

void ksystime_timer_stop_internal(KTIMER* timer)
{
    if (timer->active)
    {
        dlist_remove((DLIST**)&__KERNEL->timers, (DLIST*)timer);
        timer->active = false;
    }
}

void ksystime_timer_init_internal(KTIMER* timer, void (*callback)(void*), void* param)
{
    timer->callback = callback;
    timer->param = param;
    timer->active = false;
}

void ksystime_soft_timer_timeout(void* param)
{
    SOFT_TIMER* timer = (SOFT_TIMER*)param;
    IPC ipc;
    ipc.process = timer->owner;
    ipc.cmd = HAL_CMD(timer->hal, IPC_TIMEOUT);
    ipc.param1 = timer->param;
    ipc.param2 = (unsigned int)timer;
    kipc_post(KERNEL_HANDLE, &ipc);
}

HANDLE ksystime_soft_timer_create(HANDLE process, HANDLE param, HAL hal)
{
    SOFT_TIMER* timer = kmalloc(sizeof(SOFT_TIMER));
    if (timer == NULL)
        return INVALID_HANDLE;
    DO_MAGIC(timer, MAGIC_TIMER);
    timer->owner = process;
    timer->param = param;
    timer->hal = hal;
    ksystime_timer_init_internal(&timer->timer, ksystime_soft_timer_timeout, timer);
    return (HANDLE)timer;
}

void ksystime_soft_timer_destroy(HANDLE t)
{
    SOFT_TIMER* timer = (SOFT_TIMER*)t;
    if (t == INVALID_HANDLE)
        return;
    CHECK_MAGIC(timer, MAGIC_TIMER);
    CLEAR_MAGIC(timer);
    kfree(timer);
}

void ksystime_soft_timer_start(HANDLE t, SYSTIME* time)
{
    bool active;
    SOFT_TIMER* timer = (SOFT_TIMER*)t;
    CHECK_MAGIC(timer, MAGIC_TIMER);
    disable_interrupts();
    active = timer->timer.active;
    enable_interrupts();
    if (active)
    {
        error(ERROR_ALREADY_CONFIGURED);
        return;
    }
    ksystime_timer_start_internal(&timer->timer, time);
}

void ksystime_soft_timer_start_ms(HANDLE t, unsigned int ms)
{
    SYSTIME time;
    ms_to_systime(ms, &time);
    ksystime_soft_timer_start(t, &time);
}

void ksystime_soft_timer_start_us(HANDLE t, unsigned int us)
{
    SYSTIME time;
    us_to_systime(us, &time);
    ksystime_soft_timer_start(t, &time);
}

void ksystime_soft_timer_stop(HANDLE t)
{
    SOFT_TIMER* timer = (SOFT_TIMER*)t;
    CHECK_MAGIC(timer, MAGIC_TIMER);
    //in case it shouting right now
    disable_interrupts();
    ksystime_timer_stop_internal(&timer->timer);
    enable_interrupts();
}

void ksystime_init()
{
    __KERNEL->cb_ktimer.start = hpet_start_stub;
    __KERNEL->cb_ktimer.stop = hpet_stop_stub;
    __KERNEL->cb_ktimer.elapsed = hpet_elapsed_stub;
}
