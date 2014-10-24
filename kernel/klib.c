/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "klib.h"
#include "dbg.h"
#include "kernel_config.h"
#include "../lib/lib_std.h"
#include "../lib/lib_stdio.h"
#include "../lib/lib_time.h"
#include "../lib/lib_array.h"

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
#if (KERNEL_LIB_ARRAY)
    (const void *const)&__LIB_ARRAY
#else
    (const void *const)NULL
#endif //KERNEL_LIB_ARRAY
};


