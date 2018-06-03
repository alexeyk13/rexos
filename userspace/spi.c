/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2018, Alexey Kramarenko
    All rights reserved.
*/

#include "spi.h"
#include "gpio.h"
#include "object.h"
#include "sys_config.h"
#include "core/core.h"

#ifdef EXODRIVERS
bool spi_open(int port,  SPI_MODE* mode)
{
    uint32_t prom = (mode->speed) | ((mode->size-1) << 8);
    if(mode->order == SPI_LSBFIRST) prom |= SPI_LSBFIRST_MSK;
    if(mode->cpol) prom |= SPI_CPOL_MSK;
    if(mode->cpha) prom |= SPI_CPHA_MSK;
    return get_handle_exo(HAL_REQ(HAL_SPI, IPC_OPEN), port, prom, mode->cs_pin) != INVALID_HANDLE;
}

void spi_close(int port)
{
    ipc_post_exo(HAL_CMD(HAL_SPI, IPC_CLOSE), port, 0, 0);
}

bool spi_send(uint32_t port, uint32_t size, uint16_t* data, uint32_t cs_pin)
{
    IPC ipc;
    ipc.process = KERNEL_HANDLE;
    ipc.cmd = HAL_REQ(HAL_SPI, IPC_WRITE);
    if(size>3) return false;
    ipc.param1 = (data[2] << 16) | (size << 8) | port;
    ipc.param2 = data[1];
    ipc.param3 = data[0];
    gpio_reset_pin(cs_pin);
    call(&ipc);
    gpio_set_pin(cs_pin);
    data[0] = ipc.param3;
    if(size > 1)
        data[1] = ipc.param2;
    if(size > 2)
        data[2] = ipc.param1 & 0xFFFF;
    return true;
}


#endif //EXODRIVERS
