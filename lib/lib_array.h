/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef LIB_ARRAY_H
#define LIB_ARRAY_H

#include "../userspace/lib/types.h"

ARRAY* __array_create(unsigned int reserved);
void __array_destroy(ARRAY* ar);
ARRAY* __array_add(ARRAY* ar, unsigned int size);
ARRAY* __array_remove(ARRAY* ar, unsigned int index);
ARRAY* __array_squeeze(ARRAY* ar);

#endif // LIB_ARRAY_H
