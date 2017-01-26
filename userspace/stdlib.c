/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "stdlib.h"
#include "systime.h"
#include "svc.h"

void* malloc(size_t size)
{
    return ((const LIB_STD*)__GLOBAL->lib[LIB_ID_STD])->pool_malloc(&__PROCESS->pool, size, get_sp());
}

void* realloc(void* ptr, size_t size)
{
    return ((const LIB_STD*)__GLOBAL->lib[LIB_ID_STD])->pool_realloc(&__PROCESS->pool, ptr, size, get_sp());
}

void free(void* ptr)
{
    ((const LIB_STD*)__GLOBAL->lib[LIB_ID_STD])->pool_free(&__PROCESS->pool, ptr);
}

unsigned long atou(const char *const buf, int size)
{
    return ((const LIB_STD*)__GLOBAL->lib[LIB_ID_STD])->atou(buf, size);
}

int utoa(char* buf, unsigned long value, int radix, bool uppercase)
{
    return ((const LIB_STD*)__GLOBAL->lib[LIB_ID_STD])->utoa(buf, value, radix, uppercase);
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
