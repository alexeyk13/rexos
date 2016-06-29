/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2016, Alexey Kramarenko
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
#include "../../userspace/stm32/stm32_driver.h"
#include "stm32_config.h"
#include "sys_config.h"
#include "stm32_core.h"

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
#if defined(STM32F0) && (UARTS_COUNT > 3)
    unsigned char isr3_cnt;
#endif
} UART_DRV;

void stm32_uart_init(CORE* core);
void stm32_uart_request(CORE* core, IPC* ipc);

#endif // STM32_UART_H
