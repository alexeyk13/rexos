/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2016, Alexey Kramarenko
    All rights reserved.
*/

#include "kstdlib.h"
#include "../userspace/process.h"
#include "../lib/lib_lib.h"
#include "../userspace/stdlib.h"
#include "../userspace/svc.h"

void* kmalloc_internal(size_t size)
{
    return ((const LIB_STD*)__GLOBAL->lib[LIB_ID_STD])->pool_malloc(&__KERNEL->paged, size, get_sp());
}

void* kmalloc(size_t size)
{
    void* res;
    LIB_ENTER
    res = kmalloc_internal(size);
    LIB_EXIT
    return res;
}

void* krealloc_internal(void* ptr, size_t size)
{
    return ((const LIB_STD*)__GLOBAL->lib[LIB_ID_STD])->pool_realloc(&__KERNEL->paged, ptr, size, get_sp());
}

void* krealloc(void* ptr, size_t size)
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

const STD_MEM __KSTD_MEM = {
    kmalloc_internal,
    krealloc_internal,
    kfree_internal
};
