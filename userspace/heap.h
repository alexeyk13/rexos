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

typedef enum {
    HEAP_STRUCT_NAME,
    HEAP_STRUCT_FREE
} HEAP_STRUCT_TYPE;

/*
    int error
    self handle - optional
    system - remove
    stdout - optional
    stdin - optional
    direct - optional
    POOL optional

 */

#define HEAP_PERSISTENT_NAME                                (1 << 0)

typedef struct {
    int error;
    char flags;
    POOL pool;
    //stdout/stdin handle. System specific
    HANDLE stdout, stdin;
    int direct_mode;
    HANDLE direct_process;
    void* direct_addr;
    unsigned int direct_size;
} HEAP;

typedef struct {
    void* (*__heap_struct_ptr)(HEAP*, HEAP_STRUCT_TYPE);
    char* (*__process_name)(HEAP*);
} LIB_HEAP;

#define __HEAP                                              ((HEAP*)(((GLOBAL*)(SRAM_BASE))->heap))

__STATIC_INLINE void* heap_struct_ptr(HEAP_STRUCT_TYPE struct_type)
{
    return ((const LIB_HEAP*)__GLOBAL->lib[LIB_ID_HEAP])->__heap_struct_ptr(__HEAP, struct_type);
}

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
