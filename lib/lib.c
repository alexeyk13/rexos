/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "../userspace/lib/lib.h"

#include "pool.h"
#include "printf.h"
#include "rand.h"
#include "lib_time.h"
#if (KERNEL_LIB_ARRAY)
#include "lib_array.h"
#endif //KERNEL_LIB_ARRAY

const LIB __LIB = {
    //pool.h
    pool_init,
    pool_malloc,
    pool_realloc,
    pool_free,

#if (KERNEL_PROFILING)
    pool_check,
    pool_stat,
#endif //KERNEL_PROFILING
    //printf.h
    format,
    pformat,
    sformat,
    __atou,
    __utoa,
    __puts,
    __putc,
    __getc,
    __gets,
    //rand.h
    __srand,
    __rand,
    //lib_time.h
    (const void* const)&__LIB_TIME,
#if (KERNEL_LIB_ARRAY)
    (const void* const)&__LIB_ARRAY
#else
    (const void* const)NULL
#endif //KERNEL_LIB_ARRAY
};


