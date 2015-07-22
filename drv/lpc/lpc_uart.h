/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef LPC_UART_H
#define LPC_UART_H

#include <stdint.h>
#include "../../userspace/sys.h"
#include "../../userspace/uart.h"
#include "lpc_config.h"
#if (MONOLITH_UART)
#include "lpc_core.h"
#endif

typedef struct {
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

void lpc_uart_init(SHARED_UART_DRV* drv);
bool lpc_uart_request(SHARED_UART_DRV* drv, IPC* ipc);

#endif // LPC_UART_H
