/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2016, Alexey Kramarenko
    All rights reserved.
*/

#include "stdlib.h"
#include "systime.h"

void* malloc(size_t size)
{
    return ((const LIB_STD*)__GLOBAL->lib[LIB_ID_STD])->pool_malloc(&__PROCESS->pool, size);
}

void* realloc(void* ptr, size_t size)
{
    return ((const LIB_STD*)__GLOBAL->lib[LIB_ID_STD])->pool_realloc(&__PROCESS->pool, ptr, size);
}

void free(void* ptr)
{
    ((const LIB_STD*)__GLOBAL->lib[LIB_ID_STD])->pool_free(&__PROCESS->pool, ptr);
}

unsigned int srand()
{
    unsigned int seed;
    SYSTIME uptime;
    seed = 0x30d02149;
    int i;
    for (i = 0; i < 0x80; ++i)
        seed ^=  *((unsigned int *)(__PROCESS + i));

    get_uptime(&uptime);
    seed ^= uptime.usec;
    return rand(&seed);
}

unsigned int rand(unsigned int* seed)
{
    SYSTIME uptime;
    get_uptime(&uptime);
    (*seed) = (*seed) * 0x1b8365e9 + 0x6071d;
    (*seed) ^= uptime.usec;
    return (*seed);
}

const STD_MEM __STD_MEM = {
    malloc,
    realloc,
    free
};
