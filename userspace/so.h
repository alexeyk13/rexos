/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef SO_H
#define SO_H

#include "array.h"
#include "types.h"
#include "stdlib.h"

//defined public for less fragmentaion
typedef struct _SO {
    ARRAY* ar;
    unsigned int first_free;
} SO;

typedef struct {
    SO* (*lib_so_create)(SO*, const STD_MEM*, unsigned int, unsigned int);
    void (*lib_so_destroy)(SO*, const STD_MEM*);
    HANDLE (*lib_so_allocate)(SO*, const STD_MEM*);
    bool (*lib_so_check_handle)(SO*, const STD_MEM*, HANDLE);
    void (*lib_so_free)(SO*, const STD_MEM*, HANDLE);
    void* (*lib_so_get)(SO*, const STD_MEM*, HANDLE);
    HANDLE (*lib_so_first)(SO*, const STD_MEM*);
    HANDLE (*lib_so_next)(SO*, const STD_MEM*, HANDLE);
    unsigned int (*lib_so_count)(SO*, const STD_MEM*);
} LIB_SO;

__STATIC_INLINE SO* so_create(SO* so, unsigned int data_size, unsigned int reserved)
{
    return ((const LIB_SO*)__GLOBAL->lib[LIB_ID_SO])->lib_so_create(so, &__STD_MEM, data_size, reserved);
}

__STATIC_INLINE void so_destroy(SO* so)
{
    ((const LIB_SO*)__GLOBAL->lib[LIB_ID_SO])->lib_so_destroy(so, &__STD_MEM);
}

//Remember: always re-fetch objects after so_allocate
__STATIC_INLINE HANDLE so_allocate(SO* so)
{
    return ((const LIB_SO*)__GLOBAL->lib[LIB_ID_SO])->lib_so_allocate(so, &__STD_MEM);
}

__STATIC_INLINE bool so_check_handle(SO* so, HANDLE handle)
{
    return ((const LIB_SO*)__GLOBAL->lib[LIB_ID_SO])->lib_so_check_handle(so, &__STD_MEM, handle);
}

__STATIC_INLINE void so_free(SO* so, HANDLE handle)
{
    ((const LIB_SO*)__GLOBAL->lib[LIB_ID_SO])->lib_so_free(so, &__STD_MEM, handle);
}

__STATIC_INLINE void* so_get(SO* so, HANDLE handle)
{
    return ((const LIB_SO*)__GLOBAL->lib[LIB_ID_SO])->lib_so_get(so, &__STD_MEM, handle);
}

__STATIC_INLINE HANDLE so_first(SO* so)
{
    return ((const LIB_SO*)__GLOBAL->lib[LIB_ID_SO])->lib_so_first(so, &__STD_MEM);
}

__STATIC_INLINE HANDLE so_next(SO* so, HANDLE prev)
{
    return ((const LIB_SO*)__GLOBAL->lib[LIB_ID_SO])->lib_so_next(so, &__STD_MEM, prev);
}

__STATIC_INLINE unsigned int so_count(SO* so)
{
    return ((const LIB_SO*)__GLOBAL->lib[LIB_ID_SO])->lib_so_count(so, &__STD_MEM);
}

#endif // SO_H
