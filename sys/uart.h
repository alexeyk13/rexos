/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef UART_H
#define UART_H

#include "../userspace/types.h"

typedef struct {
    //baudrate
    uint32_t baud;
    //data bits: 7, 8
    uint8_t data_bits;
    //parity: 'N', 'O', 'E'
    char parity;
    //stop bits: 1, 2
    uint8_t stop_bits;
}BAUD;

#endif // UART_H
