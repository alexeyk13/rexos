/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef KARRAY_H
#define KARRAY_H

#include "../userspace/array.h"

ARRAY* karray_create(ARRAY** ar, unsigned int data_size, unsigned int reserved);
void karray_destroy(ARRAY** ar);
void* karray_at_internal(ARRAY* ar, unsigned int index);
void* karray_at(ARRAY* ar, unsigned int index);
unsigned int karray_size_internal(ARRAY* ar);
unsigned int karray_size(ARRAY* ar);
void* karray_append(ARRAY** ar);
void* karray_insert(ARRAY** ar, unsigned int index);
ARRAY* karray_clear(ARRAY** ar);
ARRAY* karray_remove(ARRAY** ar, unsigned int index);
ARRAY* karray_squeeze(ARRAY** ar);

#endif // KARRAY_H
