/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "kheap.h"
#include "kstdlib.h"
#include "kprocess.h"
#include "../userspace/stdlib.h"
#include "dbg.h"

typedef struct {
    MAGIC;
    HANDLE owner;
    unsigned int granted_count;
    int kill_flag;
    POOL pool;
    void* virtual_sp;
    //data is follow
} KHEAP;

HANDLE kheap_create(unsigned int size)
{
    KHEAP* kheap = kmalloc(size + sizeof(KHEAP));
    if (kheap == NULL)
        return INVALID_HANDLE;
    DO_MAGIC(kheap, MAGIC_HEAP);
    kheap->granted_count = 0;
    kheap->owner = kprocess_get_current();
    kheap->kill_flag = false;
    kheap->virtual_sp = (void*)((unsigned int)(kheap) + sizeof(KHEAP) + size);
    pool_init(&kheap->pool, ((void*)((unsigned int)(kheap) + sizeof(KHEAP))));
    return (HANDLE)kheap;
}

void kheap_destroy(HANDLE h)
{
    KHEAP* kheap = (KHEAP*)h;
    CHECK_MAGIC(kheap, MAGIC_HEAP);
    if (kheap->owner != kprocess_get_current())
    {
        error(ERROR_ACCESS_DENIED);
        return;
    }
    if (kheap->granted_count)
        kheap->kill_flag = true;
    else
    {
        CLEAR_MAGIC(kheap);
        kfree(kheap);
    }
}

void* kheap_malloc(HANDLE h, unsigned int size)
{
    void* ptr;
    KHEAP* kheap = (KHEAP*)h;
    CHECK_MAGIC(kheap, MAGIC_HEAP);

    ptr = pool_malloc(&kheap->pool, size + sizeof(KHEAP*), kheap->virtual_sp);
    if (ptr == NULL)
        return NULL;
    *((KHEAP**)ptr) = kheap;
    if (kprocess_get_current() != kheap->owner)
        ++kheap->granted_count;
    return (void*)((unsigned int)ptr + sizeof(KHEAP*));
}

static void kheap_return(KHEAP* kheap)
{
    --kheap->granted_count;
    if (kheap->kill_flag && kheap->granted_count == 0)
    {
        CLEAR_MAGIC(kheap);
        kfree(kheap);
    }
}

void kheap_free(void* ptr)
{
    KHEAP* kheap;
    if (ptr == NULL)
        return;
    kheap = *((KHEAP**)((unsigned int)ptr - sizeof(KHEAP*)));
    CHECK_MAGIC(kheap, MAGIC_HEAP);

    pool_free(&kheap->pool, (void*)((unsigned int)ptr - sizeof(KHEAP*)));

    if ((kprocess_get_current() != kheap->owner) && kheap->granted_count)
        kheap_return(kheap);
}

void kheap_send(void* ptr, unsigned int process)
{
    KHEAP* kheap;
    HANDLE current_process;
    if (ptr == NULL)
        return;
    kheap = *((KHEAP**)((unsigned int)ptr - sizeof(KHEAP*)));
    CHECK_MAGIC(kheap, MAGIC_HEAP);
    current_process = kprocess_get_current();
    if (current_process == process)
        return;
    if (process == kheap->owner)
        kheap_return(kheap);
    else
        ++kheap->granted_count;
}

#if (KERNEL_PROFILING)
bool kheap_check(HANDLE h)
{
    KHEAP* kheap = (KHEAP*)h;
    CHECK_MAGIC(kheap, MAGIC_HEAP);

    return pool_check(&kheap->pool, kheap->virtual_sp);
}
#endif //KERNEL_PROFILING
