/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "../stdio.h"
#include "../process.h"

void printf(const char *const fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    ((const LIB_STDIO*)__GLOBAL->lib[LIB_ID_STDIO])->pformat(fmt, va);
    va_end(va);
}

static void printd_handler(const char *const buf, unsigned int size, void* param)
{
    svc_call(SVC_PRINTD, (unsigned int)buf, (unsigned int)size, 0);
}

static void iprintd_handler(const char *const buf, unsigned int size, void* param)
{
    __GLOBAL->svc_irq(SVC_PRINTD, (unsigned int)buf, (unsigned int)size, 0);
}

void printd(const char *const fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    format(fmt, va, printd_handler, NULL);
    va_end(va);
}

void iprintd(const char *const fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    format(fmt, va, iprintd_handler, NULL);
    va_end(va);
}

void sprintf(char* str, const char * const fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    ((const LIB_STDIO*)__GLOBAL->lib[LIB_ID_STDIO])->sformat(str, fmt, va);
    va_end(va);
}
