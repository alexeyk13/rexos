/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "lib_heap.h"
#include <string.h>

void* __heap_struct_ptr(HEAP* heap, HEAP_STRUCT_TYPE struct_type)
{
    unsigned int res = 0;
    if (struct_type >= HEAP_STRUCT_NAME)
        res = (unsigned int)heap + sizeof(HEAP);
    if (struct_type == HEAP_STRUCT_FREE)
        res += (heap->flags & HEAP_PERSISTENT_NAME) ? sizeof(char*) : strlen((char*)res) + 1;
    return (void*)res;
}

char* __process_name(HEAP* heap)
{
    void* ptr = __heap_struct_ptr(heap, HEAP_STRUCT_NAME);
    return (heap->flags & HEAP_PERSISTENT_NAME) ? *((char**)ptr) : (char*)ptr;
}

const LIB_HEAP __LIB_HEAP = {
    __heap_struct_ptr,
    __process_name
};
