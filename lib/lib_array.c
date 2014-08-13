/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "lib_array.h"
#include "stdlib.h"
#include "../userspace/error.h"

#define ARRAY_HEADER_SIZE                               (sizeof(unsigned int) * 2)

ARRAY* __array_create(ARRAY** ar, unsigned int reserved)
{
    *ar = malloc(ARRAY_HEADER_SIZE + sizeof(void*) * reserved);
    if (*ar)
    {
        (*ar)->reserved = reserved;
        (*ar)->size = 0;
    }
    return (*ar);
}

void __array_destroy(ARRAY **ar)
{
    free(*ar);
    *ar = NULL;
}

ARRAY* __array_add(ARRAY **ar, unsigned int size)
{
    //already have space
    if ((*ar)->reserved - (*ar)->size >= size)
    {
        (*ar)->size += size;
        return (*ar);
    }
    size -= (*ar)->reserved - (*ar)->size;
    (*ar)->size = (*ar)->reserved;
    (*ar) = realloc(*ar, ARRAY_HEADER_SIZE + sizeof(void*) * (size + (*ar)->size));
    if (*ar)
    {
        (*ar)->reserved += size;
        (*ar)->size += size;
    }
    return (*ar);
}

ARRAY* __array_remove(ARRAY** ar, unsigned int index)
{
    if (index < (*ar)->size - 1)
        (*ar)->data[index] = (*ar)->data[--(*ar)->size];
    else if (index == (*ar)->size - 1)
        --(*ar)->size;
    else
        error(ERROR_OUT_OF_RANGE);
    return (*ar);
}

ARRAY* __array_squeeze(ARRAY** ar)
{
    *ar = realloc(*ar, ARRAY_HEADER_SIZE + sizeof(void*) * (*ar)->size);
    (*ar)->reserved = (*ar)->size;
    return (*ar);
}
