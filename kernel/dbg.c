/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "dbg.h"
#include "../userspace/error.h"
#include "kernel.h"
#include <stdarg.h>
#include "../lib/printf.h"

static inline unsigned int stack_used_max(unsigned int top, unsigned int cur)
{
    unsigned int i;
    unsigned int last = cur;
    for (i = cur - sizeof(unsigned int); i >= top; i -= 4)
        if (*(unsigned int*)i != MAGIC_UNINITIALIZED)
            last = i;
    return last;
}

void printk(char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    format(fmt, va, __KERNEL->stdout, __KERNEL->stdout_param);
    va_end(va);
}

void dump(unsigned int addr, unsigned int size)
{
    printk("memory dump 0x%08x-0x%08x\n\r", addr, addr + size);
    unsigned int i = 0;
    for (i = 0; i < size; ++i)
    {
        if ((i % 0x10) == 0)
            printk("0x%08x: ", addr + i);
        printk("%02X ", ((unsigned char*)addr)[i]);
        if ((i % 0x10) == 0xf)
            printk("\n\r");
    }
    if (size % 0x10)
        printk("\n\r");
}

