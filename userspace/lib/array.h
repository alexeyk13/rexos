/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef ARRAY_H
#define ARRAY_H

#include "types.h"
#include "lib.h"
#include "../svc.h"

typedef struct {
    ARRAY* (*array_create)(ARRAY** ar, unsigned int reserved);
    void (*array_destroy)(ARRAY** ar);
    ARRAY* (*array_add)(ARRAY** ar, unsigned int size);
    ARRAY* (*array_remove)(ARRAY** ar, unsigned int index);
    ARRAY* (*array_squeeze)(ARRAY** ar);
} LIB_ARRAY;

__STATIC_INLINE ARRAY* array_create(ARRAY** ar, unsigned int reserved)
{
    return ((const LIB_ARRAY*)__GLOBAL->lib->p_lib_array)->array_create(ar, reserved);
}

__STATIC_INLINE void array_destroy(ARRAY** ar)
{
    ((const LIB_ARRAY*)__GLOBAL->lib->p_lib_array)->array_destroy(ar);
}

__STATIC_INLINE ARRAY* array_add(ARRAY** ar, unsigned int size)
{
    return ((const LIB_ARRAY*)__GLOBAL->lib->p_lib_array)->array_add(ar, size);
}

__STATIC_INLINE ARRAY* array_remove(ARRAY** ar, unsigned int index)
{
    return ((const LIB_ARRAY*)__GLOBAL->lib->p_lib_array)->array_remove(ar, index);
}

__STATIC_INLINE ARRAY* array_squeeze(ARRAY** ar)
{
    return ((const LIB_ARRAY*)__GLOBAL->lib->p_lib_array)->array_squeeze(ar);
}

#endif // ARRAY_H
