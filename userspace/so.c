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
#define SO_AT(so, index)                        (*((unsigned int*)array_at((so)->ar, (index))))
#define SO_DATA(so, handle)                     ((void*)((uint8_t*)array_at((so)->ar, SO_INDEX(handle)) + sizeof(unsigned int)))

SO* so_create(SO* so, unsigned int data_size, unsigned int reserved)
{
    if (!array_create(&so->ar, (data_size < sizeof(unsigned int) ? sizeof(unsigned int) : data_size) + sizeof(unsigned int), reserved))
        return NULL;
    so->first_free = SO_FREE;
    return so;
}

void so_destroy(SO* so)
{
    array_destroy(&so->ar);
}

unsigned int so_allocate(SO* so)
{
    unsigned int handle = INVALID_HANDLE;
    //no free, append array
    if (so->first_free == SO_FREE)
    {
        if (array_append(&so->ar))
        {
            SO_AT(so, array_size(so->ar) - 1) = 0;
            handle = SO_HANDLE(array_size(so->ar) - 1, 0);
        }
    }
    //get first
    else
    {
        handle = SO_HANDLE(so->first_free, SO_SEQUENCE(SO_AT(so, so->first_free)));
        so->first_free = SO_INDEX(SO_AT(so, so->first_free));
    }
    return handle;
}

static bool so_check_handle(SO* so, unsigned int handle)
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

void so_free(SO* so, unsigned int handle)
{
    unsigned int tmp;
    if (!so_check_handle(so, handle))
        return;
    tmp = SO_AT(so, SO_INDEX(handle));
    SO_AT(so, SO_INDEX(handle)) = SO_HANDLE(so->first_free, SO_SEQUENCE(tmp) + 1);
    so->first_free = SO_INDEX(tmp);
}

void* so_get(SO* so, unsigned int handle)
{
    if (!so_check_handle(so, handle))
        return NULL;
    return SO_DATA(so, handle);
}
