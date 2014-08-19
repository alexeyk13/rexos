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
#include "../uart.h"
#include "stm32_gpio.h"
#include "stm32_config.h"
#include "sys_config.h"

typedef enum {
    IPC_UART_SET_BAUDRATE = IPC_USER,
    IPC_UART_GET_BAUDRATE,
    IPC_UART_GET_LAST_ERROR,
    IPC_UART_CLEAR_ERROR,
    //used internally
    IPC_UART_ISR_WRITE_CHUNK_COMPLETE,
    IPC_UART_ISR_RX_CHAR
} STM32_UART_IPCS;

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
    BAUD baud;
    unsigned int tx_stream_size, rx_stream_size;
} UART_ENABLE;

#if defined(STM32F10X_LD) || defined(STM32F10X_LD_VL)
#define UARTS_COUNT                                         2
#elif defined(STM32F10X_MD) || defined(STM32F10X_MD_VL)
#define UARTS_COUNT                                         3
#elif defined(STM32F1)
#define UARTS_COUNT                                         5
#elif defined(STM32F2) || defined(STM32F40_41xxx)
#define UARTS_COUNT                                         6
#elif defined(STM32F4)
#define UARTS_COUNT                                         8
#endif

extern const REX __STM32_UART;

#endif // STM32_UART_H
