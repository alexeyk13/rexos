/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "kernel_config.h"
#include "../kernel/klib.h"
#include "lib_std.h"
#include "printf.h"
#include "pool.h"
#if (KERNEL_LIB_SOFT_RAND)
#include "rand.h"
#endif //KERNEL_LIB_SOFT_RAND

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
    (bool (*)(POOL*, void*))lib_stub,
    (void (*)(POOL*, POOL_STAT*, void*))lib_stub,
#endif //KERNEL_PROFILING
#if (KERNEL_LIB_SOFT_RAND)
    __srand,
    __rand
#else
    (unsigned int (*)())lib_stub,
    (unsigned int (*)(unsigned int*))lib_stub
#endif //KERNEL_LIB_SOFT_RAND
};
