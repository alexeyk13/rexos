/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef ARRAY_H
#define ARRAY_H

#include "types.h"
#include "lib.h"
#include "../process.h"

__STATIC_INLINE ARRAY* array_create(unsigned int reserved)
{
    return __GLOBAL->lib->array_create(reserved);
}

__STATIC_INLINE void array_destroy(ARRAY* ar)
{
    __GLOBAL->lib->array_destroy(ar);
}

__STATIC_INLINE ARRAY* array_add(ARRAY* ar, unsigned int size)
{
    return __GLOBAL->lib->array_add(ar, size);
}

__STATIC_INLINE ARRAY* array_remove(ARRAY* ar, unsigned int index)
{
    return __GLOBAL->lib->array_remove(ar, index);
}

__STATIC_INLINE ARRAY* array_squeeze(ARRAY* ar)
{
    return __GLOBAL->lib->array_squeeze(ar);
}

#endif // ARRAY_H
