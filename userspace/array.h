/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef ARRAY_H
#define ARRAY_H

#include "types.h"
#include "lib.h"
#include "heap.h"
#include "svc.h"

typedef struct {
    unsigned int size, reserved;
    char data[65535];
} ARRAY;

typedef struct {
    ARRAY* (*lib_array_create)(ARRAY** ar, unsigned int reserved);
    void (*lib_array_destroy)(ARRAY** ar);
    ARRAY* (*lib_array_add)(ARRAY** ar, unsigned int size);
    ARRAY* (*lib_array_clear)(ARRAY** ar);
    ARRAY* (*lib_array_remove)(ARRAY** ar, unsigned int index, unsigned int size);
    ARRAY* (*lib_array_squeeze)(ARRAY** ar);
} LIB_ARRAY;

__STATIC_INLINE char* array_data(ARRAY* ar)
{
    return ar->data;
}

__STATIC_INLINE unsigned int array_size(ARRAY* ar)
{
    return ar->size;
}

__STATIC_INLINE ARRAY* array_create(ARRAY** ar, unsigned int reserved)
{
    LIB_CHECK_RET(LIB_ID_ARRAY);
    return ((const LIB_ARRAY*)__GLOBAL->lib[LIB_ID_ARRAY])->lib_array_create(ar, reserved);
}

__STATIC_INLINE void array_destroy(ARRAY** ar)
{
    LIB_CHECK(LIB_ID_ARRAY);
    ((const LIB_ARRAY*)__GLOBAL->lib[LIB_ID_ARRAY])->lib_array_destroy(ar);
}

__STATIC_INLINE ARRAY* array_add(ARRAY** ar, unsigned int size)
{
    LIB_CHECK_RET(LIB_ID_ARRAY);
    return ((const LIB_ARRAY*)__GLOBAL->lib[LIB_ID_ARRAY])->lib_array_add(ar, size);
}

__STATIC_INLINE ARRAY* array_clear(ARRAY** ar)
{
    LIB_CHECK_RET(LIB_ID_ARRAY);
    return ((const LIB_ARRAY*)__GLOBAL->lib[LIB_ID_ARRAY])->lib_array_clear(ar);
}

__STATIC_INLINE ARRAY* array_remove(ARRAY** ar, unsigned int index, unsigned int size)
{
    LIB_CHECK_RET(LIB_ID_ARRAY);
    return ((const LIB_ARRAY*)__GLOBAL->lib[LIB_ID_ARRAY])->lib_array_remove(ar, index, size);
}

__STATIC_INLINE ARRAY* array_squeeze(ARRAY** ar)
{
    LIB_CHECK_RET(LIB_ID_ARRAY);
    return ((const LIB_ARRAY*)__GLOBAL->lib[LIB_ID_ARRAY])->lib_array_squeeze(ar);
}

__STATIC_INLINE void** void_array_data(ARRAY* ar)
{
    return (void*)(ar->data);
}

__STATIC_INLINE unsigned int void_array_size(ARRAY* ar)
{
    return ar->size / sizeof(void*);
}

__STATIC_INLINE ARRAY* void_array_create(ARRAY** ar, unsigned int reserved)
{
    LIB_CHECK_RET(LIB_ID_ARRAY);
    return ((const LIB_ARRAY*)__GLOBAL->lib[LIB_ID_ARRAY])->lib_array_create(ar, reserved * sizeof(void*));
}

__STATIC_INLINE ARRAY* void_array_add(ARRAY** ar, unsigned int size)
{
    LIB_CHECK_RET(LIB_ID_ARRAY);
    return ((const LIB_ARRAY*)__GLOBAL->lib[LIB_ID_ARRAY])->lib_array_add(ar, size * sizeof(void*));
}

__STATIC_INLINE ARRAY* void_array_remove(ARRAY** ar, unsigned int index, unsigned int size)
{
    LIB_CHECK_RET(LIB_ID_ARRAY);
    return ((const LIB_ARRAY*)__GLOBAL->lib[LIB_ID_ARRAY])->lib_array_remove(ar, index * sizeof(void*), size * sizeof(void*));
}

#endif // ARRAY_H
