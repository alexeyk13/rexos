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
#include "../../../userspace/lpc_driver.h"

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
