/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2016, Alexey Kramarenko
    All rights reserved.
*/

#ifndef SO_H
#define SO_H

#include "array.h"

//defined public for less fragmentaion
typedef struct _SO {
    ARRAY* ar;
    unsigned int first_free;
} SO;

typedef struct {
    SO* (*lib_so_create)(SO*, unsigned int, unsigned int);
    void (*lib_so_destroy)(SO*);
    HANDLE (*lib_so_allocate)(SO*);
    bool (*lib_so_check_handle)(SO*, HANDLE);
    void (*lib_so_free)(SO*, HANDLE);
    void* (*lib_so_get)(SO*, HANDLE);
    HANDLE (*lib_so_first)(SO*);
    HANDLE (*lib_so_next)(SO*, HANDLE);
    unsigned int (*lib_so_count)(SO*);
} LIB_SO;

__STATIC_INLINE SO* so_create(SO* so, unsigned int data_size, unsigned int reserved)
{
    return ((const LIB_SO*)__GLOBAL->lib[LIB_ID_SO])->lib_so_create(so, data_size, reserved);
}

__STATIC_INLINE void so_destroy(SO* so)
{
    ((const LIB_SO*)__GLOBAL->lib[LIB_ID_SO])->lib_so_destroy(so);
}

//Remember: always re-fetch objects after so_allocate
__STATIC_INLINE HANDLE so_allocate(SO* so)
{
    return ((const LIB_SO*)__GLOBAL->lib[LIB_ID_SO])->lib_so_allocate(so);
}

__STATIC_INLINE bool so_check_handle(SO* so, HANDLE handle)
{
    return ((const LIB_SO*)__GLOBAL->lib[LIB_ID_SO])->lib_so_check_handle(so, handle);
}

__STATIC_INLINE void so_free(SO* so, HANDLE handle)
{
    ((const LIB_SO*)__GLOBAL->lib[LIB_ID_SO])->lib_so_free(so, handle);
}

__STATIC_INLINE void* so_get(SO* so, HANDLE handle)
{
    return ((const LIB_SO*)__GLOBAL->lib[LIB_ID_SO])->lib_so_get(so, handle);
}

__STATIC_INLINE HANDLE so_first(SO* so)
{
    return ((const LIB_SO*)__GLOBAL->lib[LIB_ID_SO])->lib_so_first(so);
}

__STATIC_INLINE HANDLE so_next(SO* so, HANDLE prev)
{
    return ((const LIB_SO*)__GLOBAL->lib[LIB_ID_SO])->lib_so_next(so, prev);
}

__STATIC_INLINE unsigned int so_count(SO* so)
{
    return ((const LIB_SO*)__GLOBAL->lib[LIB_ID_SO])->lib_so_count(so);
}

#endif // SO_H
