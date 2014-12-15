/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef STDLIB_H
#define STDLIB_H

#include "lib.h"
#include "svc.h"
#include "heap.h"

typedef struct {
    unsigned long (*atou)(const char *const, int);
    int (*utoa)(char*, unsigned long, int, bool);
    void (*pool_init)(POOL*, void*);
    void* (*pool_malloc)(POOL*, size_t);
    void* (*pool_realloc)(POOL*, void*, size_t);
    void (*pool_free)(POOL*, void*);
    bool (*pool_check)(POOL*, void*);
    void (*pool_stat)(POOL*, POOL_STAT*, void*);
    unsigned int (*srand)();
    unsigned int (*rand)(unsigned int* seed);
}LIB_STD;

/** \addtogroup stdlib embedded uStdlib
 */

/**
    \brief allocate memory in current process's pool
    \param size: data size in bytes
    \retval pointer on success, NULL on out of memory conditiion
*/
__STATIC_INLINE void* malloc(int size)
{
    return ((const LIB_STD*)__GLOBAL->lib[LIB_ID_STD])->pool_malloc(&__HEAP->pool, size);
}

/**
    \brief allocate memory in current process's pool
    \param size: data size in bytes
    \retval pointer on success, NULL on out of memory conditiion
*/
__STATIC_INLINE void* realloc(void* ptr, int size)
{
    return ((const LIB_STD*)__GLOBAL->lib[LIB_ID_STD])->pool_realloc(&__HEAP->pool, ptr, size);
}

/**
    \brief free memory in current process's pool
    \details same memory pool as for malloc must be selected, or exception will be raised
    \param ptr: pointer to allocated data
    \retval none
*/
__STATIC_INLINE void free(void* ptr)
{
    ((const LIB_STD*)__GLOBAL->lib[LIB_ID_STD])->pool_free(&__HEAP->pool, ptr);
}

/** \addtogroup lib_printf embedded stdio
    \{
 */

/**
    \brief string to unsigned int
    \param buf: untruncated strings
    \param size: size of buf in bytes
    \retval integer result. 0 on error
*/
__STATIC_INLINE unsigned long atou(const char *const buf, int size)
{
    return ((const LIB_STD*)__GLOBAL->lib[LIB_ID_STD])->atou(buf, size);
}

/**
    \brief unsigned int to string. Used internally in printf
    \param buf: out buffer
    \param value: in value
    \param radix: radix of original value
    \param uppercase: upper/lower case of result for radix 16
    \retval size of resulting string. 0 on error
*/
__STATIC_INLINE int utoa(char* buf, unsigned long value, int radix, bool uppercase)
{
    return ((const LIB_STD*)__GLOBAL->lib[LIB_ID_STD])->utoa(buf, value, radix, uppercase);
}

/**
    \brief initialize random seed
    \retval seed
*/
__STATIC_INLINE unsigned int srand()
{
    return ((const LIB_STD*)__GLOBAL->lib[LIB_ID_STD])->srand();
}

/**
    \brief get next random value
    \param seed: pointer to initialized before seed
    \retval random value
*/
__STATIC_INLINE unsigned int rand(unsigned int* seed)
{
    return ((const LIB_STD*)__GLOBAL->lib[LIB_ID_STD])->rand(seed);
}

/** \} */ // end of stdlib group

#endif // STDLIB_H
