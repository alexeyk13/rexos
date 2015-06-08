/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "lib_lib.h"
#include "../kernel/dbg.h"
#include "kernel_config.h"
#include "lib_std.h"
#include "lib_stdio.h"
#include "lib_time.h"
#include "lib_heap.h"
#if (KERNEL_LIB_GPIO)
#include "lib_gpio.h"
#endif //KERNEL_LIB_GPIO

void lib_stub ()
{
    printk("Warning: lib stub called\n\r");
}

const void *const __LIB[] = {
    //lib_stdio.h
    (const void *const)&__LIB_STD,
    //lib_stdio.h
    (const void *const )&__LIB_STDIO,
    //lib_time.h
    (const void *const)&__LIB_TIME,
    //lib_heap.h
    (const void *const )&__LIB_HEAP,
#if (KERNEL_LIB_GPIO)
    (const void *const)&__LIB_GPIO
#else
    (const void *const)NULL
#endif //KERNEL_LIB_GPIO
};


