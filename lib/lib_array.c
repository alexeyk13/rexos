/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "lib_array.h"
#include "../userspace/error.h"
#include <string.h>

typedef struct _ARRAY {
    unsigned int size, reserved, data_size;
} ARRAY;

#define ARRAY_DATA(ar)                      ((void*)(((uint8_t*)(ar)) + sizeof(ARRAY)))

ARRAY* lib_array_create(ARRAY** ar, const STD_MEM* std_mem, unsigned int data_size, unsigned int reserved)
{
    *ar = std_mem->fn_malloc(sizeof(ARRAY) + data_size * reserved);
    if (*ar)
    {
        (*ar)->reserved = reserved;
        (*ar)->size = 0;
        (*ar)->data_size = data_size;
    }
    return (*ar);
}

void lib_array_destroy(ARRAY **ar, const STD_MEM* std_mem)
{
    std_mem->fn_free(*ar);
    *ar = NULL;
}

void* lib_array_at(ARRAY* ar, const STD_MEM* std_mem, unsigned int index)
{
    if (ar == NULL)
        return NULL;
    if (index >= ar->size)
    {
        error(ERROR_OUT_OF_RANGE);
        return NULL;
    }
    return ARRAY_DATA(ar) + index * ar->data_size;
}

unsigned int lib_array_size(ARRAY* ar, const STD_MEM* std_mem)
{
    if (ar == NULL)
        return 0;
    return ar->size;
}

void* lib_array_append(ARRAY **ar, const STD_MEM* std_mem)
{
    ARRAY* tmp;
    if (*ar == NULL)
        return NULL;
    //already have space
    if ((*ar)->reserved > (*ar)->size)
        ++(*ar)->size;
    else
    {
        ++(*ar)->reserved;
        ++(*ar)->size;
        tmp = std_mem->fn_realloc(*ar, sizeof(ARRAY) + (*ar)->data_size * (*ar)->reserved);
        if (tmp == NULL)
            return NULL;
        (*ar) = tmp;
    }
    return lib_array_at(*ar, std_mem, (*ar)->size - 1);
}

void* lib_array_insert(ARRAY **ar, const STD_MEM* std_mem, unsigned int index)
{
    if (array_append(ar) == NULL)
        return NULL;
    if (index >= (*ar)->size)
    {
        error(ERROR_OUT_OF_RANGE);
        return (*ar);
    }
    memmove(ARRAY_DATA(*ar) + (index + 1) * (*ar)->data_size, ARRAY_DATA(*ar) + index * (*ar)->data_size, ((*ar)->size - index - 1) * (*ar)->data_size);
    return lib_array_at(*ar, std_mem, index);
}

ARRAY* lib_array_clear(ARRAY **ar, const STD_MEM* std_mem)
{
    if (*ar == NULL)
        return NULL;
    (*ar)->size = 0;
    return (*ar);
}

ARRAY* lib_array_remove(ARRAY** ar, const STD_MEM* std_mem, unsigned int index)
{
    if (*ar == NULL)
        return NULL;
    if (index >= (*ar)->size)
    {
        error(ERROR_OUT_OF_RANGE);
        return (*ar);
    }
    memmove(ARRAY_DATA(*ar) + index * (*ar)->data_size, ARRAY_DATA(*ar) + (index + 1) * (*ar)->data_size, ((*ar)->size - index - 1) * (*ar)->data_size);
    --(*ar)->size;
    return (*ar);
}

ARRAY* lib_array_squeeze(ARRAY** ar, const STD_MEM* std_mem)
{
    if (*ar == NULL)
        return NULL;
    *ar = std_mem->fn_realloc(*ar, sizeof(ARRAY) + (*ar)->size * (*ar)->data_size);
    (*ar)->reserved = (*ar)->size;
    return (*ar);
}

const LIB_ARRAY __LIB_ARRAY = {
    lib_array_create,
    lib_array_destroy,
    lib_array_at,
    lib_array_size,
    lib_array_append,
    lib_array_insert,
    lib_array_clear,
    lib_array_remove,
    lib_array_squeeze
};
