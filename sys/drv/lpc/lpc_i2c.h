/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef LPC_I2C_H
#define LPC_I2C_H

#include "lpc_config.h"
#include "../../../userspace/sys.h"
#if (MONOLITH_I2C)
#include "lpc_core.h"
#endif

typedef enum {
    I2C_0,
    I2C_1
} I2C_PORT;

#define I2C_MASTER                  (1 << 8)
#define I2C_SLAVE                   (0 << 8)

#define I2C_NORMAL_SPEED            (0 << 9)
#define I2C_FAST_SPEED              (1 << 9)

//size of address. If 0, no Rs condition will be used. MSB goes first
#define I2C_ADDR_SIZE_POS           0
#define I2C_ADDR_SIZE_MASK          (0xf << 0)

typedef enum {
    I2C_IO_IDLE = 0,
    I2C_IO_TX,
    I2C_IO_RX
} I2C_IO;

typedef struct  {
    HANDLE block, process;
    char* ptr;
    I2C_IO io;
    unsigned int addr;
    uint16_t mode;
    uint16_t processed, size;
    uint8_t sla, addr_processed;
} I2C;

typedef struct  {
    I2C* i2cs[I2C_COUNT];
} I2C_DRV;

#if (MONOLITH_I2C)
#define SHARED_I2C_DRV                    CORE
#else

typedef struct {
    I2C_DRV i2c;
} SHARED_I2C_DRV;

#endif


#if (MONOLITH_I2C)
void lpc_i2c_init(SHARED_I2C_DRV* drv);
bool lpc_i2c_request(SHARED_I2C_DRV* drv, IPC* ipc);
#else
extern const REX __LPC_I2C;
#endif

#endif // LPC_I2C_H
