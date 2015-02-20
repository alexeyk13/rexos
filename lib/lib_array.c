/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "lib_array.h"
#include "stdlib.h"
#include "../userspace/error.h"
#include <string.h>

#define ARRAY_HEADER_SIZE                               (sizeof(unsigned int) * 2)

ARRAY* lib_array_create(ARRAY** ar, unsigned int reserved)
{
    *ar = malloc(ARRAY_HEADER_SIZE + reserved);
    if (*ar)
    {
        (*ar)->reserved = reserved;
        (*ar)->size = 0;
    }
    return (*ar);
}

void lib_array_destroy(ARRAY **ar)
{
    free(*ar);
    *ar = NULL;
}

ARRAY* lib_array_append(ARRAY **ar, unsigned int size)
{
    if (*ar == NULL)
        return NULL;
    //already have space
    if ((*ar)->reserved - (*ar)->size >= size)
    {
        (*ar)->size += size;
        return (*ar);
    }
    size -= (*ar)->reserved - (*ar)->size;
    (*ar)->size = (*ar)->reserved;
    (*ar) = realloc(*ar, ARRAY_HEADER_SIZE + (size + (*ar)->size));
    if (*ar)
    {
        (*ar)->reserved += size;
        (*ar)->size += size;
    }
    return (*ar);
}

ARRAY* lib_array_insert(ARRAY **ar, unsigned int index, unsigned int size)
{
    if (lib_array_append(ar, size) == NULL)
        return NULL;
    memmove((*ar)->data + index + size, (*ar)->data + index, (*ar)->size - index - size);
    return (*ar);
}

ARRAY* lib_array_clear(ARRAY **ar)
{
    if (*ar == NULL)
        return NULL;
    (*ar)->size = 0;
    return (*ar);
}

ARRAY* lib_array_remove(ARRAY** ar, unsigned int index, unsigned int size)
{
    if (*ar == NULL)
        return NULL;
    if (index + size > (*ar)->size)
    {
        error(ERROR_OUT_OF_RANGE);
        return (*ar);
    }
    memmove((*ar)->data + index, (*ar)->data + index + size, (*ar)->size - index - size);
    (*ar)->size -= size;
    return (*ar);
}

ARRAY* lib_array_squeeze(ARRAY** ar)
{
    if (*ar == NULL)
        return NULL;
    *ar = realloc(*ar, ARRAY_HEADER_SIZE + (*ar)->size);
    (*ar)->reserved = (*ar)->size;
    return (*ar);
}

const LIB_ARRAY __LIB_ARRAY = {
    lib_array_create,
    lib_array_destroy,
    lib_array_append,
    lib_array_insert,
    lib_array_clear,
    lib_array_remove,
    lib_array_squeeze
};
