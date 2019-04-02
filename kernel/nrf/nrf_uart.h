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
    uint16_t error;
    bool io_mode;
//    union {
//        UART_STREAM s;
//        UART_IO i;
//    };
} UART;

typedef struct {
    UART* uarts[UARTS_COUNT];
} UART_DRV;

void nrf_uart_init(EXO* exo);
void nrf_uart_request(EXO* exo, IPC* ipc);



#endif /* _NRF_UART_H_ */
