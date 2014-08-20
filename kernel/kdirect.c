/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "kdirect.h"
#include "kernel.h"
#include <string.h>

void kdirect_init(PROCESS* process)
{
    process->heap->direct_mode = DIRECT_NONE;
    process->heap->direct_process = INVALID_HANDLE;
}

void kdirect_read(PROCESS* other, void* addr, int size)
{
    PROCESS* process = kprocess_get_current();
    CHECK_ADDRESS_READ(process, addr, size);
    if (other->heap->direct_mode & DIRECT_READ && other->heap->direct_size >= size)
    {
        memcpy(addr, other->heap->direct_addr, size);
        other->heap->direct_mode &= ~DIRECT_READ;
    }
    else
        kprocess_error(process, ERROR_ACCESS_DENIED);
}

void kdirect_write(PROCESS* other, void* addr, int size)
{
    PROCESS* process = kprocess_get_current();
    CHECK_ADDRESS(process, addr, size);
    if (other->heap->direct_mode & DIRECT_WRITE && other->heap->direct_size >= size)
    {
        memcpy(other->heap->direct_addr, addr, size);
        other->heap->direct_mode &= ~DIRECT_WRITE;
    }
    else
        kprocess_error(process, ERROR_ACCESS_DENIED);
}
