/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "kernel.h"
#include "kernel_config.h"
#include "dbg.h"
#include "kirq.h"
#include "kipc.h"
#include "kstream.h"
#include "kprocess.h"
#include "kdirect.h"
#include "kblock.h"
#include "kobject.h"

#include "../userspace/error.h"
#include "../lib/lib_lib.h"
#include <string.h>

const char* const __KERNEL_NAME=                                                      "RExOS 0.2.8";

void stdout_stub(const char *const buf, unsigned int size, void* param)
{
    //what can we debug in debug stub? :)
    kprocess_error_current(ERROR_STUB_CALLED);
}

void panic()
{
#if (KERNEL_INFO)
    printk("Kernel panic\n\r");
    dump(SRAM_BASE, 0x200);
#endif
#if (KERNEL_DEVELOPER_MODE)
    HALT();
#else
    fatal();
#endif //KERNEL_DEVELOPER_MODE
}

void svc(unsigned int num, unsigned int param1, unsigned int param2, unsigned int param3)
{
    switch (num)
    {
    //process related
    case SVC_PROCESS_CREATE:
        kprocess_create((REX*)param1, (PROCESS**)param2);
        break;
    case SVC_PROCESS_GET_CURRENT:
        kprocess_get_current_svc((PROCESS**)param1);
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
        kprocess_sleep(kprocess_get_current(), (TIME*)param1, PROCESS_SYNC_TIMER_ONLY, NULL);
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
        kirq_register((int)param1, (IRQ)param2, (void*)param3);
        break;
    case SVC_IRQ_UNREGISTER:
        kirq_unregister((int)param1);
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
        ktimer_setup((CB_SVC_TIMER*)param1, (void*)param2);
        break;
#if (KERNEL_SOFT_TIMERS)
    case SVC_TIMER_CREATE:
        ktimer_create((SOFT_TIMER**)param1, param2, (HAL)param3);
        break;
    case SVC_TIMER_START:
        ktimer_start((SOFT_TIMER*)param1, (TIME*)param2, param3);
        break;
    case SVC_TIMER_STOP:
        ktimer_stop((SOFT_TIMER*)param1);
        break;
    case SVC_TIMER_DESTROY:
        ktimer_destroy((SOFT_TIMER*)param1);
        break;
#endif //KERNEL_SOFT_TIMERS
    //ipc related
    case SVC_IPC_POST:
        kipc_post((IPC*)param1);
        break;
    case SVC_IPC_READ:
        kipc_read((IPC*)param1, (TIME*)param2, param3);
        break;
    case SVC_IPC_CALL:
        kipc_call((IPC*)param1, (TIME*)param2);
        break;
    //stream related
    case SVC_STREAM_CREATE:
        kstream_create((STREAM**)param1, param2);
        break;
    case SVC_STREAM_OPEN:
        kstream_open((STREAM*)param1, (STREAM_HANDLE**)param2);
        break;
    case SVC_STREAM_CLOSE:
        kstream_close((STREAM_HANDLE*)param1);
        break;
    case SVC_STREAM_GET_SIZE:
        kstream_get_size((STREAM*)param1, (unsigned int*)param2);
        break;
    case SVC_STREAM_GET_FREE:
        kstream_get_free((STREAM*)param1, (unsigned int*)param2);
        break;
    case SVC_STREAM_LISTEN:
        kstream_listen((STREAM*)param1, param2, (HAL)param3);
        break;
    case SVC_STREAM_STOP_LISTEN:
        kstream_stop_listen((STREAM*)param1);
        break;
    case SVC_STREAM_WRITE:
        kstream_write((STREAM_HANDLE*)param1, (char*)param2, param3);
        break;
    case SVC_STREAM_READ:
        kstream_read((STREAM_HANDLE*)param1, (char*)param2, param3);
        break;
    case SVC_STREAM_FLUSH:
        kstream_flush((STREAM*)param1);
        break;
    case SVC_STREAM_DESTROY:
        kstream_destroy((STREAM*)param1);
        break;
    //direct io
    case SVC_DIRECT_READ:
        kdirect_read((PROCESS*)param1, (void*)param2, param3);
        break;
    case SVC_DIRECT_WRITE:
        kdirect_write((PROCESS*)param1, (void*)param2, param3);
        break;
    //block
    case SVC_BLOCK_CREATE:
        kblock_create((BLOCK**)param1, param2);
        break;
    case SVC_BLOCK_OPEN:
        kblock_open((BLOCK*)param1, (void**)param2);
        break;
    case SVC_BLOCK_CLOSE:
        kblock_close((BLOCK*)param1);
        break;
    case SVC_BLOCK_GET_SIZE:
        kblock_get_size((BLOCK*)param1, (unsigned int*)param2);
        break;
    case SVC_BLOCK_SEND:
        kblock_send((BLOCK*)param1, (PROCESS*)param2);
        break;
    case SVC_BLOCK_SEND_IPC:
        kblock_send_ipc((BLOCK*)param1, (PROCESS*)param2, (IPC*)param3);
        break;
    case SVC_BLOCK_RETURN:
        kblock_return((BLOCK*)param1);
        break;
    case SVC_BLOCK_DESTROY:
        kblock_destroy((BLOCK*)param1);
        break;
    case SVC_OBJECT_SET:
        kobject_set(param1, (HANDLE)param2);
        break;
    case SVC_OBJECT_GET:
        kobject_get(param1, (HANDLE*)param2);
        break;
    //other - dbg, stdout/in
    case SVC_SETUP_DBG:
        if (__KERNEL->stdout != stdout_stub)
            kprocess_error_current(ERROR_INVALID_SVC);
        else
        {
            __KERNEL->stdout = (STDOUT)param1;
            __KERNEL->stdout_param = (void*)param2;
#if KERNEL_INFO
            printk("%s\n\r", __KERNEL_NAME);
#endif
        }
        break;
    case SVC_PRINTD:
        __KERNEL->stdout((const char*)param1, param2, __KERNEL->stdout_param);
        break;
#if (KERNEL_PROFILING)
    case SVC_TEST:
        //do nothing
        break;
#endif //KERNEL_PROFILING
    default:
        kprocess_error_current(ERROR_INVALID_SVC);
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

    //initialize paged area
    pool_init(&__KERNEL->paged, (void*)(SRAM_BASE + KERNEL_GLOBAL_SIZE + sizeof(KERNEL)));

    //initilize timer
    ktimer_init();

    //initialize kernel objects
    kobject_init();

    //initialize thread subsystem, create application task
    kprocess_init(&__APP);
}
