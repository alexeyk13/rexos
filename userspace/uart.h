/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef UART_H
#define UART_H

#include <stdint.h>
#include "sys.h"
#include "cc_macro.h"
#include "file.h"
#include "ipc.h"
#include "sys_config.h"

typedef struct {
    //baudrate
    uint32_t baud;
    //data bits: 7, 8
    uint8_t data_bits;
    //parity: 'N', 'O', 'E'
    char parity;
    //stop bits: 1, 2
    uint8_t stop_bits;
}BAUD;

typedef enum {
    IPC_UART_SET_BAUDRATE = IPC_USER,
    IPC_UART_GET_BAUDRATE,
    IPC_UART_GET_LAST_ERROR,
    IPC_UART_CLEAR_ERROR,
    IPC_UART_SETUP_PRINTK,
    IPC_UART_MAX,
} UART_IPCS;

//used internally
__STATIC_INLINE void uart_encode_baudrate(BAUD* baudrate, IPC* ipc)
{
    ipc->param2 = baudrate->baud;
    ipc->param3 = (baudrate->data_bits << 16) | (baudrate->parity << 8) | baudrate->stop_bits;
}

__STATIC_INLINE void uart_decode_baudrate(IPC* ipc, BAUD* baudrate)
{
    baudrate->baud = ipc->param2;
    baudrate->data_bits = (ipc->param3 >> 16) & 0xff;
    baudrate->parity = (ipc->param3 >> 8) & 0xff;
    baudrate->stop_bits = (ipc->param3) & 0xff;
}

__STATIC_INLINE void uart_setup_printk(int num)
{
    ack(object_get(SYS_OBJ_UART), HAL_CMD(HAL_UART, IPC_UART_SETUP_PRINTK), num, 0, 0);
}

__STATIC_INLINE void uart_setup_stdout(int num)
{
    object_set(SYS_OBJ_STDOUT, get(object_get(SYS_OBJ_UART), HAL_CMD(HAL_UART, IPC_GET_TX_STREAM), num, 0, 0));
}

__STATIC_INLINE void uart_setup_stdin(int num)
{
    object_set(SYS_OBJ_STDIN, get(object_get(SYS_OBJ_UART), HAL_CMD(HAL_UART, IPC_GET_RX_STREAM), num, 0, 0));
}

__STATIC_INLINE void uart_set_baudrate(int num, BAUD* baudrate)
{
    IPC ipc;
    uart_encode_baudrate(baudrate, &ipc);
    ipc.cmd = HAL_CMD(HAL_UART, IPC_UART_SET_BAUDRATE);
    ipc.param1 = num;
    ipc.process = object_get(SYS_OBJ_UART);
    call(&ipc);
}



#endif // UART_H
