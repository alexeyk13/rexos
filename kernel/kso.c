/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "kso.h"
#include "kstdlib.h"
#include "../lib/lib_lib.h"

SO* kso_create(SO* so, unsigned int data_size, unsigned int reserved)
{
    SO* res;
    res = ((const LIB_SO*)__GLOBAL->lib[LIB_ID_SO])->lib_so_create(so, &__KSTD_MEM, data_size, reserved);
    return res;
}

void kso_destroy(SO* so)
{
    ((const LIB_SO*)__GLOBAL->lib[LIB_ID_SO])->lib_so_destroy(so, &__KSTD_MEM);
}

HANDLE kso_allocate(SO* so)
{
    HANDLE res;
    res = ((const LIB_SO*)__GLOBAL->lib[LIB_ID_SO])->lib_so_allocate(so, &__KSTD_MEM);
    return res;
}

bool kso_check_handle(SO* so, HANDLE handle)
{
    bool res;
    res = ((const LIB_SO*)__GLOBAL->lib[LIB_ID_SO])->lib_so_check_handle(so, &__KSTD_MEM, handle);
    return res;
}

void kso_free(SO* so, HANDLE handle)
{
    ((const LIB_SO*)__GLOBAL->lib[LIB_ID_SO])->lib_so_free(so, &__KSTD_MEM, handle);
}

void* kso_get(SO* so, HANDLE handle)
{
    void* res;
    res = ((const LIB_SO*)__GLOBAL->lib[LIB_ID_SO])->lib_so_get(so, &__KSTD_MEM, handle);
    return res;
}

HANDLE kso_first(SO* so)
{
    HANDLE res;
    res = ((const LIB_SO*)__GLOBAL->lib[LIB_ID_SO])->lib_so_first(so, &__KSTD_MEM);
    return res;
}

HANDLE kso_next(SO* so, HANDLE prev)
{
    HANDLE res;
    res = ((const LIB_SO*)__GLOBAL->lib[LIB_ID_SO])->lib_so_next(so, &__KSTD_MEM, prev);
    return res;
}

unsigned int kso_count(SO* so)
{
    unsigned int res;
    res = ((const LIB_SO*)__GLOBAL->lib[LIB_ID_SO])->lib_so_count(so, &__KSTD_MEM);
    return res;
}
