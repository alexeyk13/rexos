/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "kernel_config.h"
#include "lib_lib.h"
#include "lib_std.h"
#include "printf.h"
#include "pool.h"

const LIB_STD __LIB_STD = {
    //printf.h
    __atou,
    __utoa,
    //pool.h
    pool_init,
    pool_malloc,
    pool_slot_size,
    pool_realloc,
    pool_free,
#if (KERNEL_PROFILING)
    pool_check,
    pool_stat
#else
    (bool (*)(POOL*, void*))lib_stub,
    (void (*)(POOL*, POOL_STAT*, void*))lib_stub
#endif //KERNEL_PROFILING
};
