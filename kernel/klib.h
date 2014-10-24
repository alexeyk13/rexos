/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef KLIB_H
#define KLIB_H

#include "kernel.h"

#define LIB_ENTER                                           void* __saved_heap = __GLOBAL->heap;\
                                                            __GLOBAL->heap = __KERNEL; \
                                                            __KERNEL->error = ERROR_OK;

#define LIB_EXIT                                            __GLOBAL->heap = __saved_heap;

extern const void *const __LIB[];

void lib_stub ();

#endif // KLIB_H
