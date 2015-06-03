/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "ktimer.h"
#include "kernel_config.h"
#include "dbg.h"
#include "kernel.h"
#include <string.h>
#include "../userspace/error.h"
#if (KERNEL_SOFT_TIMERS)
#include "kmalloc.h"
#include "kipc.h"
#endif //KERNEL_SOFT_TIMERS

#define FREE_RUN                                        2000000

void hpet_start_stub(unsigned int value, void* param)
{
#if (KERNEL_INFO)
    printk("Warning: HPET start stub called\n\r");
#endif //KERNEL_INFO
    kprocess_error_current(ERROR_STUB_CALLED);
}

void hpet_stop_stub(void* param)
{
#if (KERNEL_INFO)
    printk("Warning: HPET stop stub called\n\r");
#endif //KERNEL_INFO
    kprocess_error_current(ERROR_STUB_CALLED);
}

unsigned int hpet_elapsed_stub(void* param)
{
#if (KERNEL_INFO)
    printk("Warning: HPET elapsed stub called\n\r");
#endif //KERNEL_INFO
    kprocess_error_current(ERROR_STUB_CALLED);
    return 0;
}

void ktimer_get_uptime_internal(TIME* res)
{
    res->sec = __KERNEL->uptime.sec;
    res->usec = __KERNEL->uptime.usec + __KERNEL->cb_ktimer.elapsed(__KERNEL->cb_ktimer_param);
    if (res->usec >= 1000000)
        res->usec = 999999;
}

void ktimer_get_uptime(TIME* res)
{
    disable_interrupts();
    ktimer_get_uptime_internal(res);
    enable_interrupts();
}

static inline void find_shoot_next()
{
    volatile KTIMER* timers_to_shoot = NULL;
    volatile KTIMER* cur;
    TIME uptime;

    disable_interrupts();
    while (__KERNEL->timers)
    {
        ktimer_get_uptime_internal(&uptime);
        if (time_compare(&__KERNEL->timers->time, &uptime) >= 0)
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

void ktimer_second_pulse()
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

void ktimer_hpet_timeout()
{
#if (KERNEL_TIMER_DEBUG)
    if (__KERNEL->hpet_value == 0)
        printk("Warning: HPET timeout on FREE RUN mode: second pulse is inactive or HPET configured improperly");
#endif
    disable_interrupts();
    __KERNEL->uptime.usec += __KERNEL->hpet_value;
    __KERNEL->hpet_value = 0;
    __KERNEL->cb_ktimer.start(FREE_RUN, __KERNEL->cb_ktimer_param);
    enable_interrupts();

    find_shoot_next();
}

void ktimer_setup(const CB_SVC_TIMER *cb_ktimer, void *cb_ktimer_param)
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
        kprocess_error_current(ERROR_INVALID_SVC);
}

void ktimer_start_internal(KTIMER* timer, TIME *time)
{
    TIME uptime;
    DLIST_ENUM de;
    KTIMER* cur;
    bool found = false;
    ktimer_get_uptime(&uptime);
    timer->time.sec = time->sec;
    timer->time.usec = time->usec;
    time_add(&uptime, &timer->time, &timer->time);
    disable_interrupts();
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
    timer->active = true;
    enable_interrupts();
    find_shoot_next();
}

void ktimer_stop_internal(KTIMER* timer)
{
    if (timer->active)
    {
        dlist_remove((DLIST**)&__KERNEL->timers, (DLIST*)timer);
        timer->active = false;
    }
}

void ktimer_init_internal(KTIMER* timer, void (*callback)(void*), void* param)
{
    timer->callback = callback;
    timer->param = param;
    timer->active = false;
}

#if (KERNEL_SOFT_TIMERS)
void ktimer_timeout(void* param)
{
    SOFT_TIMER* timer = (SOFT_TIMER*)param;
    IPC ipc;
    ipc.process = timer->owner;
    ipc.cmd = HAL_CMD(timer->hal, IPC_TIMEOUT);
    ipc.param1 = timer->param;
    ipc.param2 = (unsigned int)timer;
    kipc_post_process(&ipc, KERNEL_HANDLE);
    if (timer->mode & TIMER_MODE_PERIODIC)
        ktimer_start_internal(&timer->timer, &timer->time);
}

void ktimer_create(SOFT_TIMER** timer, unsigned int param, HAL hal)
{
    PROCESS* process = kprocess_get_current();
    CHECK_ADDRESS(process, timer, sizeof(void*));
    *timer = kmalloc(sizeof(SOFT_TIMER));
    if ((*timer) == NULL)
    {
        kprocess_error(process, ERROR_OUT_OF_SYSTEM_MEMORY);
        return;
    }
    DO_MAGIC((*timer), MAGIC_TIMER);
    (*timer)->owner = (HANDLE)process;
    (*timer)->param = param;
    (*timer)->hal = hal;
    ktimer_init_internal(&(*timer)->timer, ktimer_timeout, (*timer));
}

void ktimer_destroy(SOFT_TIMER *timer)
{
    if ((HANDLE)timer == INVALID_HANDLE)
        return;
    CHECK_HANDLE(timer, sizeof(SOFT_TIMER));
    CHECK_MAGIC(timer, MAGIC_TIMER);
    CLEAR_MAGIC(timer);
    kfree(timer);
}

void ktimer_start(SOFT_TIMER* timer, TIME* time, unsigned int mode)
{
    bool active;
    PROCESS* process = kprocess_get_current();
    CHECK_HANDLE(timer, sizeof(SOFT_TIMER));
    CHECK_MAGIC(timer, MAGIC_TIMER);
    CHECK_ADDRESS(process, time, sizeof(TIME));
    disable_interrupts();
    active = timer->timer.active;
    enable_interrupts();
    if (active)
    {
        kprocess_error(process, ERROR_ALREADY_CONFIGURED);
        return;
    }
    timer->mode = mode;
    if (timer->mode & TIMER_MODE_PERIODIC)
    {
        timer->time.sec = time->sec;
        timer->time.usec = time->usec;
    }
    ktimer_start_internal(&timer->timer, time);
}

void ktimer_stop(SOFT_TIMER* timer)
{
    CHECK_HANDLE(timer, sizeof(SOFT_TIMER));
    CHECK_MAGIC(timer, MAGIC_TIMER);
    //in case it shouting right now
    disable_interrupts();
    timer->mode &= ~TIMER_MODE_PERIODIC;
    ktimer_stop_internal(&timer->timer);
    enable_interrupts();
}
#endif //KERNEL_SOFT_TIMERS

void ktimer_init()
{
    __KERNEL->cb_ktimer.start = hpet_start_stub;
    __KERNEL->cb_ktimer.stop = hpet_stop_stub;
    __KERNEL->cb_ktimer.elapsed = hpet_elapsed_stub;
}
