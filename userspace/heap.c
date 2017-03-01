/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "heap.h"
#include "svc.h"

HANDLE heap_create(unsigned int size)
{
    HANDLE heap;
    svc_call(SVC_HEAP_CREATE, (unsigned int)&heap, size, 0);
    return heap;
}

void heap_destroy(HANDLE heap)
{
    svc_call(SVC_HEAP_DESTROY, (unsigned int)heap, 0, 0);
}

void* heap_malloc(HANDLE heap, unsigned int size)
{
    void* ptr;
    svc_call(SVC_HEAP_MALLOC, (unsigned int)&ptr, (unsigned int)heap, size);
    return ptr;
}

void heap_free(void* ptr)
{
    svc_call(SVC_HEAP_FREE, (unsigned int)ptr, 0, 0);
}
