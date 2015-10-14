/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "so.h"
#include "error.h"
#include "process.h"

#define SO_INDEX(handle)                        ((handle) >> 8)
#define SO_SEQUENCE(handle)                     ((handle) & 0xff)
#define SO_HANDLE(index, sequence)              (((index) << 8) | ((sequence) & 0xff))
#define SO_FREE                                 0xffffff
#define SO_AT(so, index)                        (*((HANDLE*)array_at((so)->ar, (index))))
#define SO_DATA(so, index)                      ((void*)((uint8_t*)array_at((so)->ar, (index)) + sizeof(HANDLE)))

SO* so_create(SO* so, unsigned int data_size, unsigned int reserved)
{
    if (!array_create(&so->ar, data_size + sizeof(HANDLE), reserved))
        return NULL;
    so->first_free = SO_FREE;
    return so;
}

void so_destroy(SO* so)
{
    array_destroy(&so->ar);
}

HANDLE so_allocate(SO* so)
{
    HANDLE handle = INVALID_HANDLE;
    //no free, append array
    if (so->first_free == SO_FREE)
    {
        if (array_append(&so->ar))
            handle = SO_AT(so, array_size(so->ar) - 1) = SO_HANDLE(array_size(so->ar) - 1, 0);
    }
    //get first
    else
    {
        handle = SO_HANDLE(so->first_free, SO_SEQUENCE(SO_AT(so, so->first_free)));
        SO_AT(so, so->first_free) = handle;
        so->first_free = *(unsigned int*)SO_DATA(so, so->first_free);
    }
    return handle;
}

static bool so_check_handle(SO* so, HANDLE handle)
{
    if (SO_INDEX(handle) >= array_size(so->ar))
    {
        error(ERROR_OUT_OF_RANGE);
        return false;
    }
    if (SO_INDEX(SO_AT(so, SO_INDEX(handle))) == SO_FREE)
    {
        error(ERROR_NOT_CONFIGURED);
        return false;
    }
    if (SO_SEQUENCE(SO_AT(so, SO_INDEX(handle))) != SO_SEQUENCE(handle))
    {
        error(ERROR_INVALID_MAGIC);
        return false;
    }
    return true;
}

void so_free(SO* so, HANDLE handle)
{
    if (!so_check_handle(so, handle))
        return;
    *(unsigned int*)SO_DATA(so, SO_INDEX(handle)) = so->first_free;
    so->first_free = SO_INDEX(handle);
    SO_AT(so, so->first_free) = SO_HANDLE(SO_FREE, SO_SEQUENCE(handle) + 1);
}

void* so_get(SO* so, HANDLE handle)
{
    if (!so_check_handle(so, handle))
        return NULL;
    return SO_DATA(so, SO_INDEX(handle));
}

HANDLE so_first(SO* so)
{
    int i;
    for (i = 0; i < array_size(so->ar); ++i)
    {
        if (SO_INDEX(SO_AT(so, i)) != SO_FREE)
            return SO_AT(so, i);
    }
    return INVALID_HANDLE;
}

HANDLE so_next(SO* so, HANDLE prev)
{
    int i;
    for (i = SO_INDEX(prev) + 1; i < array_size(so->ar); ++i)
    {
        if (SO_INDEX(SO_AT(so, i)) != SO_FREE)
            return SO_AT(so, i);
    }
    return INVALID_HANDLE;
}
