/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef UART_STM32_H
#define UART_STM32_H

/*
        UART driver. Hardware-independent part
  */

#include "types.h"
#include "dev.h"

//UART line status
typedef enum {
    UART_ERROR_OK = 0,
    UART_ERROR_OVERRUN,
    UART_ERROR_NOISE,
    UART_ERROR_FRAME,
    UART_ERROR_PARITY
}UART_ERROR;

#define UART_MODE_RX_ENABLE                (1 << 0)
#define UART_MODE_TX_ENABLE                (1 << 1)
#define UART_MODE_TX_COMPLETE_ENABLE    (1 << 2)

typedef struct {
    //baudrate
    uint32_t baud;
    //data bits: 7, 8
    uint8_t data_bits;
    //parity: 'N', 'O', 'E'
    char parity;
    //stop bits: 1, 2
    uint8_t stop_bits;
}UART_BAUD;

typedef struct {
    void (*on_read_complete)(void* param);
    void (*on_write_complete)(void* param);
    void (*on_error)(void* param, UART_ERROR error);
}UART_CB, *P_UART_CB;

typedef struct {
    UART_CB* cb;
    char* read_buf;
    char* write_buf;
    unsigned int read_size, write_size;
    void* param;
    bool isr_active;
}UART_HW;

extern void uart_enable(UART_CLASS port, UART_CB* cb, void* param, int priority);
extern void uart_disable(UART_CLASS port);
extern void uart_set_baudrate(UART_CLASS port, const UART_BAUD* config);

extern void uart_write(UART_CLASS port, char* buf, int size);
extern void uart_write_wait(UART_CLASS port);
extern void uart_read(UART_CLASS port, char* buf, int size);
extern void uart_read_cancel(UART_CLASS port);

void uart_write_svc(const char *const buf, unsigned int size, void* param);

#endif // UART_STM32_H
