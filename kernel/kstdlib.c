/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2016, Alexey Kramarenko
    All rights reserved.
*/

#include "kstdlib.h"
#include "karray.h"
#include <string.h>
#include "../userspace/process.h"
#include "../lib/lib_lib.h"
#include "../userspace/stdlib.h"
#include "../userspace/svc.h"

void* kmalloc_internal(size_t size)
{
    //TODO: support many pools
    return ((const LIB_STD*)__GLOBAL->lib[LIB_ID_STD])->pool_malloc(karray_at_internal(__KERNEL->pools, 0), size, get_sp());
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
    return ((const LIB_STD*)__GLOBAL->lib[LIB_ID_STD])->pool_realloc(karray_at_internal(__KERNEL->pools, 0), ptr, size, get_sp());
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
    ((const LIB_STD*)__GLOBAL->lib[LIB_ID_STD])->pool_free(karray_at_internal(__KERNEL->pools, 0), ptr);
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

//this once called function is used to solve chicken and egg problem - to setup pools array, we need malloc, to get malloc, we need pool
void* kmalloc_startup(size_t size)
{
    //this time it's not array itself, just pointer
    return ((const LIB_STD*)__GLOBAL->lib[LIB_ID_STD])->pool_malloc((POOL*)__KERNEL->pools, size, get_sp());
}

const STD_MEM __KSTD_MEM_STARTUP = {
    kmalloc_startup,
    NULL,
    NULL
};

void kstdlib_init()
{
    ARRAY* ar;
    POOL pool;

    pool_init(&pool, (void*)(SRAM_BASE + KERNEL_GLOBAL_SIZE + sizeof(KERNEL)));
    //Not array at this point, just single pool - we need something global
    __KERNEL->pools = (ARRAY*)&pool;
    //make sure error processing will be passed to valid pointer
    __GLOBAL->process = (PROCESS*)__KERNEL;
    __KERNEL->error = ERROR_OK;
    //Call lib directly
    ((const LIB_ARRAY*)__GLOBAL->lib[LIB_ID_ARRAY])->lib_array_create(&ar, &__KSTD_MEM_STARTUP, sizeof(POOL), 1);
    ((const LIB_ARRAY*)__GLOBAL->lib[LIB_ID_ARRAY])->lib_array_append(&ar, &__KSTD_MEM_STARTUP);
    __KERNEL->pools = ar;
    memcpy(karray_at_internal(ar, 0), &pool, sizeof(POOL));
}
