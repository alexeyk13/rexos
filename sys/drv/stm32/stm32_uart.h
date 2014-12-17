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

#include "../../../userspace/process.h"
#include "../../../userspace/uart.h"
#include "stm32_config.h"
#include "sys_config.h"
#if (MONOLITH_UART)
#include "stm32_core.h"
#endif

typedef enum {
    IPC_UART_SET_BAUDRATE = HAL_IPC(HAL_UART),
    IPC_UART_GET_BAUDRATE,
    IPC_UART_GET_LAST_ERROR,
    IPC_UART_CLEAR_ERROR,
    //used internally
    IPC_UART_ISR_TX,
    IPC_UART_ISR_RX
} STM32_UART_IPCS;

typedef enum {
    UART_1 = 0,
    UART_2,
    UART_3,
    UART_4,
    UART_5,
    UART_6,
    UART_7,
    UART_8,
    UART_MAX
}UART_PORT;

typedef struct {
    uint8_t tx, rx;
    uint16_t stream_size;
    BAUD baud;
} UART_ENABLE;

typedef struct {
    uint8_t tx_pin, rx_pin;
    uint16_t error;
    HANDLE tx_stream, tx_handle;
    uint16_t tx_total, tx_chunk_pos, tx_chunk_size;
    char tx_buf[UART_TX_BUF_SIZE];
    HANDLE rx_stream, rx_handle;
    uint16_t rx_free;
} UART;

typedef struct {
    UART* uarts[UARTS_COUNT];
} UART_DRV;

#if (MONOLITH_UART)
#define SHARED_UART_DRV                    CORE
#else

typedef struct {
    UART_DRV uart;
} SHARED_UART_DRV;

#endif

#if (SYS_INFO)
#if (UART_STDIO)
//we can't use printf in uart driver, because this can halt driver loop
void printu(const char *const fmt, ...);
#else
#define printu printf
#endif
#endif //SYS_INFO

void stm32_uart_init(SHARED_UART_DRV* drv);
bool stm32_uart_request(SHARED_UART_DRV* drv, IPC* ipc);

#if !(MONOLITH_UART)
extern const REX __STM32_UART;
#endif

#endif // STM32_UART_H
