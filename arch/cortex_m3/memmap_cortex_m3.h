/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef MEMMAP_CORTEX_M3_H
#define MEMMAP_CORTEX_M3_H

//logical memory layout

/*
    .data
    .bss
    [system pool]

    /\
    [global pool]
    \/

    [thread stacks]
    SVC stack
 */

//external memory layout
//#define SVC_STACK_TOP             (SRAM_BASE + INT_RAM_SIZE - SVC_STACK_SIZE * 4)
//#define SVC_STACK_END             (SVC_STACK_TOP + SVC_STACK_SIZE * 4)

//#define SYSTEM_POOL_BASE          (KERNEL_BASE + sizeof(KERNEL))

//#define DATA_POOL_BASE            (SRAM_BASE + SYSTEM_POOL_SIZE)
//#define DATA_POOL_SIZE            (SVC_STACK_TOP - DATA_POOL_BASE)

#endif // MEMMAP_CORTEX_M3_H
