/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef STM32_UART_H
#define STM32_UART_H

/*
        STM32 UART driver
  */

#include "../../userspace/process.h"
#include "../../userspace/uart.h"
#include "../../userspace/sys.h"
#include "stm32_config.h"
#include "sys_config.h"
#if (MONOLITH_UART)
#include "stm32_core.h"
#endif

typedef struct {
    uint16_t error;
    HANDLE tx_stream, tx_handle;
    uint16_t tx_total, tx_chunk_pos, tx_chunk_size;
    char tx_buf[UART_TX_BUF_SIZE];
    HANDLE rx_stream, rx_handle;
    uint16_t rx_free;
    BAUD baud;
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

void stm32_uart_init(SHARED_UART_DRV* drv);
bool stm32_uart_request(SHARED_UART_DRV* drv, IPC* ipc);

#if !(MONOLITH_UART)
extern const REX __STM32_UART;
#endif

#endif // STM32_UART_H
