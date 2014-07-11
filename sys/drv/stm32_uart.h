/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef STM32_UART_H
#define STM32_UART_H

/*
        STM32 UART driver
  */

#include "../../userspace/process.h"
#include "stm32_gpio.h"

typedef enum {
    IPC_UART_ENABLE = IPC_USER,
    IPC_UART_DISABLE,
    IPC_UART_SET_BAUDRATE,
    IPC_UART_GET_BAUDRATE,
    IPC_UART_FLUSH,
    IPC_UART_GET_TX_STREAM,
    IPC_UART_GET_RX_STREAM,
    IPC_UART_GET_LAST_ERROR,
    IPC_UART_CLEAR_ERROR,
    //used internally
    IPC_UART_ISR_WRITE_CHUNK_COMPLETE,
    IPC_UART_ISR_READ_CHUNK_COMPLETE
} STM32_UART_IPCS;

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

typedef struct {
    PIN tx, rx;
    UART_BAUD baud;
} UART_ENABLE;

extern const REX __STM32_UART;

#endif // STM32_UART_H
