/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "printf.h"
#include <string.h>
#include "dbg_console.h"
#include "dbg.h"

#define PRINTF_BUF_SIZE                                         10

#define FLAGS_PROCESSING                                        (1 << 0)

#define FLAGS_LEFT_JUSTIFY                                      (1 << 1)
#define FLAGS_FORCE_PLUS                                        (1 << 2)
#define FLAGS_SPACE_FOR_SIGN                                    (1 << 3)
#define FLAGS_RADIX_PREFIX                                      (1 << 5)
#define FLAGS_ZERO_PAD_NUMBERS                                  (1 << 6)
#define FLAGS_SHORT_INT                                         (1 << 7)

#define FLAGS_SIGN_MINUS                                        (1 << 8)

const char spaces[PRINTF_BUF_SIZE] =                            "          ";
const char zeroes[PRINTF_BUF_SIZE] =                            "0000000000";
const char* const DIM =                                              "KMG";

void sprintf_handler(void* param, const char *const buf, unsigned int size)
{
    char** str = (char**)param;
    memcpy(*str, buf, size);
    *str += size;
}

static void pad_spaces(WRITE_HANDLER write_handler, void *write_param, int count)
{
    while (count > PRINTF_BUF_SIZE)
    {
        write_handler(write_param, spaces, PRINTF_BUF_SIZE);
        count -= PRINTF_BUF_SIZE;
    }
    if (count)
        write_handler(write_param, spaces, count);
}

static inline void pad_zeroes(WRITE_HANDLER write_handler, void *write_param, int count)
{
    while (count > PRINTF_BUF_SIZE)
    {
        write_handler(write_param, zeroes, PRINTF_BUF_SIZE);
        count -= PRINTF_BUF_SIZE;
    }
    if (count)
        write_handler(write_param, zeroes, count);
}

/** \addtogroup lib_printf embedded stdio
    \{
 */

/**
    \brief string to unsigned int
    \param buf: untruncated strings
    \param size: size of buf in bytes
    \retval integer result. 0 on error
*/
unsigned long atou(char* buf, int size)
{
    int i;
    unsigned long res = 0;
    for (i = 0; i < size && buf[i] >= '0' && buf[i] <= '9'; ++i)
        res = res * 10 + buf[i] - '0';
    return res;
}

/**
    \brief unsigned int to string. Used internally in printf
    \param buf: out buffer
    \param value: in value
    \param radix: radix of original value
    \param uppercase: upper/lower case of result for radix 16
    \retval size of resulting string. 0 on error
*/
int utoa(char* buf, unsigned long value, int radix, bool uppercase)
{
    int size = 0;
    char c;
    if (value)
        while (value)
        {
            c = (char)(value % radix);
            if (c > 9)
                c += (uppercase ? 'A' : 'a') - 10;
            else
                c += '0';
            buf[PRINTF_BUF_SIZE - size++] = c;
            value /= radix;
        }
    else
        buf[PRINTF_BUF_SIZE - size++] = '0';
    memmove(buf, buf + PRINTF_BUF_SIZE - size + 1, size);
    return size;
}

/**
    \brief int to string, representing size in bytes.
    \param value: value in bytes
    \param buf: out string holder. Must be at least 6 bytes long
    \retval size of out string
*/
int size_in_bytes(unsigned int value, char* buf)
{
    int i, size;
    for(i = 0; i < 3 && value >= 1024; ++i)
        value /= 1024;
    size = utoa(buf, value, 10, false);
    if (i)
        buf[size++] = DIM[i - 1];
    buf[size++] = 'B';
    buf[size] = '\x0';

    return size;
}

/**
    \brief print size in bytes
    \param value: value in bytes
    \param size: size of outer string. Free space will be left-padded with spaces
    \retval none
*/
void print_size_in_bytes(unsigned int value, int size)
{
    char buf[6];
    int sz_out;
    sz_out = size_in_bytes(value, buf);
    if (size > sz_out)
        pad_spaces(printf_handler, NULL, size - sz_out);
    printf(buf);
}

/**
    \brief print minidump
    \param addr: starting address
    \param size: size of dump
    \retval none
*/
void dump(unsigned int addr, unsigned int size)
{
    printf("memory dump 0x%08x-0x%08x\n\r", addr, addr + size);
    unsigned int i = 0;
    for (i = 0; i < size; ++i)
    {
        if ((i % 0x10) == 0)
            printf("0x%08x: ", addr + i);
        printf("%02X ", ((unsigned char*)addr)[i]);
        if ((i % 0x10) == 0xf)
            printf("\n\r");
    }
    if (size % 0x10)
        printf("\n\r");
}

/** \} */ // end of lib_printf group

/** \addtogroup lib_printf embedded stdio
    \{
 */

/**
    \brief format string, using specific handler
    \param write_handler: user-specified handler
    \param write_param: param for handler
    \param fmt: format (see global description)
    \param va: va_list of arguments
    \retval none
*/
void format(WRITE_HANDLER write_handler, void *write_param, char *fmt, va_list va)
{
    char buf[PRINTF_BUF_SIZE];
    unsigned char flags;
    unsigned int start = 0;
    unsigned int cur = 0;
    unsigned int buf_size = 0;
    unsigned int width, precision;
    char* str = NULL;
    unsigned long u;
    long d;
    char c;
    while (fmt[cur])
    {
        if (fmt[cur] == '%')
        {
            //write plain block
            if (fmt[cur + 1] == '%')
            {
                ++cur;
                write_handler(write_param, fmt + start, cur - start);
                ++cur;
                start = cur;
            }
            else
            {
                if (cur > start)
                    write_handler(write_param, fmt + start, cur - start);
                ++cur;
                //1. decode flags
                flags = FLAGS_PROCESSING;
                while (fmt[cur] && (flags & FLAGS_PROCESSING))
                {
                    switch (fmt[cur])
                    {
                    case '-':
                        flags |= FLAGS_LEFT_JUSTIFY;
                        ++cur;
                        break;
                    case '+':
                        flags |= FLAGS_FORCE_PLUS;
                        ++cur;
                        break;
                    case ' ':
                        flags |= FLAGS_SPACE_FOR_SIGN;
                        ++cur;
                        break;
                    case '#':
                        flags |= FLAGS_RADIX_PREFIX;
                        ++cur;
                        break;
                    case '0':
                        flags |= FLAGS_ZERO_PAD_NUMBERS;
                        ++cur;
                        break;
                    default:
                        flags &= ~FLAGS_PROCESSING;
                    }
                }
                //2. width
                width = 0;
                if (fmt[cur] == '*')
                {
                    width = va_arg(va, int);
                    ++cur;
                }
                else
                {
                    start = cur;
                    while (fmt[cur] >= '0' && fmt[cur] <= '9')
                        ++cur;
                    if (cur > start)
                        width = atou(fmt + start, cur - start);
                }
                //3. precision
                precision = 1;
                if (fmt[cur] == '.')
                {
                    ++cur;
                    if (fmt[cur] == '*')
                    {
                        precision = va_arg(va, int);
                        ++cur;
                    }
                    else
                    {
                        start = cur;
                        while (fmt[cur] >= '0' && fmt[cur] <= '9')
                            ++cur;
                        if (cur > start)
                            precision = atou(fmt + start, cur - start);
                    }
                }
                //4. int length
                switch (fmt[cur])
                {
                case 'h':
                    flags |= FLAGS_SHORT_INT;
                    ++cur;
                    break;
                case 'l':
                    ++cur;
                    break;
                }
                //5. specifier
                //a) calculate size and format
                d = 0;
                buf_size = 0;
                switch (fmt[cur])
                {
                case 'c':
                    c = (char)va_arg(va, int);
                    d = 1;
                    break;
                case 's':
                    str = va_arg(va, char*);
                    d = strlen(str);
                    if (precision > 1 && precision < d)
                        d = precision;
                    break;
                case 'i':
                case 'd':
                    if ((flags & FLAGS_ZERO_PAD_NUMBERS) && (precision < width))
                        precision = width;
                    u = va_arg(va, unsigned long);
                    if (flags & FLAGS_SHORT_INT)
                        d = (short)u;
                    else
                        d = (long)u;
                    if (d < 0)
                    {
                        buf_size = utoa(buf, -d, 10, false);
                        d = buf_size + 1;
                        flags |= FLAGS_SIGN_MINUS;
                    }
                    else if (d > 0)
                    {
                        buf_size = utoa(buf, d, 10, false);
                        d = buf_size;
                        if (flags & (FLAGS_FORCE_PLUS | FLAGS_SPACE_FOR_SIGN))
                            ++d;
                    }
                    if (buf_size < precision)
                        d += precision - buf_size;
                    break;
                case 'u':
                    if ((flags & FLAGS_ZERO_PAD_NUMBERS) && (precision < width))
                        precision = width;
                    u = va_arg(va, unsigned long);
                    if (flags & FLAGS_SHORT_INT)
                        u = (unsigned short)u;
                    if (u > 0)
                    {
                        buf_size = utoa(buf, u, 10, false);
                        d = buf_size;
                    }
                    if (buf_size < precision)
                        d += precision - buf_size;
                    break;
                case 'x':
                case 'X':
                    if ((flags & FLAGS_ZERO_PAD_NUMBERS) && (precision < width))
                        precision = width;
                    u = va_arg(va, unsigned long);
                    if (flags & FLAGS_SHORT_INT)
                        u = (unsigned short)u;
                    if (u > 0)
                    {
                        buf_size = utoa(buf, u, 16, fmt[cur] == 'X');
                        d = buf_size;
                        if (flags & (FLAGS_RADIX_PREFIX))
                            d += 2;
                    }
                    if (buf_size < precision)
                        d += precision - buf_size;
                    break;
                case 'o':
                    if ((flags & FLAGS_ZERO_PAD_NUMBERS) && (precision < width))
                        precision = width;
                    u = va_arg(va, unsigned long);
                    if (flags & FLAGS_SHORT_INT)
                        u = (unsigned short)u;
                    if (u > 0)
                    {
                        buf_size = utoa(buf, u, 8, false);
                        d = buf_size;
                        if (flags & (FLAGS_RADIX_PREFIX))
                            d += 1;
                    }
                    if (buf_size < precision)
                        d += precision - buf_size;
                    break;
                }

                //right justify
                if ((flags & FLAGS_LEFT_JUSTIFY) == 0)
                    pad_spaces(write_handler, write_param, width - d);

                //b) output
                switch (fmt[cur++])
                {
                case 'c':
                    write_handler(write_param, &c, 1);
                    break;
                case 's':
                    write_handler(write_param, str, d);
                    break;
                case 'i':
                case 'd':
                case 'u':
                    //sign processing
                    if (flags & FLAGS_SIGN_MINUS)
                        write_handler(write_param, "-", 1);
                    else if (buf_size)
                    {
                        if (flags & FLAGS_FORCE_PLUS)
                            write_handler(write_param, "+", 1);
                        else if (flags & FLAGS_SPACE_FOR_SIGN)
                            write_handler(write_param, " ", 1);
                    }
                    //zero padding
                    if (buf_size < precision)
                        pad_zeroes(write_handler, write_param, precision - buf_size);
                    //data
                    write_handler(write_param, buf, buf_size);
                    break;
                case 'x':
                case 'X':
                    if (buf_size && (flags & FLAGS_RADIX_PREFIX))
                        write_handler(write_param, "0x", 2);
                    //zero padding
                    if (buf_size < precision)
                        pad_zeroes(write_handler, write_param, precision - buf_size);
                    //data
                    write_handler(write_param, buf, buf_size);
                    break;
                case 'o':
                    if (buf_size && (flags & FLAGS_RADIX_PREFIX))
                        write_handler(write_param, "O", 1);
                    //zero padding
                    if (buf_size < precision)
                        pad_zeroes(write_handler, write_param, precision - buf_size);
                    //data
                    write_handler(write_param, buf, buf_size);
                    break;
                }

                //left justify
                if (flags & FLAGS_LEFT_JUSTIFY)
                    pad_spaces(write_handler, write_param, width - d);

                start = cur;
            }
        }
        else
            ++cur;
    }
    if (cur > start)
        write_handler(write_param, fmt + start, cur - start);
}

/**
    \brief format string, using \ref dbg_write as handler
    \param fmt: format (see global description)
    \param ...: list of arguments
    \retval none
*/
void printf(char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    format(printf_handler, NULL, fmt, va);
    va_end(va);
}

/**
    \brief format string to \b str
    \param str: resulting string
    \param fmt: format (see global description)
    \param ...: list of arguments
    \retval none
*/
void sprintf(char* str, char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    char* str_cur = str;
    format(sprintf_handler, &str_cur, fmt, va);
    *str_cur = 0;
    va_end(va);
}

/** \} */ // end of lib_printf group
