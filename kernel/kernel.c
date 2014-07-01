/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "kernel.h"
#include "kernel_config.h"
#include "dbg.h"
#include "kmutex.h"
#include "kevent.h"
#include "ksem.h"
#include "kprocess.h"
#include "../userspace/error.h"
#include "../userspace/lib/lib.h"
#include <string.h>

void stdout_stub(const char *const buf, unsigned int size, void* param)
{
    //what can we say in stdout stub? :)
    error(ERROR_STUB_CALLED);
}

void stdin_stub(char *buf, unsigned int size, void* param)
{
    error(ERROR_STUB_CALLED);
}

void default_irq_handler(int vector)
{
#if (KERNEL_DEBUG)
    printk("Warning: irq vector %d without handler\n\r", vector);
#endif
}

void panic()
{
#if (KERNEL_DEBUG)
    printk("Kernel panic\n\r");
    dump(SRAM_BASE, 0x200);
#endif
#if (KERNEL_HALT_ON_FATAL_ERROR)
    HALT();
#else
    reset();
#endif //KERNEL_HALT_ON_FATAL_ERROR
}

void svc(unsigned int num, unsigned int param1, unsigned int param2, unsigned int param3)
{
    CRITICAL_ENTER;
    clear_error();
    switch (num)
    {
    //process related
    case SVC_PROCESS_CREATE:
        kprocess_create((REX*)param1, (PROCESS**)param2);
        break;
    case SVC_PROCESS_GET_FLAGS:
        kprocess_get_flags((PROCESS*)param1, (unsigned int*)param2);
        break;
    case SVC_PROCESS_SET_FLAGS:
        kprocess_set_flags((PROCESS*)param1, (unsigned int)param2);
        break;
    case SVC_PROCESS_GET_PRIORITY:
        kprocess_get_priority((PROCESS*)param1, (unsigned int*)param2);
        break;
    case SVC_PROCESS_SET_PRIORITY:
        kprocess_set_priority((PROCESS*)param1, (unsigned int)param2);
        break;
    case SVC_PROCESS_DESTROY:
        kprocess_destroy((PROCESS*)param1);
        break;
    case SVC_PROCESS_SLEEP:
        kprocess_sleep((TIME*)param1, PROCESS_SYNC_TIMER_ONLY, NULL);
        break;
#if (KERNEL_PROFILING)
    case SVC_PROCESS_SWITCH_TEST:
        kprocess_switch_test();
        break;
    case SVC_PROCESS_INFO:
        kprocess_info();
        break;
#endif //KERNEL_PROFILING
    //mutex related
    case SVC_MUTEX_CREATE:
        kmutex_create((MUTEX**)param1);
        break;
    case SVC_MUTEX_LOCK:
        kmutex_lock((MUTEX*)param1, (TIME*)param2);
        break;
    case SVC_MUTEX_UNLOCK:
        kmutex_unlock((MUTEX*)param1);
        break;
    case SVC_MUTEX_DESTROY:
        kmutex_destroy((MUTEX*)param1);
        break;
    //event related
    case SVC_EVENT_CREATE:
        kevent_create((EVENT**)param1);
        break;
    case SVC_EVENT_PULSE:
        kevent_pulse((EVENT*)param1);
        break;
    case SVC_EVENT_SET:
        kevent_set((EVENT*)param1);
        break;
    case SVC_EVENT_IS_SET:
        kevent_is_set((EVENT*)param1, (bool*)param2);
        break;
    case SVC_EVENT_CLEAR:
        kevent_clear((EVENT*)param1);
        break;
    case SVC_EVENT_WAIT:
        kevent_wait((EVENT*)param1, (TIME*)param2);
        break;
    case SVC_EVENT_DESTROY:
        kevent_destroy((EVENT*)param1);
        break;
    //semaphore related
    case SVC_SEM_CREATE:
        ksem_create((SEM**)param1);
        break;
    case SVC_SEM_SIGNAL:
        ksem_signal((SEM*)param1);
        break;
    case SVC_SEM_WAIT:
        ksem_wait((SEM*)param1, (TIME*)param2);
        break;
    case SVC_SEM_DESTROY:
        ksem_destroy((SEM*)param1);
        break;
    //system timer related
    case SVC_TIMER_HPET_TIMEOUT:
        ktimer_hpet_timeout();
        break;
    case SVC_TIMER_SECOND_PULSE:
        ktimer_second_pulse();
        break;
    case SVC_TIMER_GET_UPTIME:
        ktimer_get_uptime((TIME*)param1);
        break;
    case SVC_TIMER_SETUP:
        ktimer_setup((CB_SVC_TIMER*)param1);
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

void startup()
{
    //setup __GLOBAL
    __GLOBAL->svc_irq = svc;
    __GLOBAL->lib = &__LIB;

    //setup __KERNEL
    memset(__KERNEL, 0, sizeof(KERNEL));
    __KERNEL->stdout = __KERNEL->stdout_global = stdout_stub;
    __KERNEL->stdin_global = stdin_stub;
    strcpy(__KERNEL_NAME, "RExOS 0.1");
    __KERNEL->struct_size = sizeof(KERNEL) + strlen(__KERNEL_NAME) + 1;

    //initialize system memory pool
    pool_init(&__KERNEL->pool, (void*)(KERNEL_BASE + __KERNEL->struct_size));

    //initialize paged area
    pool_init(&__KERNEL->paged, (void*)(SRAM_BASE + KERNEL_SIZE));

#ifndef NVIC_PRESENT
    irq_init();
#endif //NVIC_PRESENT

    //initilize timer
    ktimer_init();

    //initialize thread subsystem, create idle task
    kprocess_init(&__INIT);

}
