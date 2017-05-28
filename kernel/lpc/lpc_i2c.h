/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef LPC_I2C_H
#define LPC_I2C_H

#include "lpc_config.h"
#include "lpc_exo.h"
#include "../../userspace/ipc.h"
#include "../../userspace/io.h"
#include "../../userspace/lpc/lpc_driver.h"
#include "../../userspace/i2c.h"

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
#if (LPC_I2C_TIMEOUT_MS)
    unsigned int cc;
    HANDLE timer;
    bool timer_pending;
#endif
    I2C_IO_MODE io_mode;
    I2C_STATE state;
    unsigned int size, processed;
} I2C;

typedef struct  {
    I2C* i2cs[I2C_COUNT];
} I2C_DRV;

void lpc_i2c_init(EXO* exo);
void lpc_i2c_request(EXO* exo, IPC* ipc);

#endif // LPC_I2C_H
