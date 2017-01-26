/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "lib_stdio.h"
#include "printf.h"
#include "../userspace/process.h"
#include "../userspace/stream.h"
#include <string.h>

static void printf_handler(const char *const buf, unsigned int size, void* param)
{
    stream_write(__PROCESS->stdout, buf, size);
}

static void pformat(const char *const fmt, va_list va)
{
    __format(fmt, va, printf_handler, NULL);
}

static void __puts(const char* s)
{
    stream_write(__PROCESS->stdout, s, strlen(s));
}

static void __putc(const char c)
{
    stream_write(__PROCESS->stdout, &c, 1);
}

static char __getc()
{
    char c;
    stream_read(__PROCESS->stdin, &c, 1);
    return c;
}

static char* __gets(char* s, int max_size)
{
    int i;
    for (i = 0; (s[i] = __getc()) != '\n' && i < max_size - 1; ++i) {}
    s[i] = 0;
    return s;
}

const LIB_STDIO __LIB_STDIO = {
    __format,
    pformat,
    sformat,
    __atou,
    __utoa,
    __puts,
    __putc,
    __getc,
    __gets,
};
