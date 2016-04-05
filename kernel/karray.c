/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2016, Alexey Kramarenko
    All rights reserved.
*/

#include "karray.h"
#include "kstdlib.h"
#include "../lib/lib_lib.h"
#include "../userspace/process.h"

ARRAY* karray_create(ARRAY** ar, unsigned int data_size, unsigned int reserved)
{
    ARRAY* res;
    LIB_ENTER
    res = ((const LIB_ARRAY*)__GLOBAL->lib[LIB_ID_ARRAY])->lib_array_create(ar, &__KSTD_MEM, data_size, reserved);
    LIB_EXIT
    return res;
}

void karray_destroy(ARRAY** ar)
{
    LIB_ENTER
    ((const LIB_ARRAY*)__GLOBAL->lib[LIB_ID_ARRAY])->lib_array_destroy(ar, &__KSTD_MEM);
    LIB_EXIT
}

void* karray_at(ARRAY* ar, unsigned int index)
{
    void* res;
    LIB_ENTER
    res = ((const LIB_ARRAY*)__GLOBAL->lib[LIB_ID_ARRAY])->lib_array_at(ar, &__KSTD_MEM, index);
    LIB_EXIT
    return res;
}

unsigned int karray_size(ARRAY* ar)
{
    unsigned int res;
    LIB_ENTER
    res = ((const LIB_ARRAY*)__GLOBAL->lib[LIB_ID_ARRAY])->lib_array_size(ar, &__KSTD_MEM);
    LIB_EXIT
    return res;
}

void* karray_append(ARRAY** ar)
{
    void* res;
    LIB_ENTER
    res = ((const LIB_ARRAY*)__GLOBAL->lib[LIB_ID_ARRAY])->lib_array_append(ar, &__KSTD_MEM);
    LIB_EXIT
    return res;
}

void* karray_insert(ARRAY** ar, unsigned int index)
{
    void* res;
    LIB_ENTER
    res = ((const LIB_ARRAY*)__GLOBAL->lib[LIB_ID_ARRAY])->lib_array_insert(ar, &__KSTD_MEM, index);
    LIB_EXIT
    return res;
}

ARRAY* karray_clear(ARRAY** ar)
{
    void* res;
    LIB_ENTER
    res = ((const LIB_ARRAY*)__GLOBAL->lib[LIB_ID_ARRAY])->lib_array_clear(ar, &__KSTD_MEM);
    LIB_EXIT
    return res;
}

ARRAY* karray_remove(ARRAY** ar, unsigned int index)
{
    void* res;
    LIB_ENTER
    res = ((const LIB_ARRAY*)__GLOBAL->lib[LIB_ID_ARRAY])->lib_array_remove(ar, &__KSTD_MEM, index);
    LIB_EXIT
    return res;
}

ARRAY* karray_squeeze(ARRAY** ar)
{
    void* res;
    LIB_ENTER
    res = ((const LIB_ARRAY*)__GLOBAL->lib[LIB_ID_ARRAY])->lib_array_squeeze(ar, &__KSTD_MEM);
    LIB_EXIT
    return res;
}
