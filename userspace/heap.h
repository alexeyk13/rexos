/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef HEAP_H
#define HEAP_H

#include "types.h"
#include "svc.h"
#include "cc_macro.h"

typedef struct {
    int error;
    POOL pool;
    //stdout/stdin handle. System specific
    HANDLE stdout, stdin;
    const char* name;
} PROCESS;

#define __HEAP                                              ((PROCESS*)(((GLOBAL*)(SRAM_BASE))->heap))

__STATIC_INLINE int get_last_error()
{
    return __HEAP->error;
}

__STATIC_INLINE void error(int error)
{
    __HEAP->error = error;
}

#endif // HEAP_H
