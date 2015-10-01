/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef ARRAY_H
#define ARRAY_H

#include "types.h"

typedef struct _ARRAY ARRAY;

ARRAY* array_create(ARRAY** ar, unsigned int data_size, unsigned int reserved);
void array_destroy(ARRAY** ar);
void* array_at(ARRAY* ar, unsigned int index);
unsigned int array_size(ARRAY* ar);
void* array_append(ARRAY** ar);
void* array_insert(ARRAY** ar, unsigned int index);
ARRAY* array_clear(ARRAY** ar);
ARRAY* array_remove(ARRAY** ar, unsigned int index);
ARRAY* array_squeeze(ARRAY** ar);

#endif // ARRAY_H
