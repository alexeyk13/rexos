/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef STDIO_H
#define STDIO_H

#include "lib.h"
#include "../process.h"

/** \addtogroup stdio embedded uStdio
 */

/**
    \brief format string, using \ref dbg_write as handler
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
void sprintf(char* str, const char *const fmt, ...);

/**
    \brief put string to stdout
    \param s: null-terminated string
    \retval none
*/
__STATIC_INLINE void puts(const char* s)
{
    __GLOBAL->lib->puts(s);
}

/**
    \brief put char to stdout
    \param c: char
    \retval none
*/
__STATIC_INLINE void putc(const char c)
{
    __GLOBAL->lib->putc(c);
}

/**
    \brief get char from stdin
    \param c: char
    \retval none
*/
__STATIC_INLINE char getc()
{
    return __GLOBAL->lib->getc();
}

/**
    \brief get string from stdin
    \param s: buffer for out string
    \param max_size: max string size, including null-terminator
    \retval null-terminated string
*/
__STATIC_INLINE char* gets(char* s, int max_size)
{
    return __GLOBAL->lib->gets(s, max_size);
}

/** \} */ // end of stdlib group


#endif // STDIO_H
