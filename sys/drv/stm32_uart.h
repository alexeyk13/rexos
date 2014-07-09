/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef STM32_UART_H
#define STM32_UART_H

/*
        UART driver. Hardware-independent part
  */

#include "../../userspace/process.h"

#define UART_MODE_RX_ENABLE                (1 << 0)
#define UART_MODE_TX_ENABLE                (1 << 1)
#define UART_MODE_TX_COMPLETE_ENABLE    (1 << 2)

typedef struct {
    //baudrate
    uint32_t baud;
    //data bits: 7, 8
    uint8_t data_bits;
    //parity: 'N', 'O', 'E'
    char parity;
    //stop bits: 1, 2
    uint8_t stop_bits;
}UART_BAUD;

typedef enum {
    UART_1 = 0,
    UART_2,
    UART_3,
    UART_4,
    UART_5,
    UART_6,
    UART_7,
    UART_8
}UART_PORT;

extern void uart_write(UART_PORT port, char* buf, int size);
extern void uart_write_wait(UART_PORT port);
extern void uart_read(UART_PORT port, char* buf, int size);
extern void uart_read_cancel(UART_PORT port);

void uart_write_svc(const char *const buf, unsigned int size, void* param);

extern const REX __STM32_UART;

#endif // STM32_UART_H
