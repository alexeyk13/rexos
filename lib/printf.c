/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "printf.h"
#include <string.h>

#define PRINTF_BUF_SIZE                                         10

#define FLAGS_PROCESSING                                        (1 << 0)

#define FLAGS_LEFT_JUSTIFY                                      (1 << 1)
#define FLAGS_FORCE_PLUS                                        (1 << 2)
#define FLAGS_SPACE_FOR_SIGN                                    (1 << 3)
#define FLAGS_RADIX_PREFIX                                      (1 << 4)
#define FLAGS_ZERO_PAD_NUMBERS                                  (1 << 5)
#define FLAGS_SHORT_INT                                         (1 << 6)

#define FLAGS_SIGN_MINUS                                        (1 << 7)

const char spaces[PRINTF_BUF_SIZE] =                            "          ";
const char zeroes[PRINTF_BUF_SIZE] =                            "0000000000";
const char* const DIM =                                         "KMG";

static void sprintf_handler(const char *const buf, unsigned int size, void* param)
{
    char** str = (char**)param;
    memcpy(*str, buf, size);
    *str += size;
}

static void pad_spaces(int count, WRITE_HANDLER write_handler, void *write_param)
{
    while (count > PRINTF_BUF_SIZE)
    {
        write_handler(spaces, PRINTF_BUF_SIZE, write_param);
        count -= PRINTF_BUF_SIZE;
    }
    if (count > 0)
        write_handler(spaces, count, write_param);
}

static inline void pad_zeroes(int count, WRITE_HANDLER write_handler, void *write_param)
{
    while (count > PRINTF_BUF_SIZE)
    {
        write_handler(zeroes, PRINTF_BUF_SIZE, write_param);
        count -= PRINTF_BUF_SIZE;
    }
    if (count > 0)
        write_handler(zeroes, count, write_param);
}

/** \addtogroup lib_printf embedded stdio
    \{
 */

unsigned long __atou(const char *const buf, int size)
{
    int i;
    unsigned long res = 0;
    for (i = 0; i < size && buf[i] >= '0' && buf[i] <= '9'; ++i)
        res = res * 10 + buf[i] - '0';
    return res;
}

int __utoa(char* buf, unsigned long value, int radix, bool uppercase)
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
    for(i = 0; i < 3 && value >= 9999; ++i)
        value /= 1024;
    size = __utoa(buf, value, 10, false);
    if (i)
    {
        buf[size++] = DIM[i - 1];
        buf[size++] = 'B';
    }
    buf[size] = '\x0';

    return size;
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
void __format(const char *const fmt, va_list va, WRITE_HANDLER write_handler, void* write_param)
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
                write_handler(fmt + start, cur - start, write_param);
                ++cur;
                start = cur;
            }
            else
            {
                if (cur > start)
                    write_handler(fmt + start, cur - start, write_param);
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
                        width = __atou(fmt + start, cur - start);
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
                            precision = __atou(fmt + start, cur - start);
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
                        buf_size = __utoa(buf, -d, 10, false);
                        d = buf_size + 1;
                        flags |= FLAGS_SIGN_MINUS;
                    }
                    else if (d > 0)
                    {
                        buf_size = __utoa(buf, d, 10, false);
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
                        buf_size = __utoa(buf, u, 10, false);
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
                        buf_size = __utoa(buf, u, 16, fmt[cur] == 'X');
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
                        buf_size = __utoa(buf, u, 8, false);
                        d = buf_size;
                        if (flags & (FLAGS_RADIX_PREFIX))
                            d += 1;
                    }
                    if (buf_size < precision)
                        d += precision - buf_size;
                    break;
                case 'b':
                    u = va_arg(va, unsigned long);
                    if (flags & FLAGS_SHORT_INT)
                        u = (unsigned short)u;
                    if (u > 0)
                    {
                        buf_size = size_in_bytes(u, buf);
                        d = buf_size;
                    }
                    if (buf_size < precision)
                        d += precision - buf_size;
                    break;
                }

                //right justify
                if ((flags & FLAGS_LEFT_JUSTIFY) == 0)
                    pad_spaces(width - d, write_handler, write_param);

                //b) output
                switch (fmt[cur++])
                {
                case 'c':
                    write_handler(&c, 1, write_param);
                    break;
                case 's':
                    write_handler(str, d, write_param);
                    break;
                case 'i':
                case 'd':
                case 'u':
                case 'b':
                    //sign processing
                    if (flags & FLAGS_SIGN_MINUS)
                        write_handler("-", 1, write_param);
                    else if (buf_size)
                    {
                        if (flags & FLAGS_FORCE_PLUS)
                            write_handler("+", 1, write_param);
                        else if (flags & FLAGS_SPACE_FOR_SIGN)
                            write_handler(" ", 1, write_param);
                    }
                    //zero padding
                    if (buf_size < precision)
                        pad_zeroes(precision - buf_size, write_handler, write_param);
                    //data
                    write_handler(buf, buf_size, write_param);
                    break;
                case 'x':
                case 'X':
                    if (flags & FLAGS_RADIX_PREFIX)
                        write_handler("0x", 2, write_param);
                    //zero padding
                    if (buf_size < precision)
                        pad_zeroes(precision - buf_size, write_handler, write_param);
                    //data
                    write_handler(buf, buf_size, write_param);
                    break;
                case 'o':
                    if (buf_size && (flags & FLAGS_RADIX_PREFIX))
                        write_handler("O", 1, write_param);
                    //zero padding
                    if (buf_size < precision)
                        pad_zeroes(precision - buf_size, write_handler, write_param);
                    //data
                    write_handler(buf, buf_size, write_param);
                    break;
                }

                //left justify
                if (flags & FLAGS_LEFT_JUSTIFY)
                    pad_spaces(width - d, write_handler, write_param);

                start = cur;
            }
        }
        else
            ++cur;
    }
    if (cur > start)
        write_handler(fmt + start, cur - start, write_param);
}

void sformat(char* str, const char *const fmt, va_list va)
{
    char* str_cur = str;
    __format(fmt, va, sprintf_handler, &str_cur);
    *str_cur = 0;
}

/** \} */ // end of lib_printf group
