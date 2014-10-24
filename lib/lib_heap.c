/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "lib_heap.h"

char* __process_name(void* heap)
{
    return ((char*)((unsigned int)heap + sizeof(HEAP)));
}

const LIB_HEAP __LIB_HEAP = {
    __process_name
};
