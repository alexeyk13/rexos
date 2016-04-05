/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2016, Alexey Kramarenko
    All rights reserved.
*/

#include "karray.h"
#include "kstdlib.h"
#include "../userspace/process.h"

ARRAY* karray_create(ARRAY** ar, unsigned int data_size, unsigned int reserved)
{
    return ((const LIB_ARRAY*)__GLOBAL->lib[LIB_ID_ARRAY])->lib_array_create(ar, &__KSTD_MEM, data_size, reserved);
}

void karray_destroy(ARRAY** ar)
{
    ((const LIB_ARRAY*)__GLOBAL->lib[LIB_ID_ARRAY])->lib_array_destroy(ar, &__KSTD_MEM);
}

void* karray_at(ARRAY* ar, unsigned int index)
{
    return ((const LIB_ARRAY*)__GLOBAL->lib[LIB_ID_ARRAY])->lib_array_at(ar, &__KSTD_MEM, index);
}

unsigned int karray_size(ARRAY* ar)
{
    return ((const LIB_ARRAY*)__GLOBAL->lib[LIB_ID_ARRAY])->lib_array_size(ar, &__KSTD_MEM);
}

void* karray_append(ARRAY** ar)
{
    return ((const LIB_ARRAY*)__GLOBAL->lib[LIB_ID_ARRAY])->lib_array_append(ar, &__KSTD_MEM);
}

void* karray_insert(ARRAY** ar, unsigned int index)
{
    return ((const LIB_ARRAY*)__GLOBAL->lib[LIB_ID_ARRAY])->lib_array_insert(ar, &__KSTD_MEM, index);
}

ARRAY* karray_clear(ARRAY** ar)
{
    return ((const LIB_ARRAY*)__GLOBAL->lib[LIB_ID_ARRAY])->lib_array_clear(ar, &__KSTD_MEM);
}

ARRAY* karray_remove(ARRAY** ar, unsigned int index)
{
    return ((const LIB_ARRAY*)__GLOBAL->lib[LIB_ID_ARRAY])->lib_array_remove(ar, &__KSTD_MEM, index);
}

ARRAY* karray_squeeze(ARRAY** ar)
{
    return ((const LIB_ARRAY*)__GLOBAL->lib[LIB_ID_ARRAY])->lib_array_squeeze(ar, &__KSTD_MEM);
}
