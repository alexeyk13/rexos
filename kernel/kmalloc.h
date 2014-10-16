/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef KMALLOC_H
#define KMALLOC_H

#include "../userspace/cc_macro.h"
#include "kernel.h"

/** \addtogroup memory kernel memory management
    \{
 */

/**
    \brief allocate memory in system pool
    \param size: data size in bytes
    \retval pointer on success, NULL on out of memory conditiion
*/
__STATIC_INLINE void* kmalloc(int size)
{
    return __GLOBAL->lib->pool_malloc(&__KERNEL->paged, size);
}

/**
    \brief re-allocate memory in system pool
    \param ptr: old pointer
    \param size: new size in bytes
    \retval pointer on success, NULL on out of memory conditiion
*/
__STATIC_INLINE void* krealloc(void* ptr, int size)
{
    return __GLOBAL->lib->pool_realloc(&__KERNEL->paged, ptr, size);
}

/**
    \brief free memory in system pool
    \param ptr: pointer to allocated data
    \retval none
*/
__STATIC_INLINE void kfree(void *ptr)
{
    __GLOBAL->lib->pool_free(&__KERNEL->paged, ptr);
}

/** \} */ // end of memory group

#endif // KMALLOC_H
