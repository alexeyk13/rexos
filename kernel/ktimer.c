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

#define FREE_RUN                                        2000000

void hpet_start_stub(unsigned int value)
{
#if (KERNEL_INFO)
    printk("Warning: HPET start stub called\n\r");
#endif //KERNEL_INFO
    kprocess_error_current(ERROR_STUB_CALLED);
}

void hpet_stop_stub()
{
#if (KERNEL_INFO)
    printk("Warning: HPET stop stub called\n\r");
#endif //KERNEL_INFO
    kprocess_error_current(ERROR_STUB_CALLED);
}

unsigned int hpet_elapsed_stud()
{
#if (KERNEL_INFO)
    printk("Warning: HPET elapsed stub called\n\r");
#endif //KERNEL_INFO
    kprocess_error_current(ERROR_STUB_CALLED);
    return 0;
}

const CB_SVC_TIMER cb_ktimer_stub = {
    hpet_start_stub,
    hpet_stop_stub,
    hpet_elapsed_stud
};

void ktimer_get_uptime(TIME* res)
{
    res->sec = __KERNEL->uptime.sec;
    res->usec = __KERNEL->uptime.usec + __KERNEL->cb_ktimer.elapsed();
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
        if (__KERNEL->timers)
        {
            //ignore seconds adjustment
            uptime.sec = __KERNEL->uptime.sec;
            uptime.usec = __KERNEL->uptime.usec + __KERNEL->cb_ktimer.elapsed();
            if (time_compare(&__KERNEL->timers->time, &uptime) >= 0)
            {
                to_shoot = __KERNEL->timers;
                dlist_remove_head((DLIST**)&__KERNEL->timers);
            }
            //add to this second events
            else if (__KERNEL->timers->time.sec == uptime.sec)
            {
                __KERNEL->uptime.usec += __KERNEL->cb_ktimer.elapsed();
                __KERNEL->cb_ktimer.stop();
                __KERNEL->hpet_value = __KERNEL->timers->time.usec - __KERNEL->uptime.usec;
                __KERNEL->cb_ktimer.start(__KERNEL->hpet_value);
            }
            if (to_shoot)
            {
                __KERNEL->timer_executed = true;
                to_shoot->callback(to_shoot->param);
                __KERNEL->timer_executed = false;
            }
        }
    } while (to_shoot);
}

void ktimer_second_pulse()
{
    __KERNEL->hpet_value = 0;
    __KERNEL->cb_ktimer.stop();
    __KERNEL->cb_ktimer.start(FREE_RUN);
    ++__KERNEL->uptime.sec;
    __KERNEL->uptime.usec = 0;

    find_shoot_next();
}

void ktimer_hpet_timeout()
{
#if (KERNEL_TIMER_DEBUG)
    if (__KERNEL->hpet_value == 0)
        printk("Warning: HPET timeout on FREE RUN mode: second pulse is inactive or HPET configured improperly");
#endif
    __KERNEL->uptime.usec += __KERNEL->hpet_value;
    __KERNEL->hpet_value = 0;
    __KERNEL->cb_ktimer.start(FREE_RUN);

    find_shoot_next();
}

void ktimer_setup(const CB_SVC_TIMER *cb_ktimer)
{
    if (!__KERNEL->timer_locked)
    {
        memcpy(&__KERNEL->cb_ktimer, cb_ktimer, sizeof(CB_SVC_TIMER));
        __KERNEL->cb_ktimer.start(FREE_RUN);
        __KERNEL->timer_locked = true;
    }
    else
        kprocess_error_current(ERROR_INVALID_SVC);
}

void ktimer_start(TIMER* timer)
{
    TIME uptime;
    DLIST_ENUM de;
    TIMER* cur;
    bool found = false;
    ktimer_get_uptime(&uptime);
    time_add(&uptime, &timer->time, &timer->time);
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
    if (!__KERNEL->timer_executed)
        find_shoot_next();
}

void ktimer_stop(TIMER* timer)
{
    dlist_remove((DLIST**)&__KERNEL->timers, (DLIST*)timer);
}

void ktimer_init()
{
    memcpy(&__KERNEL->cb_ktimer, &cb_ktimer_stub, sizeof(CB_SVC_TIMER));
}
