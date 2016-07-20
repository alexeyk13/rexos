/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2016, Alexey Kramarenko
    All rights reserved.
*/

#ifndef STM32_I2C_H
#define STM32_I2C_H

#include "stm32_core.h"
#include "../../userspace/io.h"
#include "../../userspace/i2c.h"
#include "../../userspace/process.h"
#include <stdbool.h>

typedef enum {
    I2C_IO_MODE_IDLE = 0,
    I2C_IO_MODE_TX,
    I2C_IO_MODE_RX
} I2C_IO_MODE;

typedef enum {
    I2C_STATE_ADDR = 0,
    I2C_STATE_LEN,
    I2C_STATE_DATA
} I2C_STATE;

typedef struct  {
    IO* io;
    I2C_STACK* stack;
    HANDLE process;
    I2C_IO_MODE io_mode;
    I2C_STATE state;
    unsigned int size;
} I2C;

typedef struct  {
    I2C* i2cs[I2C_COUNT];
} I2C_DRV;

void stm32_i2c_init(CORE* core);
void stm32_i2c_request(CORE* core, IPC* ipc);

#endif // STM32_I2C_H
