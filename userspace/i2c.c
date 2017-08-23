/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "i2c.h"
#include "core/core.h"

#ifdef STM32F1
#define PIN_ENCODE_MASTER(scl, sda) ((scl << 16) | (sda << 24) | I2C_MODE_MASTER)
#define PIN_ENCODE_SLAVE(scl, sda)  ((scl << 16) | (sda << 24) | I2C_MODE_SLAVE)

bool i2c_open_master(int port, uint32_t speed, uint32_t pin_scl, uint32_t pin_sda)
{
    return get_handle_exo(HAL_REQ(HAL_I2C, IPC_OPEN), port, PIN_ENCODE_MASTER(pin_scl, pin_sda), speed) != INVALID_HANDLE;
}
bool i2c_open_slave(int port, uint32_t addr,  uint32_t speed, uint32_t pin_scl, uint32_t pin_sda)
{
    return get_handle_exo(HAL_REQ(HAL_I2C, IPC_OPEN), port, PIN_ENCODE_SLAVE(pin_scl, pin_sda) | addr , speed) != INVALID_HANDLE;
}

#else

bool i2c_open(int port, unsigned int mode, unsigned int speed)
{
    return get_handle_exo(HAL_REQ(HAL_I2C, IPC_OPEN), port, mode, speed) != INVALID_HANDLE;
}

#endif // STM32F1
void i2c_set_register(int port, IO* io,  uint8_t addr)
{
    ipc_post_exo(HAL_REQ(HAL_I2C, I2C_SET_REGISTER), port, (uint32_t)io, addr);
}

void i2c_clear_register(int port, uint8_t addr)
{
    ipc_post_exo(HAL_CMD(HAL_I2C, I2C_SET_REGISTER), port, 0, addr);
}

int i2c_read_slave(int port, IO* io, uint32_t max_size)
{
    return io_read_sync_exo(HAL_IO_REQ(HAL_I2C, IPC_READ), port, io, max_size);
}

void i2c_close(int port)
{
    ipc_post_exo(HAL_CMD(HAL_I2C, IPC_CLOSE), port, 0, 0);
}

int i2c_read(int port, uint8_t sla, IO* io, unsigned int max_size)
{
    int res;
    I2C_STACK* stack = io_push(io, sizeof(I2C_STACK));
    stack->sla = sla;
    stack->flags = 0;
    res = io_read_sync_exo(HAL_IO_REQ(HAL_I2C, IPC_READ), port, io, max_size);
    io_pop(io, sizeof(I2C_STACK));
    return res;
}

int i2c_read_len(int port, uint8_t sla, IO* io, unsigned int max_size)
{
    int res;
    I2C_STACK* stack = io_push(io, sizeof(I2C_STACK));
    stack->sla = sla;
    stack->flags = I2C_FLAG_LEN;
    res = io_read_sync_exo(HAL_IO_REQ(HAL_I2C, IPC_READ), port, io, max_size);
    io_pop(io, sizeof(I2C_STACK));
    return res;
}

int i2c_read_addr(int port, uint8_t sla, uint8_t addr, IO* io, unsigned int max_size)
{
    int res;
    I2C_STACK* stack = io_push(io, sizeof(I2C_STACK));
    stack->addr = addr;
    stack->sla = sla;
    stack->flags = I2C_FLAG_ADDR;
    res = io_read_sync_exo(HAL_IO_REQ(HAL_I2C, IPC_READ), port, io, max_size);
    io_pop(io, sizeof(I2C_STACK));
    return res;
}

int i2c_read_addr_len(int port, uint8_t sla, uint8_t addr, IO* io, unsigned int max_size)
{
    int res;
    I2C_STACK* stack = io_push(io, sizeof(I2C_STACK));
    stack->addr = addr;
    stack->sla = sla;
    stack->flags = I2C_FLAG_ADDR | I2C_FLAG_LEN;
    res = io_read_sync_exo(HAL_IO_REQ(HAL_I2C, IPC_READ), port, io, max_size);
    io_pop(io, sizeof(I2C_STACK));
    return res;
}

int i2c_write(int port, uint8_t sla, IO* io)
{
    int res;
    I2C_STACK* stack = io_push(io, sizeof(I2C_STACK));
    stack->sla = sla;
    stack->flags = 0;
    res = io_write_sync_exo(HAL_IO_REQ(HAL_I2C, IPC_WRITE), port, io);
    io_pop(io, sizeof(I2C_STACK));
    return res;
}

int i2c_write_len(int port, uint8_t sla, IO* io)
{
    int res;
    I2C_STACK* stack = io_push(io, sizeof(I2C_STACK));
    stack->sla = sla;
    stack->flags = I2C_FLAG_LEN;
    res = io_write_sync_exo(HAL_IO_REQ(HAL_I2C, IPC_WRITE), port, io);
    io_pop(io, sizeof(I2C_STACK));
    return res;
}

int i2c_write_addr(int port, uint8_t sla, uint8_t addr, IO* io)
{
    int res;
    I2C_STACK* stack = io_push(io, sizeof(I2C_STACK));
    stack->addr = addr;
    stack->sla = sla;
    stack->flags = I2C_FLAG_ADDR;
    res = io_write_sync_exo(HAL_IO_REQ(HAL_I2C, IPC_WRITE), port, io);
    io_pop(io, sizeof(I2C_STACK));
    return res;
}

int i2c_write_addr_len(int port, uint8_t sla, uint8_t addr, IO* io)
{
    int res;
    I2C_STACK* stack = io_push(io, sizeof(I2C_STACK));
    stack->addr = addr;
    stack->sla = sla;
    stack->flags = I2C_FLAG_ADDR | I2C_FLAG_LEN;
    res = io_write_sync_exo(HAL_IO_REQ(HAL_I2C, IPC_WRITE), port, io);
    io_pop(io, sizeof(I2C_STACK));
    return res;
}

