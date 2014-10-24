/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef LIB_H
#define LIB_H

#include "types.h"
#include "kernel_config.h"

typedef struct {
    //pool.h
    void (*pool_init)(POOL*, void*);
    void* (*pool_malloc)(POOL*, size_t);
    void* (*pool_realloc)(POOL*, void*, size_t);
    void (*pool_free)(POOL*, void*);

#if (KERNEL_PROFILING)
    bool (*pool_check)(POOL*, void*);
    void (*pool_stat)(POOL*, POOL_STAT*, void*);
#endif //KERNEL_PROFILING
    //printf.h
    void (*format)(const char *const, va_list, STDOUT, void*);
    void (*pformat)(const char *const, va_list);
    void (*sformat)(char*, const char *const, va_list);
    unsigned long (*atou)(const char *const, int);
    int (*utoa)(char*, unsigned long, int, bool);
    void (*puts)(const char*);
    void (*putc)(const char);
    char (*getc)();
    char* (*gets)(char*, int);

    //rand.h
    unsigned int (*srand)();
    unsigned int (*rand)(unsigned int* seed);
    //time.h
    const void* const p_lib_time;
    //array.h
    const void* const p_lib_array;
} LIB;

#endif // LIB_H
