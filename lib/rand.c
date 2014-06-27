/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "rand.h"
#include "../userspace/timer.h"
#include "../userspace/process.h"

unsigned int srand()
{
    unsigned int seed;
    TIME uptime;
    seed = 0x30d02149;
	int i;
    for (i = 0; i < 0x80; ++i)
        seed ^=  *((unsigned int *)(__HEAP + i));

    get_uptime(&uptime);
    seed ^= uptime.usec;
    return rand(&seed);
}

unsigned int rand(unsigned int* seed)
{
    TIME uptime;
    get_uptime(&uptime);
    (*seed) = (*seed) * 0x1b8365e9 + 0x6071d;
    (*seed) ^= uptime.usec;
    return (*seed);
}
