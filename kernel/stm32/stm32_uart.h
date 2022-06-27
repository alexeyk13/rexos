/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: Alexey E. Kramarenko (alexeyk13@yandex.ru)
*/

#ifndef STM32_UART_H
#define STM32_UART_H

/*
        STM32 UART driver
  */

#include "../../userspace/process.h"
#include "../../userspace/uart.h"
#include "../../userspace/io.h"
#include "../../userspace/stm32/stm32_driver.h"
#include "stm32_config.h"
#include "sys_config.h"
#include "stm32_exo.h"
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
#if (UART_ISO7816_MODE_SUPPORT)
    uint8_t t, crc;
#endif //UART_ISO7816_MODE_SUPPORT
} UART_IO;

typedef struct {
    uint16_t error;
#if (UART_IO_MODE_SUPPORT)
    bool io_mode;
    union {
        UART_STREAM s;
        UART_IO i;
    };
#else
    UART_STREAM s;
#endif //UART_IO_MODE_SUPPORT
} UART;

typedef struct {
    UART* uarts[UARTS_COUNT];
#if defined(STM32F0) && (UARTS_COUNT > 3)
    unsigned char isr3_cnt;
#endif
} UART_DRV;

void stm32_uart_init(EXO* exo);
void stm32_uart_request(EXO* exo, IPC* ipc);

#endif // STM32_UART_H
