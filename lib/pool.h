/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef POOL_H
#define POOL_H

#include "../userspace/types.h"
#include "kernel_config.h"

void pool_init(POOL* pool, void* data);
void* pool_malloc(POOL* pool, size_t size, void *sp);
size_t pool_slot_size(POOL* poll, void* ptr);
void* pool_realloc(POOL* pool, void* ptr, size_t size, void* sp);
void pool_free(POOL* pool, void* ptr);

#if (KERNEL_PROFILING)
void* pool_free_ptr(POOL* pool);
bool pool_check(POOL* pool, void *sp);
void pool_stat(POOL* pool, POOL_STAT* stat, void *sp);
#endif

#endif // POOL_H
