/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef LPC_UART_H
#define LPC_UART_H

#include <stdint.h>
#include "../../userspace/sys.h"
#include "../../userspace/uart.h"
#include "../../userspace/io.h"
#include "../../userspace/rb.h"
#include "lpc_config.h"
#include "lpc_core.h"

typedef struct {
    HANDLE tx_stream, tx_handle, rx_stream, rx_handle;
    uint16_t tx_total, tx_chunk_pos, tx_chunk_size, rx_free;
    char tx_buf[UART_BUF_SIZE];
} UART_STREAM;

typedef struct {
    IO* tx_io;
    IO* rx_io;
    unsigned int rx_max, tx_processed;
    HANDLE tx_process, rx_process, rx_timer;
    unsigned int rx_char_timeout, rx_interleaved_timeout;
#if (UART_IO_PREBUFFER)
    char rx_buf[UART_BUF_SIZE];
    RB rx_rb;
#endif //UART_IO_PREBUFFER
} UART_IO;

typedef struct {
    uint16_t error;
    bool io_mode;
    union {
        UART_STREAM s;
        UART_IO i;
    };
} UART;

typedef struct {
    UART* uarts[UARTS_COUNT];
#ifdef LPC11U6x
    uint8_t uart13, uart24;
#endif
} UART_DRV;

void lpc_uart_init(CORE* drv);
void lpc_uart_request(CORE* drv, IPC* ipc);

#endif // LPC_UART_H
