/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: Alexey E. Kramarenko (alexeyk13@yandex.ru)
*/

#include "kernel.h"
#include "kernel_config.h"
#include "dbg.h"
#include "kirq.h"
#include "kipc.h"
#include "kstream.h"
#include "kprocess.h"
#include "kio.h"
#include "kobject.h"
#include "ksystime.h"
#include "kstdlib.h"

#include "../userspace/error.h"
#include "../userspace/core/core.h"
#include "../lib/lib_lib.h"
#include <string.h>

const char* const __KERNEL_NAME=                                                      "RExOS 0.6.3";

void stdout_stub(const char *const buf, unsigned int size, void* param)
{
    //what can we debug in debug stub? :)
}

void panic()
{
#if (KERNEL_DEBUG)
    printk("Kernel panic\n");
#if (KERNEL_SVC_DEBUG)
    printk("Last SVC: %08X, (%08X, %08X, %08X)\n", __KERNEL->num, __KERNEL->param1, __KERNEL->param2, __KERNEL->param3);
    printk("caller: %s\n", kprocess_name(__KERNEL->call_process));
#endif //KERNEL_SVC_DEBUG
    dump(SRAM_BASE, 0x200);
#endif
#if (KERNEL_DEVELOPER_MODE)
    HALT();
#else
    fatal();
#endif //KERNEL_DEVELOPER_MODE
}

void kernel_setup_dbg(STDOUT stdout, void* param)
{
    if (__KERNEL->stdout != stdout_stub)
        error(ERROR_INVALID_SVC);
    else
    {
        __KERNEL->stdout = stdout;
        __KERNEL->stdout_param = param;
#if KERNEL_DEBUG
        printk("%s\n", __KERNEL_NAME);
#endif
    }
}

void svc(unsigned int num, unsigned int param1, unsigned int param2, unsigned int param3)
{
    HANDLE process = kprocess_get_current();
#if (KERNEL_SVC_DEBUG)
    __KERNEL->num = num;
    __KERNEL->param1 = param1;
    __KERNEL->param2 = param2;
    __KERNEL->param3 = param3;
    __KERNEL->call_process = process;
#endif //KERNEL_SVC_DEBUG
    switch (num)
    {
    //process related
    case SVC_PROCESS_CREATE:
        CHECK_ADDRESS(process, (HANDLE*)param2, sizeof(HANDLE));
        *((HANDLE*)param2) = kprocess_create((REX*)param1);
        break;
    case SVC_PROCESS_GET_CURRENT:
        CHECK_ADDRESS(process, (HANDLE*)param1, sizeof(HANDLE));
        *((HANDLE*)param1) = process;
        break;
    case SVC_PROCESS_GET_FLAGS:
        CHECK_ADDRESS(process, (unsigned int*)param2, sizeof(unsigned int));
        *((unsigned int*)param2) = kprocess_get_flags(param1);
        break;
    case SVC_PROCESS_SET_FLAGS:
        kprocess_set_flags(param1, param2);
        break;
    case SVC_PROCESS_GET_PRIORITY:
        CHECK_ADDRESS(process, (unsigned int*)param2, sizeof(unsigned int));
        *((unsigned int*)param2) = kprocess_get_priority((HANDLE)param1);
        break;
    case SVC_PROCESS_SET_PRIORITY:
        kprocess_set_priority((HANDLE)param1, (unsigned int)param2);
        break;
    case SVC_PROCESS_DESTROY:
        kprocess_destroy(param1);
        break;
    case SVC_PROCESS_SLEEP:
        CHECK_ADDRESS(process, (SYSTIME*)param1, sizeof(SYSTIME));
        kprocess_sleep(process, (SYSTIME*)param1, PROCESS_SYNC_TIMER_ONLY, INVALID_HANDLE);
        break;
#if (KERNEL_PROFILING)
    case SVC_PROCESS_SWITCH_TEST:
        kprocess_switch_test();
        break;
    case SVC_PROCESS_INFO:
        kprocess_info();
        break;
#endif //KERNEL_PROFILING
    //irq related
    case SVC_IRQ_REGISTER:
        kirq_register(process, (int)param1, (IRQ)param2, (void*)param3);
        break;
    case SVC_IRQ_UNREGISTER:
        kirq_unregister(process, (int)param1);
        break;
    //system timer related
#ifndef EXODRIVERS
    case SVC_SYSTIME_HPET_TIMEOUT:
        ksystime_hpet_timeout();
        break;
    case SVC_SYSTIME_SECOND_PULSE:
        ksystime_second_pulse();
        break;
    case SVC_SYSTIME_HPET_SETUP:
        ksystime_hpet_setup((CB_SVC_TIMER*)param1, (void*)param2);
        break;
#endif //EXODRIVERS
    case SVC_SYSTIME_GET_UPTIME:
        CHECK_ADDRESS(process, (SYSTIME*)param1, sizeof(SYSTIME));
        ksystime_get_uptime((SYSTIME*)param1);
        break;
    case SVC_SYSTIME_SOFT_TIMER_CREATE:
        CHECK_ADDRESS(process, (HANDLE*)param1, sizeof(HANDLE));
        *((HANDLE*)param1) = ksystime_soft_timer_create(process, (HANDLE)param2, (HAL)param3);
        break;
    case SVC_SYSTIME_SOFT_TIMER_START:
        CHECK_ADDRESS(process, (SYSTIME*)param2, sizeof(SYSTIME));
        ksystime_soft_timer_start((HANDLE)param1, (SYSTIME*)param2);
        break;
    case SVC_SYSTIME_SOFT_TIMER_STOP:
        ksystime_soft_timer_stop((HANDLE)param1);
        break;
    case SVC_SYSTIME_SOFT_TIMER_DESTROY:
        ksystime_soft_timer_destroy((HANDLE)param1);
        break;
    //ipc related
    case SVC_IPC_POST:
        CHECK_ADDRESS(process, (IPC*)param1, sizeof(IPC));
        CHECK_IO_ADDRESS(process, (IPC*)param1);
        kipc_post(process, (IPC*)param1);
        break;
    case SVC_IPC_WAIT:
        kipc_wait(process, param1, param2, param3);
        break;
    case SVC_IPC_CALL:
        CHECK_ADDRESS(process, (IPC*)param1, sizeof(IPC));
        CHECK_IO_ADDRESS(process, (IPC*)param1);
        kipc_call(process, (IPC*)param1);
        break;
    //stream related
    case SVC_STREAM_CREATE:
        CHECK_ADDRESS(process, (HANDLE*)param1, sizeof(HANDLE));
        *((HANDLE*)param1) = kstream_create(param2);
        break;
    case SVC_STREAM_OPEN:
        CHECK_ADDRESS(process, (HANDLE*)param2, sizeof(HANDLE));
        *((HANDLE*)param2) = kstream_open(process, param1);
        break;
    case SVC_STREAM_CLOSE:
        kstream_close(process, param1);
        break;
    case SVC_STREAM_GET_SIZE:
        CHECK_ADDRESS(process, (unsigned int*)param2, sizeof(unsigned int));
        *((unsigned int*)param2) = kstream_get_size(param1);
        break;
    case SVC_STREAM_GET_FREE:
        CHECK_ADDRESS(process, (unsigned int*)param2, sizeof(unsigned int));
        *((unsigned int*)param2) = kstream_get_free(param1);
        break;
    case SVC_STREAM_LISTEN:
        kstream_listen(process, param1, param2, (HAL)param3);
        break;
    case SVC_STREAM_STOP_LISTEN:
        kstream_stop_listen(process, param1);
        break;
    case SVC_STREAM_WRITE_NO_BLOCK:
        CHECK_ADDRESS(process, (unsigned int*)param3, sizeof(unsigned int));
        CHECK_ADDRESS(process, (char*)param2, *((unsigned int*)param3));
        *((unsigned int*)param3) = kstream_write_no_block(param1, (char*)param2, *((unsigned int*)param3));
        break;
    case SVC_STREAM_WRITE:
        CHECK_ADDRESS(process, (char*)param2, *((unsigned int*)param3));
        kstream_write(process, param1, (char*)param2, param3);
        break;
    case SVC_STREAM_READ:
        CHECK_ADDRESS(process, (char*)param2, *((unsigned int*)param3));
        kstream_read(process, param1, (char*)param2, param3);
        break;
    case SVC_STREAM_READ_NO_BLOCK:
        CHECK_ADDRESS(process, (unsigned int*)param3, sizeof(unsigned int));
        CHECK_ADDRESS(process, (char*)param2, *((unsigned int*)param3));
        *((unsigned int*)param3) = kstream_read_no_block(param1, (char*)param2, *((unsigned int*)param3));
        break;
    case SVC_STREAM_FLUSH:
        kstream_flush(param1);
        break;
    case SVC_STREAM_DESTROY:
        kstream_destroy(param1);
        break;
    case SVC_IO_CREATE:
        CHECK_ADDRESS(process, (IO**)param1, sizeof(IO*));
        *((IO**)param1) = kio_create(param2);
        break;
    case SVC_IO_DESTROY:
        kio_destroy((IO*)param1);
        break;
    case SVC_OBJECT_SET:
        kobject_set(process, param1, (HANDLE)param2);
        break;
    case SVC_OBJECT_GET:
        CHECK_ADDRESS(process, (HANDLE*)param2, sizeof(HANDLE));
        *((HANDLE*)param2) = kobject_get(param1);
        break;
    //other - dbg, stdout/in
    case SVC_ADD_POOL:
        kstdlib_add_pool(param1, param2);
        break;
#ifndef EXODRIVERS
    case SVC_SETUP_DBG:
        kernel_setup_dbg((STDOUT)param1, (void*)param2);
        break;
#endif //EXODRIVERS
#if (KERNEL_PROFILING)
    case SVC_TEST:
        //do nothing
        break;
#endif //KERNEL_PROFILING
    default:
        error(ERROR_INVALID_SVC);
    }
}

void startup()
{
    //setup __GLOBAL
    __GLOBAL->svc_irq = svc;
    __GLOBAL->lib = (const void**)&__LIB;

    //setup __KERNEL
    memset(__KERNEL, 0, sizeof(KERNEL));
    __KERNEL->stdout = stdout_stub;

    //initialize irq subsystem
    kirq_init();

    //initialize main pool
    kstdlib_init();

    //initilize system time
    ksystime_init();

    //initialize kernel objects
    kobject_init();

    //initialize exokernel drivers
#ifdef EXODRIVERS
    exodriver_init();
#endif //EXODRIVERS

    //initialize thread subsystem, create application task
    kprocess_init(&__APP);
}
