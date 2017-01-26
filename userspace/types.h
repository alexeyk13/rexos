/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef TYPES_H
#define TYPES_H

#ifndef NULL
#define NULL								0
#endif

#define HANDLE								unsigned int
#define INVALID_HANDLE                      ((unsigned int)-1)
#define ANY_HANDLE                          ((unsigned int)-2)
#define KERNEL_HANDLE                       ((unsigned int)-3)

#define INFINITE							0x0

#define WORD_SIZE							sizeof(unsigned int)
#define WORD_SIZE_BITS                      (WORD_SIZE * 8)

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>

typedef struct {
    void* free_slot;
    void* first_slot;
    void* last_slot;
} POOL;

typedef struct {
    unsigned int free_slots;
    unsigned int used_slots;
    unsigned int free;
    unsigned int used;
    unsigned int largest_free;
} POOL_STAT;

#endif // TYPES_H
