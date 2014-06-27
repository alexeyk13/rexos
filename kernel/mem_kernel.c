/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "mem_kernel.h"
#include "sys_calls.h"
#include "memmap.h"
#include "core/core_kernel.h"
#include "../userspace/error.h"
#include "dbg.h"
#include "mutex.h"
#include "kernel_config.h"
#include "thread_kernel.h"
#if (KERNEL_PROFILING)
#include "string.h"
#endif //KERNEL_PROFILING
#include "../lib/pool.h"

/** \addtogroup memory dynamic memory management
    \{
 */

/**
    \brief allocate memory in system pool
    \details cannot be called from USER context
    \param size: data size in bytes
    \retval pointer on success, NULL on out of memory conditiion
*/
void* sys_alloc(int size)
{
    CHECK_CONTEXT(IRQ_CONTEXT | SUPERVISOR_CONTEXT);
    void* ptr = NULL;
    CRITICAL_ENTER;
    ptr = pool_malloc(&__KERNEL->pool, size);
    CRITICAL_LEAVE;
    return ptr;
}

/**
    \brief free memory in system pool
    \details cannot be called from USER context
    \param ptr: pointer to allocated data
    \retval none
*/
void sys_free(void *ptr)
{
    CHECK_CONTEXT(IRQ_CONTEXT | SUPERVISOR_CONTEXT);
    CRITICAL_ENTER;
    pool_free(&__KERNEL->pool, ptr);
    CRITICAL_LEAVE;
}

/** \} */ // end of event group

void* stack_alloc(int size)
{
    CHECK_CONTEXT(IRQ_CONTEXT | SUPERVISOR_CONTEXT);
    return pool_malloc(&__KERNEL->paged, size);
}

void stack_free(void* ptr)
{
    CHECK_CONTEXT(IRQ_CONTEXT | SUPERVISOR_CONTEXT);
    pool_free(&__KERNEL->paged, ptr);
}

#if (KERNEL_PROFILING)
void print_value(unsigned int value)
{
    if (value < 1024)
        printf(" %3d", value);
    else if (value / 1024 < 1024)
        printf("%3dK", value / 1024);
    else
        printf("%3dM", value / 1024 / 1024);
}

/*void print_pool_stat(MEM_POOL* pool)
{
    int i;
    MEM_POOL_STAT stat;
    mem_pool_stat(pool, &stat);
    printf("%s ", pool->name);
    for (i = strlen(pool->name); i <= 16; ++i)
        printf(" ");
    print_value(pool->size);
    printf("   ");
    print_value(stat.total_used);
    printf("(%d)   ", stat.used_blocks_count);
    print_value(stat.total_free);
    printf("(%d)\n\r", stat.free_blocks_count);
}
*/
static inline void svc_mem_stat()
{
    //just to see all text right
    CRITICAL_ENTER;
    printf("name              size     used     free  \n\r");
    printf("---------------------------------------------\n\r");
//    print_pool_stat(&__KERNEL->sys_pool);
//    print_pool_stat(&__KERNEL->stack_pool);
//    print_pool_stat(&__KERNEL->data_pool);
    CRITICAL_LEAVE;
}

#endif //KERNEL_PROFILING

void svc_mem_handler(unsigned int num)
{
    switch (num)
    {
#if (KERNEL_PROFILING)
    case SYS_CALL_MEM_STAT:
        svc_mem_stat();
        break;
#endif //KERNEL_PROFILING
    default:
        error(ERROR_INVALID_SYS_CALL);
    }
}
