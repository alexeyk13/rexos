/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "svc.h"
#include "../userspace/core/core.h"
#include "../userspace/core/sys_calls.h"
#include "../userspace/error.h"
#include "../kernel/kernel.h"

#include "dbg.h"
#include "svc_process.h"
#include "mutex_kernel.h"
#include "event_kernel.h"
#include "sem_kernel.h"
#include "mem_kernel.h"
#include "svc_timer.h"

void svc(unsigned int num, unsigned int param1, unsigned int param2, unsigned int param3)
{
    CRITICAL_ENTER;
    clear_error();
    switch (num & 0x0000ff00)
    {
    case SVC_PROCESS:
        switch (num)
        {
        case SVC_PROCESS_CREATE:
            svc_process_create((REX*)param1, (PROCESS**)param2);
            break;
        case SVC_PROCESS_GET_FLAGS:
            svc_process_get_flags((PROCESS*)param1, (unsigned int*)param2);
            break;
        case SVC_PROCESS_SET_FLAGS:
            svc_process_set_flags((PROCESS*)param1, (unsigned int)param2);
            break;
        case SVC_PROCESS_GET_PRIORITY:
            svc_process_get_priority((PROCESS*)param1, (unsigned int*)param2);
            break;
        case SVC_PROCESS_SET_PRIORITY:
            svc_process_set_priority((PROCESS*)param1, (unsigned int)param2);
            break;
        case SVC_PROCESS_DESTROY:
            svc_process_destroy((PROCESS*)param1);
            break;
        case SVC_PROCESS_SLEEP:
            svc_process_sleep((TIME*)param1, PROCESS_SYNC_TIMER_ONLY, NULL);
            break;
    #if (KERNEL_PROFILING)
        case SVC_PROCESS_SWITCH_TEST:
            svc_process_switch_test();
            break;
        case SVC_PROCESS_INFO:
            svc_process_info();
            break;
    #endif //KERNEL_PROFILING
        default:
            error(ERROR_INVALID_SVC);
        }
        break;
    case SVC_MUTEX:
        svc_mutex_handler(num, param1, param2);
        break;
    case SVC_EVENT:
        svc_event_handler(num, param1, param2);
        break;
    case SVC_SEM:
        svc_sem_handler(num, param1, param2);
        break;
    case SVC_MEM:
        svc_mem_handler(num);
        break;
    case (SVC_TIMER):
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
        case SVC_TIMER_SETUP:
            svc_timer_setup((CB_SVC_TIMER*)param1);
            break;
        default:
            error(ERROR_INVALID_SVC);
        }
        break;
    case (SVC_DBG):
        switch (num)
        {
        case  SVC_DBG_WRITE:
            if (__KERNEL->dbg_console)
                console_write(__KERNEL->dbg_console, (char*)param1, (int)param2);
            break;
        case SVC_DBG_PUSH:
            if (__KERNEL->dbg_console)
                console_push(__KERNEL->dbg_console);
            break;
        default:
            error(ERROR_INVALID_SVC);
        }
        break;
    default:
        error(ERROR_INVALID_SVC);
    }
    CRITICAL_LEAVE;
}

void svc_irq(unsigned int num, unsigned int param1, unsigned int param2, unsigned int param3)
{
    ///TODO: CONTEXT_PUSH(SVC); really need this shit?
    svc(num, param1, param2, param3);
    ///TODO: CONTEXT_POP();
}
