/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "karray.h"
#include "kstdlib.h"
#include "../lib/lib_lib.h"
#include "../userspace/process.h"

ARRAY* karray_create(ARRAY** ar, unsigned int data_size, unsigned int reserved)
{
    ARRAY* res;
    res = ((const LIB_ARRAY*)__GLOBAL->lib[LIB_ID_ARRAY])->lib_array_create(ar, &__KSTD_MEM, data_size, reserved);
    return res;
}

void karray_destroy(ARRAY** ar)
{
    ((const LIB_ARRAY*)__GLOBAL->lib[LIB_ID_ARRAY])->lib_array_destroy(ar, &__KSTD_MEM);
}

void* karray_at_internal(ARRAY* ar, unsigned int index)
{
    return ((const LIB_ARRAY*)__GLOBAL->lib[LIB_ID_ARRAY])->lib_array_at(ar, &__KSTD_MEM, index);
}

void* karray_at(ARRAY* ar, unsigned int index)
{
    void* res;
    res = karray_at_internal(ar, index);
    return res;
}

unsigned int karray_size_internal(ARRAY* ar)
{
    return ((const LIB_ARRAY*)__GLOBAL->lib[LIB_ID_ARRAY])->lib_array_size(ar, &__KSTD_MEM);
}

unsigned int karray_size(ARRAY* ar)
{
    unsigned int res;
    res = karray_size_internal(ar);
    return res;
}

void* karray_append(ARRAY** ar)
{
    void* res;
    res = ((const LIB_ARRAY*)__GLOBAL->lib[LIB_ID_ARRAY])->lib_array_append(ar, &__KSTD_MEM);
    return res;
}

void* karray_insert(ARRAY** ar, unsigned int index)
{
    void* res;
    res = ((const LIB_ARRAY*)__GLOBAL->lib[LIB_ID_ARRAY])->lib_array_insert(ar, &__KSTD_MEM, index);
    return res;
}

ARRAY* karray_clear(ARRAY** ar)
{
    void* res;
    res = ((const LIB_ARRAY*)__GLOBAL->lib[LIB_ID_ARRAY])->lib_array_clear(ar, &__KSTD_MEM);
    return res;
}

ARRAY* karray_remove(ARRAY** ar, unsigned int index)
{
    void* res;
    res = ((const LIB_ARRAY*)__GLOBAL->lib[LIB_ID_ARRAY])->lib_array_remove(ar, &__KSTD_MEM, index);
    return res;
}

ARRAY* karray_squeeze(ARRAY** ar)
{
    void* res;
    res = ((const LIB_ARRAY*)__GLOBAL->lib[LIB_ID_ARRAY])->lib_array_squeeze(ar, &__KSTD_MEM);
    return res;
}
