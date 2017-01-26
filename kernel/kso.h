/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef KSO_H
#define KSO_H

#include "../userspace/so.h"

SO* kso_create(SO* so, unsigned int data_size, unsigned int reserved);
void kso_destroy(SO* so);
//Remember: always re-fetch objects after kso_allocate
HANDLE kso_allocate(SO* so);
bool kso_check_handle(SO* so, HANDLE handle);
void kso_free(SO* so, HANDLE handle);
void* kso_get(SO* so, HANDLE handle);
HANDLE kso_first(SO* so);
HANDLE kso_next(SO* so, HANDLE prev);
unsigned int kso_count(SO* so);

#endif // KSO_H
