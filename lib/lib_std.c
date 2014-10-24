/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "kernel_config.h"
#include "lib_std.h"
#include "printf.h"
#include "pool.h"
//TODO if
#include "rand.h"

const LIB_STD __LIB_STD = {
    //printf.h
    __atou,
    __utoa,
    //pool.h
    pool_init,
    pool_malloc,
    pool_realloc,
    pool_free,

#if (KERNEL_PROFILING)
    pool_check,
    pool_stat,
#else
    lib_stub,
    lib_stub,
#endif //KERNEL_PROFILING
    //rand.h
    __srand,
    __rand,
};
