/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2016, Alexey Kramarenko
    All rights reserved.
*/

#ifndef ARRAY_H
#define ARRAY_H

#include "types.h"
#include "lib.h"
#include "process.h"

typedef struct _ARRAY ARRAY;

typedef struct {
    ARRAY* (*lib_array_create)(ARRAY**, unsigned int, unsigned int);
    void (*lib_array_destroy)(ARRAY**);
    void* (*lib_array_at)(ARRAY*, unsigned int);
    unsigned int (*lib_array_size)(ARRAY*);
    void* (*lib_array_append)(ARRAY**);
    void* (*lib_array_insert)(ARRAY**, unsigned int);
    ARRAY* (*lib_array_clear)(ARRAY**);
    ARRAY* (*lib_array_remove)(ARRAY**, unsigned int);
    ARRAY* (*lib_array_squeeze)(ARRAY**);
} LIB_ARRAY;

__STATIC_INLINE ARRAY* array_create(ARRAY** ar, unsigned int data_size, unsigned int reserved)
{
    return ((const LIB_ARRAY*)__GLOBAL->lib[LIB_ID_ARRAY])->lib_array_create(ar, data_size, reserved);
}

__STATIC_INLINE void array_destroy(ARRAY** ar)
{
    ((const LIB_ARRAY*)__GLOBAL->lib[LIB_ID_ARRAY])->lib_array_destroy(ar);
}

__STATIC_INLINE void* array_at(ARRAY* ar, unsigned int index)
{
    return ((const LIB_ARRAY*)__GLOBAL->lib[LIB_ID_ARRAY])->lib_array_at(ar, index);
}

__STATIC_INLINE unsigned int array_size(ARRAY* ar)
{
    return ((const LIB_ARRAY*)__GLOBAL->lib[LIB_ID_ARRAY])->lib_array_size(ar);
}

__STATIC_INLINE void* array_append(ARRAY** ar)
{
    return ((const LIB_ARRAY*)__GLOBAL->lib[LIB_ID_ARRAY])->lib_array_append(ar);
}

__STATIC_INLINE void* array_insert(ARRAY** ar, unsigned int index)
{
    return ((const LIB_ARRAY*)__GLOBAL->lib[LIB_ID_ARRAY])->lib_array_insert(ar, index);
}

__STATIC_INLINE ARRAY* array_clear(ARRAY** ar)
{
    return ((const LIB_ARRAY*)__GLOBAL->lib[LIB_ID_ARRAY])->lib_array_clear(ar);
}

__STATIC_INLINE ARRAY* array_remove(ARRAY** ar, unsigned int index)
{
    return ((const LIB_ARRAY*)__GLOBAL->lib[LIB_ID_ARRAY])->lib_array_remove(ar, index);
}

__STATIC_INLINE ARRAY* array_squeeze(ARRAY** ar)
{
    return ((const LIB_ARRAY*)__GLOBAL->lib[LIB_ID_ARRAY])->lib_array_squeeze(ar);
}

#endif // ARRAY_H
