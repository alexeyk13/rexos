/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef LPC_UART_H
#define LPC_UART_H

#include <stdint.h>
#include "../../../userspace/sys.h"
#include "../../../userspace/uart.h"
#include "lpc_config.h"
#if (MONOLITH_UART)
#include "lpc_core.h"
#endif

typedef enum {
    IPC_UART_SET_BAUDRATE = HAL_IPC(HAL_UART),
    IPC_UART_GET_BAUDRATE,
    IPC_UART_GET_LAST_ERROR,
    IPC_UART_CLEAR_ERROR,
    //used internally
    IPC_UART_ISR_TX,
    IPC_UART_ISR_RX
} LPC_UART_IPCS;

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
#ifdef LPC11U6x
    uint8_t uart13, uart24;
#endif
} UART_DRV;

#if (MONOLITH_UART)
#define SHARED_UART_DRV                    CORE
#else

typedef struct {
    UART_DRV uart;
} SHARED_UART_DRV;

#endif

typedef enum {
    UART_0 = 0,
    UART_1,
    UART_2,
    UART_3,
    UART_4,
    UART_MAX
}UART_PORT;

typedef struct {
    uint8_t tx, rx;
    uint16_t stream_size;
    BAUD baud;
} UART_ENABLE;

#if (MONOLITH_UART)
void lpc_uart_init(SHARED_UART_DRV* drv);
bool lpc_uart_request(SHARED_UART_DRV* drv, IPC* ipc);
#else
extern const REX __LPC_UART;
#endif

#if (SYS_INFO)
#if (UART_STDIO)
//we can't use printf in uart driver, because this can halt driver loop
void printu(const char *const fmt, ...);
#else
#define printu printf
#endif
#endif //SYS_INFO

#endif // LPC_UART_H
