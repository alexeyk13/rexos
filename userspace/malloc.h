/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef MALLOC_H
#define MALLOC_H

#include "kernel_config.h"
#include "cc_macro.h"
#include "../lib/pool.h"
#include "core/core.h"
#include "thread.h"

/** \addtogroup memory dynamic memory management
    RExOS has embedded dynamic memory manager

    All data is allocated internally inside memory pools.

    If configured, pools are automatically checked for:
        - range checking - both up and bottom
        - consistency: every entry on debug supplied with "magic"

    Usage profiling for memory pools are also available (if enabled).

    Each process has own memory pool, allocated at creation time.

    Any data is aligned by WORD_SIZE() by default.
    \{
 */

/**
    \brief allocate memory in current thread's pool
    \param size: data size in bytes
    \retval pointer on success, NULL on out of memory conditiion
*/
__STATIC_INLINE void* malloc(int size)
{
    return pool_malloc(&__HEAP->pool, size);
}

/**
    \brief allocate memory in current thread's pool
    \param size: data size in bytes
    \retval pointer on success, NULL on out of memory conditiion
*/
__STATIC_INLINE void* realloc(void* ptr, int size)
{
    return pool_realloc(&__HEAP->pool, ptr, size);
}

/**
    \brief free memory in current thread's pool
    \details same memory pool as for malloc must be selected, or exception will be raised
    \param ptr: pointer to allocated data
    \retval none
*/
__STATIC_INLINE void free(void* ptr)
{
    pool_free(&__HEAP->pool, ptr);
}

/** \} */ // end of memory group

#if (KERNEL_PROFILING)
/** \addtogroup profiling profiling
    \{
 */

/**
    \brief display memory statistics
    \details display info about all global memory pools
    and thread local memory pool (if selected): total size,
    used size, free size in bytes and objects count. Also
    displays free blocks fragmentation.
    \retval none
*/
__STATIC_INLINE void mem_stat()
{
    sys_call(SYS_CALL_MEM_STAT, 0, 0, 0);
}

/** \} */ // end of profiling group
#endif //KERNEL_PROFILING


#endif // MALLOC_H
