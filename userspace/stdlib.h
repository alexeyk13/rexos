/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef STDLIB_H
#define STDLIB_H

#include "lib.h"
#include "types.h"
#include "process.h"

typedef struct {
    unsigned long (*atou)(const char *const, int);
    int (*utoa)(char*, unsigned long, int, bool);
    void (*pool_init)(POOL*, void*);
    void* (*pool_malloc)(POOL*, size_t, void*);
    size_t(*pool_slot_size)(POOL*, void*);
    void* (*pool_realloc)(POOL*, void*, size_t, void*);
    void (*pool_free)(POOL*, void*);
    bool (*pool_check)(POOL*, void*);
    void (*pool_stat)(POOL*, POOL_STAT*, void*);
} LIB_STD;

typedef struct {
    void* (*fn_malloc)(size_t);
    void* (*fn_realloc)(void*, size_t);
    void (*fn_free)(void*);
} STD_MEM;

extern const STD_MEM __STD_MEM;

/** \addtogroup stdlib embedded uStdlib
 */

/**
    \brief allocate memory in current process's pool
    \param size: data size in bytes
    \retval pointer on success, NULL on out of memory conditiion
*/
void* malloc(size_t size);

/**
    \brief allocate memory in current process's pool
    \param size: data size in bytes
    \retval pointer on success, NULL on out of memory conditiion
*/
void* realloc(void* ptr, size_t size);

/**
    \brief free memory in current process's pool
    \details same memory pool as for malloc must be selected, or exception will be raised
    \param ptr: pointer to allocated data
    \retval none
*/
void free(void* ptr);

/** \addtogroup lib_printf embedded stdio
    \{
 */

/**
    \brief string to unsigned int
    \param buf: untruncated strings
    \param size: size of buf in bytes
    \retval integer result. 0 on error
*/
unsigned long atou(const char *const buf, int size);

/**
    \brief unsigned int to string. Used internally in printf
    \param buf: out buffer
    \param value: in value
    \param radix: radix of original value
    \param uppercase: upper/lower case of result for radix 16
    \retval size of resulting string. 0 on error
*/
int utoa(char* buf, unsigned long value, int radix, bool uppercase);

/**
    \brief initialize random seed
    \retval seed
*/
unsigned int srand();

/**
    \brief get next random value
    \param seed: pointer to initialized before seed
    \retval random value
*/
unsigned int rand(unsigned int* seed);

/** \} */ // end of stdlib group

#endif // STDLIB_H
