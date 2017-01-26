/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "lib_lib.h"
#include "../kernel/dbg.h"
#include "kernel_config.h"
#include "lib_std.h"
#include "lib_stdio.h"
#include "lib_systime.h"
#include "lib_array.h"
#include "lib_so.h"

void lib_stub ()
{
#if (KERNEL_DEBUG)
    printk("Warning: lib stub called\n");
#endif //KERNEL_DEBUG
}

const void *const __LIB[] = {
    //lib_stdio.h
    (const void *const)&__LIB_STD,
    //lib_stdio.h
    (const void *const )&__LIB_STDIO,
    //lib_systime.h
    (const void *const)&__LIB_SYSTIME,
    //lib_array.h
    (const void *const)&__LIB_ARRAY,
    //lib_so.h
    (const void *const)&__LIB_SO
};


