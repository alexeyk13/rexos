/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "systime.h"
#include "lib.h"
#include "svc.h"
#include "process.h"

int systime_compare(SYSTIME* from, SYSTIME* to)
{
    return ((const LIB_SYSTIME*)__GLOBAL->lib[LIB_ID_SYSTIME])->lib_systime_compare(from, to);
}

void systime_add(SYSTIME* from, SYSTIME* to, SYSTIME* res)
{
    ((const LIB_SYSTIME*)__GLOBAL->lib[LIB_ID_SYSTIME])->lib_systime_add(from, to, res);
}

void systime_sub(SYSTIME* from, SYSTIME* to, SYSTIME* res)
{
    ((const LIB_SYSTIME*)__GLOBAL->lib[LIB_ID_SYSTIME])->lib_systime_sub(from, to, res);
}

void us_to_systime(int us, SYSTIME* time)
{
    ((const LIB_SYSTIME*)__GLOBAL->lib[LIB_ID_SYSTIME])->lib_us_to_systime(us, time);
}

void ms_to_systime(int ms, SYSTIME* time)
{
    ((const LIB_SYSTIME*)__GLOBAL->lib[LIB_ID_SYSTIME])->lib_ms_to_systime(ms, time);
}

int systime_to_us(SYSTIME* time)
{
    return ((const LIB_SYSTIME*)__GLOBAL->lib[LIB_ID_SYSTIME])->lib_systime_to_us(time);
}

int systime_to_ms(SYSTIME* time)
{
    return ((const LIB_SYSTIME*)__GLOBAL->lib[LIB_ID_SYSTIME])->lib_systime_to_ms(time);
}

SYSTIME* systime_elapsed(SYSTIME* from, SYSTIME* res)
{
    return ((const LIB_SYSTIME*)__GLOBAL->lib[LIB_ID_SYSTIME])->lib_systime_elapsed(from, res);
}

unsigned int systime_elapsed_ms(SYSTIME* from)
{
    return ((const LIB_SYSTIME*)__GLOBAL->lib[LIB_ID_SYSTIME])->lib_systime_elapsed_ms(from);
}

unsigned int systime_elapsed_us(SYSTIME* from)
{
    return ((const LIB_SYSTIME*)__GLOBAL->lib[LIB_ID_SYSTIME])->lib_systime_elapsed_us(from);
}

void get_uptime(SYSTIME* uptime)
{
    svc_call(SVC_SYSTIME_GET_UPTIME, (unsigned int)uptime, 0, 0);
}

void systime_hpet_setup(CB_SVC_TIMER* cb_svc_timer, void* cb_svc_timer_param)
{
    svc_call(SVC_SYSTIME_HPET_SETUP, (unsigned int)cb_svc_timer, (unsigned int)cb_svc_timer_param, 0);
}

void systime_second_pulse()
{
    __GLOBAL->svc_irq(SVC_SYSTIME_SECOND_PULSE, 0, 0, 0);
}

void systime_hpet_timeout()
{
    __GLOBAL->svc_irq(SVC_SYSTIME_HPET_TIMEOUT, 0, 0, 0);
}

HANDLE timer_create(unsigned int param, HAL hal)
{
    HANDLE handle;
    svc_call(SVC_SYSTIME_SOFT_TIMER_CREATE, (unsigned int)&handle, param, (unsigned int)hal);
    return handle;
}

void timer_start(HANDLE timer, SYSTIME* time)
{
    svc_call(SVC_SYSTIME_SOFT_TIMER_START, (unsigned int)timer, (unsigned int)time, 0);
}

void timer_istart(HANDLE timer, SYSTIME* time)
{
    __GLOBAL->svc_irq(SVC_SYSTIME_SOFT_TIMER_START, (unsigned int)timer, (unsigned int)time, 0);
}

void timer_start_ms(HANDLE timer, unsigned int time_ms)
{
    SYSTIME time;
    ms_to_systime(time_ms, &time);
    timer_start(timer, &time);
}

void timer_istart_ms(HANDLE timer, unsigned int time_ms)
{
    SYSTIME time;
    ms_to_systime(time_ms, &time);
    timer_istart(timer, &time);
}

void timer_start_us(HANDLE timer, unsigned int time_us)
{
    SYSTIME time;
    us_to_systime(time_us, &time);
    timer_start(timer, &time);
}

void timer_istart_us(HANDLE timer, unsigned int time_us)
{
    SYSTIME time;
    us_to_systime(time_us, &time);
    timer_istart(timer, &time);
}

void timer_stop(HANDLE timer, unsigned int param, HAL hal)
{
    svc_call(SVC_SYSTIME_SOFT_TIMER_STOP, (unsigned int)timer, 0, 0);
    //remove IPC if timer elapsed during timer stop
    ipc_remove(KERNEL_HANDLE, HAL_CMD(hal, IPC_TIMEOUT), param);
}

void timer_istop(HANDLE timer)
{
    __GLOBAL->svc_irq(SVC_SYSTIME_SOFT_TIMER_STOP, (unsigned int)timer, 0, 0);
}

void timer_destroy(HANDLE timer)
{
    svc_call(SVC_SYSTIME_SOFT_TIMER_DESTROY, (unsigned int)timer, 0, 0);
}
