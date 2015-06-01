/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef STDIO_H
#define STDIO_H

#include "lib.h"
#include "svc.h"
#include <stdarg.h>
#include <stdbool.h>

typedef struct {
    void (*__format)(const char *const, va_list, STDOUT, void*);
    void (*pformat)(const char *const, va_list);
    void (*sformat)(char*, const char *const, va_list);
    unsigned long (*atou)(const char *const, int);
    int (*utoa)(char*, unsigned long, int, bool);
    void (*puts)(const char*);
    void (*putc)(const char);
    char (*getc)();
    char* (*gets)(char*, int);
} LIB_STDIO;

/** \addtogroup stdio embedded uStdio
 */

/**
    \brief format string
    \param c: char
    \retval none
*/
__STATIC_INLINE void format(const char *const fmt, va_list va, STDOUT write_handler, void* write_param)
{
    return ((const LIB_STDIO*)__GLOBAL->lib[LIB_ID_STDIO])->__format(fmt, va, write_handler, write_param);
}

/**
    \brief format string, using \ref stdout as handler
    \param fmt: format (see global description)
    \param ...: list of arguments
    \retval none
*/
void printf(const char *const fmt, ...);

/**
    \brief format string to \b str
    \param str: resulting string
    \param fmt: format (see global description)
    \param ...: list of arguments
    \retval none
*/

/**
    \brief format string, using \ref SVC_PRINTD as handler
    \param fmt: format (see global description)
    \param ...: list of arguments
    \retval none
*/
void printd(const char *const fmt, ...);

/**
    \brief format string, using \ref SVC_PRINTD as handler. ISR version
    \param fmt: format (see global description)
    \param ...: list of arguments
    \retval none
*/
void iprintd(const char *const fmt, ...);

/**
    \brief format string to \b str
    \param str: resulting string
    \param fmt: format (see global description)
    \param ...: list of arguments
    \retval none
*/

void sprintf(char* str, const char *const fmt, ...);

/**
    \brief put string to stdout
    \param s: null-terminated string
    \retval none
*/
__STATIC_INLINE void puts(const char* s)
{
    ((const LIB_STDIO*)__GLOBAL->lib[LIB_ID_STDIO])->puts(s);
}

/**
    \brief put char to stdout
    \param c: char
    \retval none
*/
__STATIC_INLINE void putc(const char c)
{
    ((const LIB_STDIO*)__GLOBAL->lib[LIB_ID_STDIO])->putc(c);
}

/**
    \brief get char from stdin
    \param c: char
    \retval none
*/
__STATIC_INLINE char getc()
{
    return ((const LIB_STDIO*)__GLOBAL->lib[LIB_ID_STDIO])->getc();
}

/**
    \brief get string from stdin
    \param s: buffer for out string
    \param max_size: max string size, including null-terminator
    \retval null-terminated string
*/
__STATIC_INLINE char* gets(char* s, int max_size)
{
    return ((const LIB_STDIO*)__GLOBAL->lib[LIB_ID_STDIO])->gets(s, max_size);
}

/** \} */ // end of stdio group


#endif // STDIO_H
