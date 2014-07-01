#include "stdio.h"
#include "../process.h"

void printf(const char *const fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    __GLOBAL->lib->format(fmt, va, __HEAP->stdout, __HEAP->stdout_param);
    va_end(va);
}

void sprintf(char* str, const char * const fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    __GLOBAL->lib->sformat(str, fmt, va);
    va_end(va);
}

