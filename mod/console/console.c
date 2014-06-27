/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "console.h"
#include "../../userspace/core/core.h"
#include "string.h"
#include "delay.h"
#include "../../error.h"
#include "../../kernel/mem_kernel.h"

#define CONSOLE_FLAG_BUSY                                        (1 << 0)
#define CONSOLE_FLAG_LN                                            (1 << 1)

static void uart_on_write_complete(void* param);
static void uart_on_read_complete(void* param);

static const UART_CB _console_uart_cb = {
    uart_on_read_complete,
    uart_on_write_complete,
    NULL
};

static void uart_on_write_complete(void* param)
{
    CONSOLE* console = (CONSOLE*)param;
    rb_get(&console->tx_buf);
    if (!rb_is_empty(&console->tx_buf))
        uart_write(console->h.port, &(console->tx_buf.data[console->tx_buf.header.tail]), 1);
}

static void uart_on_read_complete(void* param)
{
    CONSOLE* console = (CONSOLE*)param;
    //no ln, event immediattly
    if (console->h.rx_buf == NULL)
        event_pulse(console->h.rx_event);
    //eol, symbol is ignored, but event pulsed
    else if(console->h.rx_buf[0] == '\r')
    {
        console->h.rx_buf = NULL;
        event_pulse(console->h.rx_event);
    }
    //read max
    else if (--console->h.size_max == 0)
    {
        console->h.rx_buf = NULL;
        event_pulse(console->h.rx_event);
    }
    //read more
    else
    {
        console->h.rx_buf++;
        uart_read(console->h.port, console->h.rx_buf, 1);
    }
}

CONSOLE* console_create(UART_CLASS port, int tx_size, int priority)
{
    CONSOLE* console = sys_alloc(sizeof(CONSOLE_HEADER) + sizeof(RB_HEADER) + tx_size);
    if (console)
    {
        rb_init(&console->tx_buf, tx_size);
        console->h.port = port;
        console->h.rx_event =  event_create();
        uart_enable(port, (P_UART_CB)&_console_uart_cb, console, priority);
        console->h.uart_mode = 0;
        console->h.rx_buf = NULL;
        console->h.size_max = 0;
    }
    else
        error(ERROR_OUT_OF_SYSTEM_MEMORY);

    return console;
}

void console_destroy(CONSOLE* console)
{
    console_push(console);
    //make sure last char is written
    delay_ms(1);
    uart_disable(console->h.port);
    event_destroy(console->h.rx_event);
    sys_free(console);
}

void console_write(CONSOLE* console, char* buf, int size)
{
    int sent = 0;
    while (sent < size)
    {
        while (rb_is_full(&console->tx_buf))
            uart_write_wait(console->h.port);
        CRITICAL_ENTER;
        if (!rb_is_full(&console->tx_buf))
        {
            bool was_empty = rb_is_empty(&console->tx_buf);
            rb_put(&console->tx_buf, buf[sent++]);
            if (was_empty)
                uart_write(console->h.port, &(console->tx_buf.data[console->tx_buf.header.tail]), 1);
        }
        CRITICAL_LEAVE;
    }
}

void console_writeln(CONSOLE* console, char* buf)
{
    console_write(console, buf, strlen(buf));
    console_write(console, "\n\r", 2);
}

void console_putc(CONSOLE* console, char c)
{
    console_write(console, &c, 1);
}

void console_push(CONSOLE* console)
{
    while (!rb_is_empty(&console->tx_buf))
        uart_write_wait(console->h.port);
}

int console_read(CONSOLE* console, char* buf, int size_max)
{
    console->h.rx_buf = NULL;
    console->h.size_max = 0;
    uart_read(console->h.port, buf, size_max);
    event_wait(console->h.rx_event, INFINITE);
    return size_max;
}

int console_readln(CONSOLE* console, char* buf, int size_max)
{
    console->h.rx_buf = buf;
    console->h.size_max = size_max;
    uart_read(console->h.port, console->h.rx_buf, 1);
    event_wait(console->h.rx_event, INFINITE);

    if (console->h.size_max)
        buf[size_max - console->h.size_max] = '\x0';
    return size_max - console->h.size_max;
}

char console_getc(CONSOLE* console)
{
    char c;
    console_read(console, &c ,1);
    return c;
}
