/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "dbg.h"
#include "../userspace/core/sys_calls.h"
#include "../userspace/error.h"
#include "kernel.h"

void printf_handler(void* param, const char *const buf, unsigned int size)
{
    dbg_write(buf, size);
}

static inline unsigned int stack_used_max(unsigned int top, unsigned int cur)
{
    unsigned int i;
    unsigned int last = cur;
    for (i = cur - sizeof(unsigned int); i >= top; i -= 4)
        if (*(unsigned int*)i != MAGIC_UNINITIALIZED)
            last = i;
    return last;
}

