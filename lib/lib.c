/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "../kernel/dbg.h"
#include "lib_std.h"
#include "lib_stdio.h"
#include "lib_time.h"
#if (KERNEL_LIB_ARRAY)
#include "lib_array.h"
#endif //KERNEL_LIB_ARRAY

void lib_stub ()
{
    printk("Warning: lib stub called\n\r");
}

const LIB __LIB = {
    //lib_stdio.h
    (const void* const)&__LIB_STD,
    //lib_stdio.h
    (const void* const)&__LIB_STDIO,
    //lib_time.h
    (const void* const)&__LIB_TIME,
#if (KERNEL_LIB_ARRAY)
    (const void* const)&__LIB_ARRAY
#else
    (const void* const)NULL
#endif //KERNEL_LIB_ARRAY
};


