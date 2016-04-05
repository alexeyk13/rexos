/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2016, Alexey Kramarenko
    All rights reserved.
*/

#ifndef KSTDLIB_H
#define KSTDLIB_H

#include "kernel.h"

/** \addtogroup memory kernel memory management
    \{
 */

/**
    \brief allocate memory in system pool. No LIB wrapper
    \param size: data size in bytes
    \retval pointer on success, NULL on out of memory conditiion
*/
void* kmalloc_internal(int size);

/**
    \brief allocate memory in system pool
    \param size: data size in bytes
    \retval pointer on success, NULL on out of memory conditiion
*/
void* kmalloc(int size);

/**
    \brief re-allocate memory in system pool. No LIB wrapper
    \param ptr: old pointer
    \param size: new size in bytes
    \retval pointer on success, NULL on out of memory conditiion
*/
void* krealloc_internal(void* ptr, int size);

/**
    \brief re-allocate memory in system pool
    \param ptr: old pointer
    \param size: new size in bytes
    \retval pointer on success, NULL on out of memory conditiion
*/
void* krealloc(void* ptr, int size);

/**
    \brief free memory in system pool. No LIB wrapper
    \param ptr: pointer to allocated data
    \retval none
*/
void kfree_internal(void *ptr);

/**
    \brief free memory in system pool
    \param ptr: pointer to allocated data
    \retval none
*/
void kfree(void *ptr);

/** \} */ // end of memory group

#endif // KSTDLIB_H
