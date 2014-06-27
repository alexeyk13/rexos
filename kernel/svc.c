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
#include "thread_kernel.h"
#include "mutex_kernel.h"
#include "event_kernel.h"
#include "sem_kernel.h"
#include "mem_kernel.h"
#include "svc_timer.h"

void svc(unsigned int num, unsigned int param1, unsigned int param2, unsigned int param3)
{
    clear_error();
    switch (num & 0x0000ff00)
    {
    case SVC_THREAD:
        svc_thread_handler(num, param1, param2);
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
        svc_timer_handler(num, param1, param2);
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
}

void svc_irq(unsigned int num, unsigned int param1, unsigned int param2, unsigned int param3)
{
    ///TODO: CONTEXT_PUSH(SVC); really need this shit?
    svc(num, param1, param2, param3);
    ///TODO: CONTEXT_POP();
}
