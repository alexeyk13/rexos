/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "svc.h"
#include "../userspace/core/core.h"
#include "../userspace/error.h"
#include "../kernel/kernel.h"

#include "dbg.h"
#include "svc_process.h"
#include "svc_mutex.h"
#include "svc_event.h"
#include "svc_sem.h"
#include "svc_timer.h"

void svc(unsigned int num, unsigned int param1, unsigned int param2, unsigned int param3)
{
    CRITICAL_ENTER;
    clear_error();
    switch (num)
    {
    //process related
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
    //mutex related
    case SVC_MUTEX_CREATE:
        svc_mutex_create((MUTEX**)param1);
        break;
    case SVC_MUTEX_LOCK:
        svc_mutex_lock((MUTEX*)param1, (TIME*)param2);
        break;
    case SVC_MUTEX_UNLOCK:
        svc_mutex_unlock((MUTEX*)param1);
        break;
    case SVC_MUTEX_DESTROY:
        svc_mutex_destroy((MUTEX*)param1);
        break;
    //event related
    case SVC_EVENT_CREATE:
        svc_event_create((EVENT**)param1);
        break;
    case SVC_EVENT_PULSE:
        svc_event_pulse((EVENT*)param1);
        break;
    case SVC_EVENT_SET:
        svc_event_set((EVENT*)param1);
        break;
    case SVC_EVENT_IS_SET:
        svc_event_is_set((EVENT*)param1, (bool*)param2);
        break;
    case SVC_EVENT_CLEAR:
        svc_event_clear((EVENT*)param1);
        break;
    case SVC_EVENT_WAIT:
        svc_event_wait((EVENT*)param1, (TIME*)param2);
        break;
    case SVC_EVENT_DESTROY:
        svc_event_destroy((EVENT*)param1);
        break;
    //semaphore related
    case SVC_SEM_CREATE:
        svc_sem_create((SEM**)param1);
        break;
    case SVC_SEM_SIGNAL:
        svc_sem_signal((SEM*)param1);
        break;
    case SVC_SEM_WAIT:
        svc_sem_wait((SEM*)param1, (TIME*)param2);
        break;
    case SVC_SEM_DESTROY:
        svc_sem_destroy((SEM*)param1);
        break;
    //system timer related
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
    //other - dbg, stdout/in
    case SVC_SETUP_STDOUT:
        __KERNEL->stdout_global = (STDOUT)param1;
        __KERNEL->stdout_global_param = (void*)param2;
        break;
    case SVC_SETUP_STDIN:
        __KERNEL->stdin_global = (STDIN)param1;
        __KERNEL->stdin_global_param = (void*)param2;
        break;
    case SVC_SETUP_DBG:
        if (__KERNEL->dbg_locked)
            error(ERROR_INVALID_SVC);
        else
        {
            __KERNEL->stdout = (STDOUT)param1;
            __KERNEL->stdout_param = (void*)param2;
            __KERNEL->dbg_locked = true;
        }
        break;
    default:
        error(ERROR_INVALID_SVC);
    }
    CRITICAL_LEAVE;
}

void svc_irq(unsigned int num, unsigned int param1, unsigned int param2, unsigned int param3)
{
    switch (num)
    {

    }

    svc(num, param1, param2, param3);
}
