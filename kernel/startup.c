/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "sys_call.h"
#include <string.h>
#include "sys_timer.h"
#include "rcc.h"
#include "thread_kernel.h"
#include "kernel_config.h"
#include "core/core_kernel.h"
#include "../lib/pool.h"

void startup()
{
    //setup __GLOBAL
    __GLOBAL->sys_handler_direct = sys_handler_direct;

    //setup __KERNEL
    strcpy(__KERNEL_NAME, "RExOS");
    __KERNEL->struct_size = sizeof(KERNEL) + strlen(__KERNEL_NAME) + 1;

    //initialize system memory pool
    pool_init(&__KERNEL->pool, (void*)(KERNEL_BASE + __KERNEL->struct_size));

    //initialize paged area
    pool_init(&__KERNEL->paged, (void*)(SRAM_BASE + KERNEL_SIZE));

#ifndef NVIC_PRESENT
    irq_init();
#endif //NVIC_PRESENT

    //initialize thread subsystem, create idle task
    thread_init();

    //move me userspace
    set_core_freq(0);

    //initialize system timers
    sys_timer_init();
}
