/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "lib_so.h"
#include "lib_array.h"
#include "../userspace/error.h"
#include "kernel_config.h"

#define SO_INDEX(handle)                        ((handle) >> 8)
#define SO_SEQUENCE(handle)                     ((handle) & 0xff)
#define SO_HANDLE(index, sequence)              (((index) << 8) | ((sequence) & 0xff))
#define SO_FREE                                 0xffffff
#define SO_AT(so, std_mem, index)               (*((HANDLE*)lib_array_at((so)->ar, (std_mem), (index))))
#define SO_DATA(so, std_mem, index)             ((void*)((uint8_t*)lib_array_at((so)->ar, (std_mem), (index)) + sizeof(HANDLE)))

SO* lib_so_create(SO* so, const STD_MEM* std_mem, unsigned int data_size, unsigned int reserved)
{
    if (!lib_array_create(&so->ar, std_mem, data_size + sizeof(HANDLE), reserved))
        return NULL;
    so->first_free = SO_FREE;
    return so;
}

void lib_so_destroy(SO* so, const STD_MEM* std_mem)
{
    lib_array_destroy(&so->ar, std_mem);
}

HANDLE lib_so_allocate(SO* so, const STD_MEM* std_mem)
{
    HANDLE handle = INVALID_HANDLE;
    //no free, append array
    if (so->first_free == SO_FREE)
    {
        if (lib_array_append(&so->ar, std_mem))
            handle = SO_AT(so, std_mem, lib_array_size(so->ar, std_mem) - 1) = SO_HANDLE(lib_array_size(so->ar, std_mem) - 1, 0);
    }
    //get first
    else
    {
        handle = SO_HANDLE(so->first_free, SO_SEQUENCE(SO_AT(so, std_mem, so->first_free)));
        SO_AT(so, std_mem, so->first_free) = handle;
        so->first_free = *(unsigned int*)SO_DATA(so, std_mem, so->first_free);
    }
    return handle;
}

bool lib_so_check_handle(SO* so, const STD_MEM* std_mem, HANDLE handle)
{
    if (SO_INDEX(handle) >= lib_array_size(so->ar, std_mem))
    {
        error(ERROR_OUT_OF_RANGE);
        return false;
    }
    if (SO_INDEX(SO_AT(so, std_mem, SO_INDEX(handle))) == SO_FREE)
    {
        error(ERROR_NOT_CONFIGURED);
        return false;
    }
    if (SO_SEQUENCE(SO_AT(so, std_mem, SO_INDEX(handle))) != SO_SEQUENCE(handle))
    {
        error(ERROR_INVALID_MAGIC);
        return false;
    }
    return true;
}

void lib_so_free(SO* so, const STD_MEM* std_mem, HANDLE handle)
{
    if (!lib_so_check_handle(so, std_mem, handle))
        return;
    *(unsigned int*)SO_DATA(so, std_mem, SO_INDEX(handle)) = so->first_free;
    so->first_free = SO_INDEX(handle);
    SO_AT(so, std_mem, so->first_free) = SO_HANDLE(SO_FREE, SO_SEQUENCE(handle) + 1);
}

void* lib_so_get(SO* so, const STD_MEM* std_mem, HANDLE handle)
{
#if (KERNEL_HANDLE_CHECKING)
    if (!lib_so_check_handle(so, std_mem, handle))
        return NULL;
#endif //KERNEL_HANDLE_CHECKING
    return SO_DATA(so, std_mem, SO_INDEX(handle));
}

HANDLE lib_so_first(SO* so, const STD_MEM* std_mem)
{
    int i;
    for (i = 0; i < lib_array_size(so->ar, std_mem); ++i)
    {
        if (SO_INDEX(SO_AT(so, std_mem, i)) != SO_FREE)
            return SO_AT(so, std_mem, i);
    }
    return INVALID_HANDLE;
}

HANDLE lib_so_next(SO* so, const STD_MEM* std_mem, HANDLE prev)
{
    int i;
    for (i = SO_INDEX(prev) + 1; i < lib_array_size(so->ar, std_mem); ++i)
    {
        if (SO_INDEX(SO_AT(so, std_mem, i)) != SO_FREE)
            return SO_AT(so, std_mem, i);
    }
    return INVALID_HANDLE;
}

unsigned int lib_so_count(SO* so, const STD_MEM* std_mem)
{
    unsigned int idx;
    unsigned int res = lib_array_size(so->ar, std_mem);
    if (so->first_free == SO_FREE)
        return res;
    for (idx = so->first_free; idx != SO_FREE; idx = SO_INDEX(SO_AT(so, std_mem, idx)))
        --res;
    return res;
}

const LIB_SO __LIB_SO = {
    lib_so_create,
    lib_so_destroy,
    lib_so_allocate,
    lib_so_check_handle,
    lib_so_free,
    lib_so_get,
    lib_so_first,
    lib_so_next,
    lib_so_count
};
