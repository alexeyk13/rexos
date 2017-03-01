/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef TI_DRIVER_H
#define TI_DRIVER_H

#include "../ipc.h"

//------------------------------------------------- GPIO ---------------------------------------------------------------------
typedef enum {
    DIO0 = 0, DIO1,  DIO2,  DIO3,  DIO4,  DIO5,  DIO6,  DIO7,  DIO8,  DIO9,  DIO10, DIO11, DIO12, DIO13, DIO14, DIO15,
    DIO16,    DIO17, DIO18, DIO19, DIO20, DIO21, DIO22, DIO23, DIO24, DIO25, DIO26, DIO27, DIO28, DIO29, DIO30, DIO31,
    DIO_MAX
} DIO;

//------------------------------------------------ Timer ---------------------------------------------------------------------

typedef enum {
    TIMER_GPT0 = 0,
    TIMER_GPT1,
    TIMER_GPT2,
    TIMER_GPT3,
    TIMER_MAX
} TIMER;

//-------------------------------------------------- I2C ---------------------------------------------------------------------
typedef enum {
    I2C_0,
    I2C_MAX
} I2C_PORT;

//------------------------------------------------- UART ---------------------------------------------------------------------
typedef enum {
    UART_0 = 0,
    UART_MAX
} UART_PORT;

//-------------------------------------------------- RF ----------------------------------------------------------------------
typedef enum {
    IPC_RF_GET_FW_VERSION = IPC_USER,
    IPC_RF_POWER_UP,
    IPC_RF_POWER_DOWN,
    IPC_RF_SET_TX_POWER,
    IPC_RF_GET_RSSI,
    IPC_RF_MAX
} RF_IPCS;

#endif // TI_DRIVER_H
