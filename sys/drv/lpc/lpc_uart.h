/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef LPC_UART_H
#define LPC_UART_H

#include <stdint.h>
#include "../../sys.h"

//TODO: adjust for driver
typedef struct {
    uint8_t tx_pin, rx_pin;
    uint16_t error;
//    HANDLE tx_stream, tx_handle;
//    uint16_t tx_total, tx_chunk_pos, tx_chunk_size;
//    HANDLE rx_stream, rx_handle;
//    uint16_t rx_free;
} UART;

typedef struct {
    //TODO: '*'
    UART uarts[UARTS_COUNT];
} UART_DRV;


//TODO: move to uart.h
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
    UART_0 = 0,
    UART_1,
    UART_2,
    UART_3,
    UART_4,
    UART_MAX
}UART_PORT;

typedef struct {
    uint8_t tx, rx;
    uint16_t stream_size;
    BAUD baud;
} UART_ENABLE;

void lpc_uart_init(UART_DRV* drv);
void lpc_uart_open_internal(UART_DRV *drv, UART_PORT port, UART_ENABLE *ue);

void uart_write_kernel(const char *const buf, unsigned int size, void* param);

#endif // LPC_UART_H
