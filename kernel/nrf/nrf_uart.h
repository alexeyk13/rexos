/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: RJ (jam_roma@yahoo.com)
*/

#ifndef _NRF_UART_H_
#define _NRF_UART_H_

/*
        nRF UART driver
  */

#include "../../userspace/process.h"
#include "../../userspace/uart.h"
#include "../../userspace/io.h"
#include "sys_config.h"
#include "nrf_exo.h"
#include <stdbool.h>

typedef struct {
    HANDLE tx_stream, tx_handle, rx_stream, rx_handle;
    uint16_t tx_size, tx_total, rx_free;
    char tx_buf[UART_BUF_SIZE];
} UART_STREAM;

typedef struct {
    IO* tx_io;
    IO* rx_io;
#if (UART_DOUBLE_BUFFERING)
    IO* rx2_io;
    unsigned int rx2_max;
#endif //
    unsigned int rx_max, tx_processed;
    HANDLE tx_process, rx_process, rx_timer;
    unsigned int rx_char_timeout, rx_interleaved_timeout;
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
} UART_DRV;

void nrf_uart_init(EXO* exo);
void nrf_uart_request(EXO* exo, IPC* ipc);



#endif /* _NRF_UART_H_ */
