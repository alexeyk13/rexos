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

void svc_dbg_handler(unsigned int num, unsigned int param1, unsigned int param2)
{
    CHECK_CONTEXT(SUPERVISOR_CONTEXT | IRQ_CONTEXT);
    switch (num)
    {
    case  SVC_DBG_WRITE:
        if (__KERNEL->dbg_console)
            console_write(__KERNEL->dbg_console, (char*)param1, (int)param2);
        break;
    case SVC_DBG_PUSH:
        if (__KERNEL->dbg_console)
            console_push(__KERNEL->dbg_console);
        break;
    default:
        error(ERROR_INVALID_SVC);
    }
}

