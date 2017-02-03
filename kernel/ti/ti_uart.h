/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef TI_UART_H
#define TI_UART_H

#include "ti_exo.h"
#include "../../userspace/ti/ti.h"
#include "../../userspace/types.h"
#include "../../userspace/ipc.h"
#include <stdint.h>
#include <stdbool.h>
#include "sys_config.h"

typedef struct {
    uint16_t error;
    HANDLE tx_stream, tx_handle, rx_stream, rx_handle;
    uint16_t tx_size, tx_total;
    char tx_buf[UART_BUF_SIZE];
    bool active;
} UART_DRV;

void ti_uart_init(EXO* exo);
void ti_uart_request(EXO* exo, IPC* ipc);


#endif // TI_UART_H
