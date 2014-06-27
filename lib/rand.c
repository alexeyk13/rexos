/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "rand.h"
#include "delay.h"
#include "../lib/time.h"
#include "hw_config.h"
#include "memmap.h"
#include "../kernel/core/core_kernel.h"

#ifndef HW_RAND

void srand()
{
    __KERNEL->rand = 0x30d02149;
	int i;
	for (i = 0; i < 0x100; ++i)
        __KERNEL->rand ^=  *((unsigned int *)(__HEAP + i));
    __KERNEL->rand ^= get_uptime();
    rand();
}

unsigned int rand()
{
    __KERNEL->rand = __KERNEL->rand * 0x1b8365e9 + 0x6071d;
    __KERNEL->rand ^= get_uptime();
    return __KERNEL->rand;
}

#endif
