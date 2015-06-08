/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "array.h"
#include "stdlib.h"
#include "heap.h"
#include "svc.h"
#include <string.h>

#define ARRAY_DATA(ar)                      (((void*)(ar)) + sizeof(ARRAY))

ARRAY* array_create(ARRAY** ar, unsigned int data_size, unsigned int reserved)
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

void array_destroy(ARRAY **ar)
{
    free(*ar);
    *ar = NULL;
}

void* array_at(ARRAY* ar, unsigned int index)
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

unsigned int array_size(ARRAY* ar)
{
    if (ar == NULL)
        return 0;
    return ar->size;
}

ARRAY* array_append(ARRAY **ar)
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

ARRAY* array_insert(ARRAY **ar, unsigned int index)
{
    if (array_append(ar) == NULL)
        return NULL;
    if (index >= (*ar)->size)
    {
        error(ERROR_OUT_OF_RANGE);
        return (*ar);
    }
    memmove(ARRAY_DATA(*ar) + (index + 1) * (*ar)->data_size, ARRAY_DATA(*ar) + index * (*ar)->data_size, ((*ar)->size - index - 1) * (*ar)->data_size);
    return (*ar);
}

ARRAY* array_clear(ARRAY **ar)
{
    if (*ar == NULL)
        return NULL;
    (*ar)->size = 0;
    return (*ar);
}

ARRAY* array_remove(ARRAY** ar, unsigned int index)
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

ARRAY* array_squeeze(ARRAY** ar)
{
    if (*ar == NULL)
        return NULL;
    *ar = realloc(*ar, sizeof(ARRAY) + (*ar)->size * (*ar)->data_size);
    (*ar)->reserved = (*ar)->size;
    return (*ar);
}
