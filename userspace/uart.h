/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: Alexey E. Kramarenko (alexeyk13@yandex.ru)
*/

#ifndef UART_H
#define UART_H

#include <stdint.h>
#include "ipc.h"

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
    IPC_UART_SET_COMM_TIMEOUTS,
    IPC_UART_MAX,
} UART_IPCS;

#define UART_RX_STREAM                          (1 << 0)
#define UART_TX_STREAM                          (1 << 1)

#define UART_MODE                               (3 << 2)
#define UART_MODE_STREAM                        (0 << 2)
#define UART_MODE_IO                            (1 << 2)
#define UART_MODE_ISO7816                       (2 << 2)

#define UART_DMA_RX_MODE                        (1 << 4)
#define UART_DMA_TX_MODE                        (1 << 5)

//used internally by driver
void uart_encode_baudrate(BAUD* baudrate, IPC* ipc);
void uart_decode_baudrate(IPC* ipc, BAUD* baudrate);
//dbg related
#define uart_setup_printk(num)                                                                              ipc_post_exo(HAL_CMD(HAL_UART, IPC_UART_SETUP_PRINTK), (num), 0, 0)
#define uart_setup_stdout(num)                                                                              object_set(SYS_OBJ_STDOUT, get_handle_exo(HAL_REQ(HAL_UART, IPC_GET_TX_STREAM), (num), 0, 0))
#define uart_setup_stdin(num)                                                                               object_set(SYS_OBJ_STDIN, get_handle_exo(HAL_REQ(HAL_UART, IPC_GET_RX_STREAM), (num), 0, 0))

#define uart_open(num, mode)                                                                                get_handle_exo(HAL_REQ(HAL_UART, IPC_OPEN), (num), (mode), 0)
#define uart_close(num)                                                                                     ipc_post_exo(HAL_CMD(HAL_UART, IPC_CLOSE), (num), 0, 0)
void uart_set_baudrate(int num, BAUD* baudrate);
#define uart_set_comm_timeouts(num, char_timeout_us, interleaved_timeout_us)                                get_exo(HAL_REQ(HAL_UART, IPC_UART_SET_COMM_TIMEOUTS), (num), (char_timeout_us), (interleaved_timeout_us))
#define uart_flush(num)                                                                                     ipc_post_exo(HAL_CMD(HAL_UART, IPC_FLUSH), (num), 0, 0)
#define uart_get_last_error(num)                                                                            ((int)get_exo(HAL_REQ(HAL_UART, IPC_UART_GET_LAST_ERROR), (num), 0, 0))
#define uart_clear_error(num)                                                                               get_exo(HAL_REQ(HAL_UART, IPC_UART_CLEAR_ERROR), (num), 0, 0)

#define uart_tx(num, io)                                                                                    io_write_exo(HAL_IO_REQ(HAL_UART, IPC_WRITE), (num), (io));
#define uart_rx(num, io, size)                                                                              io_read_exo(HAL_IO_REQ(HAL_UART, IPC_READ), (num), (io), (size));

#endif // UART_H
