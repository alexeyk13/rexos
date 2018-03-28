/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "stdio.h"
#include "process.h"

void format(const char *const fmt, va_list va, STDOUT write_handler, void* write_param)
{
    return ((const LIB_STDIO*)__GLOBAL->lib[LIB_ID_STDIO])->__format(fmt, va, write_handler, write_param);
}

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

void puts(const char* s)
{
    ((const LIB_STDIO*)__GLOBAL->lib[LIB_ID_STDIO])->puts(s);
}

void putc(const char c)
{
    ((const LIB_STDIO*)__GLOBAL->lib[LIB_ID_STDIO])->putc(c);
}

char getc()
{
    return ((const LIB_STDIO*)__GLOBAL->lib[LIB_ID_STDIO])->getc();
}

char* gets(char* s, int max_size)
{
    return ((const LIB_STDIO*)__GLOBAL->lib[LIB_ID_STDIO])->gets(s, max_size);
}
