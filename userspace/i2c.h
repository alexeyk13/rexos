/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2016, Alexey Kramarenko
    All rights reserved.
*/

#ifndef I2C_H
#define I2C_H

#include <stdint.h>
#include "io.h"

typedef struct {
    uint8_t sla, addr, flags;
} I2C_STACK;

#define I2C_NORMAL_CLOCK                 100000
#define I2C_FAST_CLOCK                   400000
#define I2C_FAST_PLUS_CLOCK              1000000


#define I2C_MODE_MASTER                  (1 << 0)
#define I2C_MODE_SLAVE                   (0 << 0)

//set address during transfer
#define I2C_FLAG_ADDR                    (1 << 0)
//First received byte is len. Used is some smartcard transfsers
#define I2C_FLAG_LEN                     (1 << 1)

bool i2c_open(int port, unsigned int mode, unsigned int speed);
void i2c_close(int port);
int i2c_read(int port, uint8_t sla, IO* io, unsigned int max_size);
int i2c_read_len(int port, uint8_t sla, IO* io, unsigned int max_size);
int i2c_read_addr(int port, uint8_t sla, uint8_t addr, IO* io, unsigned int max_size);
int i2c_read_addr_len(int port, uint8_t sla, uint8_t addr, IO* io, unsigned int max_size);
int i2c_write(int port, uint8_t sla, IO* io);
int i2c_write_len(int port, uint8_t sla, IO* io);
int i2c_write_addr(int port, uint8_t sla, uint8_t addr, IO* io);
int i2c_write_addr_len(int port, uint8_t sla, uint8_t addr, IO* io);

#endif // I2C_H
