/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
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

void kobject_set(HANDLE process, int idx, HANDLE handle)
{
    if (idx < KERNEL_OBJECTS_COUNT)
    {
        if (__KERNEL->objects[idx] == INVALID_HANDLE || (__KERNEL->objects[idx] == process && handle == INVALID_HANDLE))
            __KERNEL->objects[idx] = handle;
        else
            error(ERROR_ACCESS_DENIED);
    }
    else
        error(ERROR_OUT_OF_RANGE);
}

HANDLE kobject_get(int idx)
{
    if (idx < KERNEL_OBJECTS_COUNT)
        return __KERNEL->objects[idx];
    error(ERROR_OUT_OF_RANGE);
    return INVALID_HANDLE;
}
