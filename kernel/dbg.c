/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "dbg.h"
#include "../userspace/error.h"
#include "kernel.h"
#include "../userspace/stdio.h"

void printk(const char *const fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    format(fmt, va, __KERNEL->stdout, __KERNEL->stdout_param);
    va_end(va);
}

void dump(unsigned int addr, unsigned int size)
{
    printk("memory dump 0x%08x-0x%08x\n", addr, addr + size);
    unsigned int i = 0;
    for (i = 0; i < size; ++i)
    {
        if ((i % 0x10) == 0)
            printk("0x%08x: ", addr + i);
        printk("%02X ", ((unsigned char*)addr)[i]);
        if ((i % 0x10) == 0xf)
            printk("\n");
    }
    if (size % 0x10)
        printk("\n");
}

