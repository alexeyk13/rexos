/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef I2C_H
#define I2C_H

#include <stdint.h>
#include "io.h"

typedef enum {
    I2C_SET_REGISTER = IPC_USER
} I2C_IPCS;

typedef struct {
    uint8_t sla, addr, flags;
} I2C_STACK;

typedef struct {
    uint8_t addr;
} I2C_SLAVE_STACK;

#define I2C_NORMAL_CLOCK                 100000
#define I2C_FAST_CLOCK                   400000
#define I2C_FAST_PLUS_CLOCK              1000000

#define I2C_MODE_MASTER                  (1 << 7)
#define I2C_MODE_SLAVE                   (0 << 7)
#define I2C_MODE                         (1 << 7)
#define I2C_SLAVE_ADDR                   (0x7F)
#define I2C_SLAVE_ADDR_POS               0
#define I2C_REG_COUNT                    (0xFF << 8)
#define I2C_REG_EMPTY                    (0xFF)

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

void i2c_read_slave(int port, IO* io, uint32_t max_size);
void i2c_set_register(int port, IO* io, uint8_t addr);
void i2c_clear_register(int port, uint8_t addr);

#endif // I2C_H
