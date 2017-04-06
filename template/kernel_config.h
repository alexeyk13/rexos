/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2016, Alexey Kramarenko
    All rights reserved.
*/

#ifndef KERNEL_CONFIG_H
#define KERNEL_CONFIG_H

//----------------------------------- kernel ------------------------------------------------------------------
//enable kernel info. Disabling this you can save some flash size, but kernel will be much less verbose, especially on critical errors. Generally doesn't affect on perfomance
#define KERNEL_DEBUG                                1
//marks objects with magic in headers. Decrease perfomance on few tacts, but very useful for debug if you don't have MPU enabled
#define KERNEL_MARKS                                0
//check range of dynamic objects in pools
#define KERNEL_RANGE_CHECKING                       0
//check kernel handles. Require few tacts, but making kernel calls much safer
#define KERNEL_HANDLE_CHECKING                      1
//check user adresses. Require few tacts, but making kernel calls much safer
#define KERNEL_ADDRESS_CHECKING                     0
//some kernel statistics (stack, mem, etc). Decrease perfomance in any object creation.
#define KERNEL_PROFILING                            1
//Enabling this you will get stats on each thread uptime, but decreasing context switching up to 2 times
#define KERNEL_PROCESS_STAT                         1
//Kernel halt on fatal error, disable power save mode
//Don't forget to turn off in production.
#define KERNEL_DEVELOPER_MODE                       1
//enable this only if you have problems with system timer. May decrease perfomance
#define KERNEL_TIMER_DEBUG                          0
//size of IPC queue per process
#define KERNEL_IPC_COUNT                            7
//enable this only if you have problems with IPC oferflow.
#define KERNEL_IPC_DEBUG                            1
//maximum number of global handles. Must be at least 1
#define KERNEL_OBJECTS_COUNT                        5
//enable multi-process safe dynamic heap. Required for most of high-level stacks (BLE, TCP/IP, etc)
//disable to save few bytes
#define KERNEL_HEAP                                 1

#endif // KERNEL_CONFIG_H
