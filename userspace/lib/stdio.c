/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "stdio.h"
#include "../process.h"

void printf(const char *const fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    ((const LIB_STDIO*)__GLOBAL->lib[LIB_ID_STDIO])->pformat(fmt, va);
    va_end(va);
}

void sprintf(char* str, const char * const fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    ((const LIB_STDIO*)__GLOBAL->lib[LIB_ID_STDIO])->sformat(str, fmt, va);
    va_end(va);
}
