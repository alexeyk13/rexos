/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef STM32_I2C_H
#define STM32_I2C_H

#include "stm32_exo.h"
#include "../../userspace/io.h"
#include "../../userspace/i2c.h"
#include "../../userspace/process.h"
#include "../../userspace/array.h"
#include <stdbool.h>

typedef struct {
    uint8_t addr;
    uint8_t len;
    uint8_t* data;
}REG;

typedef enum {
    I2C_IO_MODE_IDLE = 0,
    I2C_IO_MODE_TX,
    I2C_IO_MODE_RX,
    I2C_IO_MODE_SLAVE
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
#ifdef STM32F1
    uint32_t pin_scl;
    uint32_t pin_sda;
#endif
// only for slave
    ARRAY* regs;
    uint8_t reg_addr;
    uint8_t* reg_data;
    uint8_t reg_len;
} I2C;

typedef struct  {
    I2C* i2cs[I2C_COUNT];
} I2C_DRV;

void stm32_i2c_init(EXO* exo);
void stm32_i2c_request(EXO* exo, IPC* ipc);

#endif // STM32_I2C_H
