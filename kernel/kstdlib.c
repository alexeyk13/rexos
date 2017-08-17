/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "kstdlib.h"
#include "karray.h"
#include "kprocess.h"
#include <string.h>
#include "../userspace/process.h"
#include "../lib/lib_lib.h"
#include "../userspace/stdlib.h"
#include "../userspace/svc.h"

KPOOL* kpool_at(unsigned int idx)
{
    return karray_at_internal(__KERNEL->pools, idx);
}

void kpool_stat(unsigned int idx, POOL_STAT* stat)
{
    KPOOL* kpool;
    kpool = kpool_at(idx);
    ((const LIB_STD*)__GLOBAL->lib[LIB_ID_STD])->pool_stat(&kpool->pool, stat, idx == 0 ? get_sp() : (void*)kpool->base + kpool->size);
}

static int kpool_idx(void* ptr)
{
    int idx;
    KPOOL* kpool;
    for (idx = 0; idx < karray_size_internal(__KERNEL->pools); ++idx)
    {
        kpool = kpool_at(idx);
        if (kpool->base < (unsigned int)ptr && kpool->base + kpool->size > (unsigned int)ptr)
            return idx;
    }
    return -1;
}

void* kmalloc_internal(size_t size)
{
    int idx;
    void* res;
    KPOOL* kpool;
    for (idx = karray_size_internal(__KERNEL->pools) - 1; idx >= 0; --idx)
    {
        kpool = kpool_at(idx);
        res = ((const LIB_STD*)__GLOBAL->lib[LIB_ID_STD])->pool_malloc(&kpool_at(idx)->pool, size, idx == 0 ? get_sp() : (void*)(kpool->base + kpool->size));
        if (res)
            return res;
    }
    return NULL;
}

void* kmalloc(size_t size)
{
    void* res;
    disable_interrupts();
    res = kmalloc_internal(size);
    enable_interrupts();
    return res;
}

void* krealloc_internal(void* ptr, size_t size)
{
    void* res;
    KPOOL* kpool;
    int idx, idx_cur;
    if (ptr == NULL)
        return kmalloc_internal(size);
    idx_cur = kpool_idx(ptr);

    if (idx_cur < 0)
        return NULL;
    kpool = kpool_at(idx_cur);
    res = ((const LIB_STD*)__GLOBAL->lib[LIB_ID_STD])->pool_realloc(&kpool->pool, ptr, size, idx_cur == 0 ? get_sp() : (void*)(kpool->base + kpool->size));
    if (res != NULL)
        return res;

    for (idx = karray_size_internal(__KERNEL->pools) - 1; idx >= 0; --idx)
    {
        if (idx == idx_cur)
            continue;
        kpool = kpool_at(idx);
        res = ((const LIB_STD*)__GLOBAL->lib[LIB_ID_STD])->pool_malloc(&kpool_at(idx)->pool, size, idx == 0 ? get_sp() : (void*)(kpool->base + kpool->size));
        if (res != NULL)
        {
            memcpy(res, ptr, ((const LIB_STD*)__GLOBAL->lib[LIB_ID_STD])->pool_slot_size(&kpool->pool, ptr));
            ((const LIB_STD*)__GLOBAL->lib[LIB_ID_STD])->pool_free(&kpool_at(idx_cur)->pool, ptr);
            return res;
        }
    }
    return NULL;
}

void* krealloc(void* ptr, size_t size)
{
    void* res;
    disable_interrupts();
    res = krealloc_internal(ptr, size);
    enable_interrupts();
    return res;
}

void kfree_internal(void *ptr)
{
    int idx_cur;
    if (ptr == NULL)
        return;
    idx_cur = kpool_idx(ptr);
    if (idx_cur < 0)
        return;
    ((const LIB_STD*)__GLOBAL->lib[LIB_ID_STD])->pool_free(&kpool_at(idx_cur)->pool, ptr);
}

void kfree(void *ptr)
{
    disable_interrupts();
    kfree_internal(ptr);
    enable_interrupts();
}

const STD_MEM __KSTD_MEM = {
    kmalloc_internal,
    krealloc_internal,
    kfree_internal
};

//this once called function is used to solve chicken and egg problem - to setup pools array, we need malloc, to get malloc, we need pool
void* kmalloc_startup(size_t size)
{
    //at this point it's not array itself, just pointer
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
    KPOOL* kpool0;

    pool_init(&pool, (void*)(SRAM_BASE + KERNEL_GLOBAL_SIZE + sizeof(KERNEL)));
    //Not array at this point, just single pool - we need something global
    __KERNEL->pools = (ARRAY*)&pool;
    //make sure error processing will be passed to valid pointer
    __GLOBAL->process = (PROCESS*)__KERNEL;
    //Call lib directly
    ((const LIB_ARRAY*)__GLOBAL->lib[LIB_ID_ARRAY])->lib_array_create(&ar, &__KSTD_MEM_STARTUP, sizeof(KPOOL), 1);
    kpool0 = ((const LIB_ARRAY*)__GLOBAL->lib[LIB_ID_ARRAY])->lib_array_append(&ar, &__KSTD_MEM_STARTUP);
    __KERNEL->pools = ar;
    memcpy(&kpool0->pool, &pool, sizeof(POOL));
    kpool0->base = SRAM_BASE + KERNEL_GLOBAL_SIZE + sizeof(KERNEL);
    kpool0->size = SRAM_SIZE - KERNEL_GLOBAL_SIZE - sizeof(KERNEL);
}

void kstdlib_add_pool(unsigned int base, unsigned int size)
{
    int i;
    KPOOL* kpool;
    //check not interlaps with configured pools
    for (i = 0; i < karray_size_internal(__KERNEL->pools); ++i)
    {
        kpool = kpool_at(i);
        if ((base >= kpool->base) && (base + size <= kpool->base + kpool->size))
        {
            error(ERROR_ALREADY_CONFIGURED);
            return;
        }
    }
    disable_interrupts();
    kpool = karray_append(&__KERNEL->pools);
    kpool->base = base;
    kpool->size = size;
    pool_init(&kpool->pool, (void*)base);
    enable_interrupts();
}
