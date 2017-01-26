/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
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
    IPC_UART_MAX,
} UART_IPCS;

#define UART_RX_STREAM                          (1 << 0)
#define UART_TX_STREAM                          (1 << 1)
#define UART_MODE                               (3 << 2)
#define UART_MODE_STREAM                        (0 << 2)
#define UART_MODE_IO                            (1 << 2)
#define UART_MODE_ISO7816                       (2 << 2)

//used internally by driver
void uart_encode_baudrate(BAUD* baudrate, IPC* ipc);
void uart_decode_baudrate(IPC* ipc, BAUD* baudrate);
//dbg related
void uart_setup_printk(int num);
void uart_setup_stdout(int num);
void uart_setup_stdin(int num);

bool uart_open(int num, unsigned int mode);
void uart_close(int num);
void uart_set_baudrate(int num, BAUD* baudrate);
void uart_flush(int num);

#endif // UART_H
