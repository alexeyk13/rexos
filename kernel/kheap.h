/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef KHEAP_H
#define KHEAP_H

#include "../userspace/types.h"

HANDLE kheap_create(unsigned int size);
void kheap_destroy(HANDLE h);

void* kheap_malloc(HANDLE h, unsigned int size);
void kheap_free(void* ptr);

void kheap_send(void* ptr, HANDLE process);

#endif // KHEAP_H
