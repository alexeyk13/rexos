/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "process.h"
#include "svc.h"

HANDLE process_create(const REX* rex)
{
    HANDLE handle;
    svc_call(SVC_PROCESS_CREATE, (unsigned int)rex, (unsigned int)&handle, 0);
    return handle;
}

HANDLE process_get_current()
{
    HANDLE handle;
    svc_call(SVC_PROCESS_GET_CURRENT, (unsigned int)&handle, 0, 0);
    return handle;
}

const char* process_name()
{
    return __PROCESS->name;
}

int get_last_error()
{
    return __PROCESS->error;
}

void error(int error)
{
    __PROCESS->error = error;
}

HANDLE process_iget_current()
{
    HANDLE handle;
    __GLOBAL->svc_irq(SVC_PROCESS_GET_CURRENT, (unsigned int)&handle, 0, 0);
    return handle;
}

unsigned int process_get_flags(HANDLE process)
{
    unsigned int flags;
    svc_call(SVC_PROCESS_GET_FLAGS, (unsigned int)process, (unsigned int)&flags, 0);
    return flags;
}

void process_set_flags(HANDLE process, unsigned int flags)
{
    svc_call(SVC_PROCESS_SET_FLAGS, (unsigned int)process, flags, 0);
}

void process_unfreeze(HANDLE process)
{
    process_set_flags(process, PROCESS_FLAGS_ACTIVE);
}

void process_freeze(HANDLE process)
{
    process_set_flags(process, 0);
}

unsigned int process_get_priority(HANDLE process)
{
    unsigned int priority;
    svc_call(SVC_PROCESS_GET_PRIORITY, (unsigned int)process, (unsigned int)&priority, 0);
    return priority;
}

unsigned int process_get_current_priority()
{
    return process_get_priority(process_get_current());
}

void process_set_priority(HANDLE process, unsigned int priority)
{
    svc_call(SVC_PROCESS_SET_PRIORITY, (unsigned int)process, priority, 0);
}

void process_set_current_priority(unsigned int priority)
{
    process_set_priority(process_get_current(), priority);
}

void process_destroy(HANDLE process)
{
    svc_call(SVC_PROCESS_DESTROY, (unsigned int)process, 0, 0);
}

void process_exit()
{
    process_destroy(process_get_current());
}

void sleep(SYSTIME* time)
{
    svc_call(SVC_PROCESS_SLEEP, (unsigned int)time, 0, 0);
}

void sleep_ms(unsigned int ms)
{
    SYSTIME time;
    ms_to_systime(ms, &time);
    sleep(&time);
}

void sleep_us(unsigned int us)
{
    SYSTIME time;
    us_to_systime(us, &time);
    sleep(&time);
}

void process_switch_test()
{
    svc_call(SVC_PROCESS_SWITCH_TEST, 0, 0, 0);
}

void process_info()
{
    svc_call(SVC_PROCESS_INFO, 0, 0, 0);
}
