/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef KERNEL_CONFIG_H
#define KERNEL_CONFIG_H

//----------------------------------- kernel ------------------------------------------------------------------
//enable kernel info. Disabling this you can save some flash size, but kernel will be much less verbose, especially on critical errors. Generally doesn't affect on perfomance
#define KERNEL_INFO                                 1
//marks objects with magic in headers. Decrease perfomance on few tacts, but very useful for debug if you don't have MPU enabled
#define KERNEL_MARKS                                1
//check range of dynamic objects in pools
#define KERNEL_RANGE_CHECKING                       1
//check kernel handles. Require few tacts, but making kernel calls much safer
#define KERNEL_HANDLE_CHECKING                      1
//check user adresses. Require few tacts, but making kernel calls much safer
#define KERNEL_ADDRESS_CHECKING                     1
//some kernel statistics (stack, mem, etc). Decrease perfomance in any object creation.
#define KERNEL_PROFILING                            1
//enable kernel assertions. If assertions happens, system will be halted with FILE and LINE, instead of returning error.
//Doesn't affect on perfomance, but recommended only for debug
#define KERNEL_ASSERTIONS                           1
//halt system instead of reset. Only for development. Turn off in production.
#define KERNEL_HALT_ON_FATAL_ERROR                  1
//enable this only if you have problems with system timer. May decrease perfomance
#define KERNEL_TIMER_DEBUG                          0
//Enabling this you will get stats on each thread uptime, but decreasing context switching up to 2 times
#define KERNEL_PROCESS_STAT                         0
//Enable this only if you have problems with IPC oferflow.
#define KERNEL_IPC_DEBUG                            0
//maximum number of blocks, that can be opened at same time. Generally, it's MPU blocks count - 2
#define KERNEL_BLOCKS_COUNT                         6
//mutex, event, semaphore are now deprecated. Use stream, direct, block instead
#define KERNEL_MES                                  0

//size of GLOBAL consts. For MPU must be pow of 2, starting from 32
//you must have good reason to changing this
#define KERNEL_GLOBAL_SIZE                          32
//total size of kernel RAM, including GLOBAL. For MPU Must be pow of 2, starting from 32
#define KERNEL_SIZE                                 0x2000

//save stdio, stdlib and time are required libs, all rest is optional
#define LIB_ARRAY                                   1


#endif // KERNEL_CONFIG_H
