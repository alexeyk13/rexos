/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2016, Alexey Kramarenko
    All rights reserved.
*/

#include "kstdlib.h"
#include "../userspace/process.h"
#include "../lib/lib_lib.h"
#include "../userspace/stdlib.h"

void* kmalloc_internal(int size)
{
    return ((const LIB_STD*)__GLOBAL->lib[LIB_ID_STD])->pool_malloc(&__KERNEL->paged, size);
}

void* kmalloc(int size)
{
    void* res;
    LIB_ENTER
    res = kmalloc_internal(size);
    LIB_EXIT
    return res;
}

void* krealloc_internal(void* ptr, int size)
{
    return ((const LIB_STD*)__GLOBAL->lib[LIB_ID_STD])->pool_realloc(&__KERNEL->paged, ptr, size);
}

void* krealloc(void* ptr, int size)
{
    void* res;
    LIB_ENTER
    res = krealloc_internal(ptr, size);
    LIB_EXIT
    return res;
}

void kfree_internal(void *ptr)
{
    ((const LIB_STD*)__GLOBAL->lib[LIB_ID_STD])->pool_free(&__KERNEL->paged, ptr);
}

void kfree(void *ptr)
{
    LIB_ENTER
    kfree_internal(ptr);
    LIB_EXIT
}
