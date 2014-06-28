/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "svc.h"
#include <string.h>
#include "svc_process.h"
#include "kernel_config.h"
#include "kernel.h"
#include "../lib/pool.h"
#include "svc_timer.h"

extern const REX INIT;

void startup()
{
    //setup __GLOBAL
    __GLOBAL->svc_irq = svc_irq;

    //setup __KERNEL
    memset(__KERNEL, 0, sizeof(KERNEL));
    strcpy(__KERNEL_NAME, "RExOS");
    __KERNEL->struct_size = sizeof(KERNEL) + strlen(__KERNEL_NAME) + 1;

    //initialize system memory pool
    pool_init(&__KERNEL->pool, (void*)(KERNEL_BASE + __KERNEL->struct_size));

    //initialize paged area
    pool_init(&__KERNEL->paged, (void*)(SRAM_BASE + KERNEL_SIZE));

#ifndef NVIC_PRESENT
    irq_init();
#endif //NVIC_PRESENT

    //initilize timer
    svc_timer_init();

    //initialize thread subsystem, create idle task
    svc_process_init(&INIT);

}
