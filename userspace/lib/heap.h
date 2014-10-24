/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef HEAP_H
#define HEAP_H

#include "types.h"
#include "lib.h"
#include "../svc.h"

typedef struct {
    char* (*__process_name)(void*);
} LIB_HEAP;

typedef struct {
    int error;
    //header size including name
    int struct_size;
    POOL pool;
    //self handle
    HANDLE handle;
    //stdout/stdin handle. System specific
    HANDLE stdout, stdin;
    int direct_mode;
    HANDLE direct_process;
    void* direct_addr;
    unsigned int direct_size;
    //name is following
} HEAP;

#define __HEAP                                              ((HEAP*)(((GLOBAL*)(SRAM_BASE))->heap))

__STATIC_INLINE char* process_name()
{
    return ((const LIB_HEAP*)__GLOBAL->lib[LIB_ID_HEAP])->__process_name(__HEAP);
}

__STATIC_INLINE int get_last_error()
{
    return __HEAP->error;
}

__STATIC_INLINE void error(int error)
{
    __HEAP->error = error;
}

#endif // HEAP_H
