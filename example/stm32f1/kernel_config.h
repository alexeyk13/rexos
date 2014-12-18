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
//Kernel assertions, halt on fatal error, disable power save mode
//Don't forget to turn off in production.
#define KERNEL_DEVELOPER_MODE                       1
//enable this only if you have problems with system timer. May decrease perfomance
#define KERNEL_TIMER_DEBUG                          0
//Enabling this you will get stats on each thread uptime, but decreasing context switching up to 2 times
#define KERNEL_PROCESS_STAT                         1
//Enable this only if you have problems with IPC oferflow.
#define KERNEL_IPC_DEBUG                            0
//maximum number of global handles. Must be at least 1
#define KERNEL_OBJECTS_COUNT                        4
//mutex, event, semaphore are now deprecated. Use stream, direct, block instead
#define KERNEL_MES                                  0

//save stdio, stdlib and time are required libs, all rest is optional
#define KERNEL_LIB_ARRAY                            1
#define KERNEL_LIB_SOFT_RAND                        0
#define KERNEL_LIB_USB                              1
#define KERNEL_LIB_GPIO                             1

#endif // KERNEL_CONFIG_H
