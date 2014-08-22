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
#if (LIB_ARRAY)
#include "lib_array.h"
#endif //LIB_ARRAY

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
    __mktime,
    __gmtime,
    __time_compare,
    __time_add,
    __time_sub,
    __us_to_time,
    __ms_to_time,
    __time_to_us,
    __time_to_ms,
    __time_elapsed,
    __time_elapsed_ms,
    __time_elapsed_us
#if (LIB_ARRAY)
    ,
    __array_create,
    __array_destroy,
    __array_add,
    __array_remove,
    __array_squeeze
#endif //LIB_ARRAY
};


