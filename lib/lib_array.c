/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "lib_array.h"
#include "stdlib.h"
#include "../userspace/error.h"
#include <string.h>

#define ARRAY_DATA(ar)                      (((void*)(ar)) + sizeof(ARRAY))

ARRAY* lib_array_create(ARRAY** ar, unsigned int data_size, unsigned int reserved)
{
    *ar = malloc(sizeof(ARRAY) + data_size * reserved);
    if (*ar)
    {
        (*ar)->reserved = reserved;
        (*ar)->size = 0;
        (*ar)->data_size = data_size;
    }
    return (*ar);
}

void lib_array_destroy(ARRAY **ar)
{
    free(*ar);
    *ar = NULL;
}

void* lib_array_at(ARRAY* ar, unsigned int index)
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

unsigned int lib_array_size(ARRAY* ar)
{
    if (ar == NULL)
        return 0;
    return ar->size;
}

ARRAY* lib_array_append(ARRAY **ar)
{
    if (*ar == NULL)
        return NULL;
    //already have space
    if ((*ar)->reserved > (*ar)->size)
        ++(*ar)->size;
    else
    {
        ++(*ar)->reserved;
        ++(*ar)->size;
        (*ar) = realloc(*ar, sizeof(ARRAY) + (*ar)->data_size * (*ar)->reserved);
    }
    return (*ar);
}

ARRAY* lib_array_insert(ARRAY **ar, unsigned int index)
{
    if (lib_array_append(ar) == NULL)
        return NULL;
    if (index >= (*ar)->size)
    {
        error(ERROR_OUT_OF_RANGE);
        return (*ar);
    }
    memmove(ARRAY_DATA(*ar) + (index + 1) * (*ar)->data_size, ARRAY_DATA(*ar) + index * (*ar)->data_size, ((*ar)->size - index - 1) * (*ar)->data_size);
    return (*ar);
}

ARRAY* lib_array_clear(ARRAY **ar)
{
    if (*ar == NULL)
        return NULL;
    (*ar)->size = 0;
    return (*ar);
}

ARRAY* lib_array_remove(ARRAY** ar, unsigned int index)
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

ARRAY* lib_array_squeeze(ARRAY** ar)
{
    if (*ar == NULL)
        return NULL;
    *ar = realloc(*ar, sizeof(ARRAY) + (*ar)->size * (*ar)->data_size);
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
