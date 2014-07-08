#ifndef KERNEL_CONFIG_H
#define KERNEL_CONFIG_H

//----------------------------------- kernel ------------------------------------------------------------------
//enable kernel debug. Disabling this you can save some flash size for debug info. Generally doesn't affect on perfomance
#define KERNEL_DEBUG                                1
//marks objects with magic in headers. Decrease perfomance on few tacts, but very useful for debug if you don't have MPU enabled
#define KERNEL_MARKS                                1
//check range of dynamic objects in pools
#define KERNEL_RANGE_CHECKING                       1
//some kernel statistics (stack, mem, etc). Decrease perfomance in any object creation.
#define KERNEL_PROFILING                            1
//halt system instead of reset. Only for development. Turn off in production.
#define KERNEL_HALT_ON_FATAL_ERROR                  1
//enable this only if you have problems with system timer. May decrease perfomance
#define KERNEL_TIMER_DEBUG                          0
//Enabling this you will get stats on each thread uptime, but decreasing context switching up to 2 times
#define KERNEL_PROCESS_STAT                         0
//Enable this only if you have problems with IPC oferflow.
#define KERNEL_IPC_DEBUG                            0

//size of GLOBAL consts. For MPU must be pow of 2, starting from 32
//you must have good reason to changing this
#define KERNEL_GLOBAL_SIZE                          32
//total size of kernel RAM, including GLOBAL. For MPU Must be pow of 2, starting from 32
#define KERNEL_SIZE                                 0x2000

#endif // KERNEL_CONFIG_H
