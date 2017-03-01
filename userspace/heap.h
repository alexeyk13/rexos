/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef HEAP_H
#define HEAP_H

#include "types.h"

HANDLE heap_create(unsigned int size);
void heap_destroy(HANDLE heap);

void* heap_malloc(HANDLE heap, unsigned int size);
void heap_free(void* ptr);

#endif // HEAP_H
