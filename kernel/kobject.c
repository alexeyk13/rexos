/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "kobject.h"
#include "kernel.h"

void kobject_init()
{
    int i;
    for (i = 0; i < KERNEL_OBJECTS_COUNT; ++i)
        __KERNEL->objects[i] = INVALID_HANDLE;
}

void kobject_set(int idx, HANDLE handle)
{
    PROCESS* process = kprocess_get_current();
    if (idx < KERNEL_OBJECTS_COUNT)
    {
        if (__KERNEL->objects[idx] == INVALID_HANDLE || (__KERNEL->objects[idx] == (HANDLE)process && handle == INVALID_HANDLE))
            __KERNEL->objects[idx] = handle;
        else
            kprocess_error(process, ERROR_ACCESS_DENIED);
    }
    else
        kprocess_error(process, ERROR_OUT_OF_RANGE);
}

void kobject_get(int idx, HANDLE* handle)
{
    PROCESS* process = kprocess_get_current();
    CHECK_ADDRESS(process, handle, sizeof(HANDLE));
    if (idx < KERNEL_OBJECTS_COUNT)
        *handle = __KERNEL->objects[idx];
    else
        kprocess_error(process, ERROR_OUT_OF_RANGE);
}
