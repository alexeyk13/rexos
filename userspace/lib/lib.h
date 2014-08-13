/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef LIB_H
#define LIB_H

#include "types.h"

typedef struct {
    //pool.h
    void (*pool_init)(POOL*, void*);
    void* (*pool_malloc)(POOL*, size_t);
    void* (*pool_realloc)(POOL*, void*, size_t);
    void (*pool_free)(POOL*, void*);

    bool (*pool_check)(POOL*, void*);
    void (*pool_stat)(POOL*, POOL_STAT*, void*);
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
    time_t (*mktime)(struct tm*);
    struct tm* (*gmtime)(time_t, struct tm*);
    int (*time_compare)(TIME*, TIME*);
    void (*time_add)(TIME*, TIME*, TIME*);
    void (*time_sub)(TIME*, TIME*, TIME*);
    void (*us_to_time)(int, TIME*);
    void (*ms_to_time)(int, TIME*);
    int (*time_to_us)(TIME*);
    int (*time_to_ms)(TIME*);
    TIME* (*time_elapsed)(TIME*, TIME*);
    unsigned int (*time_elapsed_ms)(TIME*);
    unsigned int (*time_elapsed_us)(TIME*);
    //array.h
    ARRAY* (*array_create)(ARRAY** ar, unsigned int reserved);
    void (*array_destroy)(ARRAY** ar);
    ARRAY* (*array_add)(ARRAY** ar, unsigned int size);
    ARRAY* (*array_remove)(ARRAY** ar, unsigned int index);
    ARRAY* (*array_squeeze)(ARRAY** ar);
} LIB;

#endif // LIB_H
