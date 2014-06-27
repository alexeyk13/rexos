/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef CONSOLE_H
#define CONSOLE_H

#include "dev.h"
#include "event.h"
#include "mutex.h"
#include "rb.h"
#include "uart.h"

typedef struct {
    UART_CLASS port;
    unsigned int uart_mode;
    HANDLE rx_event;
    char* rx_buf;
    int size_max;
}CONSOLE_HEADER;

typedef struct {
    CONSOLE_HEADER h;
    RB tx_buf;
}CONSOLE;

CONSOLE* console_create(UART_CLASS port, int tx_size, int priority);
void console_destroy(CONSOLE* console);
void console_write(CONSOLE* console, char* buf, int size);
void console_writeln(CONSOLE* console, char* buf);
void console_putc(CONSOLE* console, char c);
void console_push(CONSOLE* console);
int console_read(CONSOLE* console, char* buf, int size_max);
int console_readln(CONSOLE* console, char* buf, int size_max);
char console_getc(CONSOLE* console);

#endif // CONSOLE_H
