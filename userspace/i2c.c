/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2016, Alexey Kramarenko
    All rights reserved.
*/

#include "i2c.h"
#include "object.h"
#include "sys_config.h"

bool i2c_open(I2C_PORT port, unsigned int mode, unsigned int speed)
{
    return get_handle(object_get(SYS_OBJ_CORE), HAL_REQ(HAL_I2C, IPC_OPEN), port, mode, speed) != INVALID_HANDLE;
}

void i2c_close(I2C_PORT port)
{
    ack(object_get(SYS_OBJ_CORE), HAL_REQ(HAL_I2C, IPC_CLOSE), port, 0, 0);
}

int i2c_read(I2C_PORT port, uint8_t sla, IO* io, unsigned int max_size)
{
    int res;
    I2C_STACK* stack = io_push(io, sizeof(I2C_STACK));
    stack->sla = sla;
    stack->flags = 0;
    res = io_read_sync(object_get(SYS_OBJ_CORE), HAL_IO_REQ(HAL_I2C, IPC_READ), port, io, max_size);
    io_pop(io, sizeof(I2C_STACK));
    return res;
}

int i2c_read_len(I2C_PORT port, uint8_t sla, IO* io, unsigned int max_size)
{
    int res;
    I2C_STACK* stack = io_push(io, sizeof(I2C_STACK));
    stack->sla = sla;
    stack->flags = I2C_FLAG_LEN;
    res = io_read_sync(object_get(SYS_OBJ_CORE), HAL_IO_REQ(HAL_I2C, IPC_READ), port, io, max_size);
    io_pop(io, sizeof(I2C_STACK));
    return res;
}

int i2c_read_addr(I2C_PORT port, uint8_t sla, uint8_t addr, IO* io, unsigned int max_size)
{
    int res;
    I2C_STACK* stack = io_push(io, sizeof(I2C_STACK));
    stack->addr = addr;
    stack->sla = sla;
    stack->flags = I2C_FLAG_ADDR;
    res = io_read_sync(object_get(SYS_OBJ_CORE), HAL_IO_REQ(HAL_I2C, IPC_READ), port, io, max_size);
    io_pop(io, sizeof(I2C_STACK));
    return res;
}

int i2c_read_addr_len(I2C_PORT port, uint8_t sla, uint8_t addr, IO* io, unsigned int max_size)
{
    int res;
    I2C_STACK* stack = io_push(io, sizeof(I2C_STACK));
    stack->addr = addr;
    stack->sla = sla;
    stack->flags = I2C_FLAG_ADDR | I2C_FLAG_LEN;
    res = io_read_sync(object_get(SYS_OBJ_CORE), HAL_IO_REQ(HAL_I2C, IPC_READ), port, io, max_size);
    io_pop(io, sizeof(I2C_STACK));
    return res;
}

int i2c_write(I2C_PORT port, uint8_t sla, IO* io)
{
    int res;
    I2C_STACK* stack = io_push(io, sizeof(I2C_STACK));
    stack->sla = sla;
    stack->flags = 0;
    res = io_write_sync(object_get(SYS_OBJ_CORE), HAL_IO_REQ(HAL_I2C, IPC_WRITE), port, io);
    io_pop(io, sizeof(I2C_STACK));
    return res;
}

int i2c_write_len(I2C_PORT port, uint8_t sla, IO* io)
{
    int res;
    I2C_STACK* stack = io_push(io, sizeof(I2C_STACK));
    stack->sla = sla;
    stack->flags = I2C_FLAG_LEN;
    res = io_write_sync(object_get(SYS_OBJ_CORE), HAL_IO_REQ(HAL_I2C, IPC_WRITE), port, io);
    io_pop(io, sizeof(I2C_STACK));
    return res;
}

int i2c_write_addr(I2C_PORT port, uint8_t sla, uint8_t addr, IO* io)
{
    int res;
    I2C_STACK* stack = io_push(io, sizeof(I2C_STACK));
    stack->addr = addr;
    stack->sla = sla;
    stack->flags = I2C_FLAG_ADDR;
    res = io_write_sync(object_get(SYS_OBJ_CORE), HAL_IO_REQ(HAL_I2C, IPC_WRITE), port, io);
    io_pop(io, sizeof(I2C_STACK));
    return res;
}

int i2c_write_addr_len(I2C_PORT port, uint8_t sla, uint8_t addr, IO* io)
{
    int res;
    I2C_STACK* stack = io_push(io, sizeof(I2C_STACK));
    stack->addr = addr;
    stack->sla = sla;
    stack->flags = I2C_FLAG_ADDR | I2C_FLAG_LEN;
    res = io_write_sync(object_get(SYS_OBJ_CORE), HAL_IO_REQ(HAL_I2C, IPC_WRITE), port, io);
    io_pop(io, sizeof(I2C_STACK));
    return res;
}
