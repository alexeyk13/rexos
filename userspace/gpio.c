/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "gpio.h"

void gpio_enable_pin(unsigned int pin, GPIO_MODE mode)
{
    ipc_post_exo(HAL_CMD(HAL_GPIO, IPC_GPIO_ENABLE_PIN), pin, mode, 0);
}

void gpio_disable_pin(unsigned int pin)
{
    ipc_post_exo(HAL_CMD(HAL_GPIO, IPC_GPIO_DISABLE_PIN), pin, 0, 0);
}

void gpio_set_pin(unsigned int pin)
{
    ipc_post_exo(HAL_CMD(HAL_GPIO, IPC_GPIO_SET_PIN), pin, 0, 0);
}

void gpio_set_mask(unsigned int port, unsigned int mask)
{
    ipc_post_exo(HAL_CMD(HAL_GPIO, IPC_GPIO_SET_MASK), port, mask, 0);
}

void gpio_reset_pin(unsigned int pin)
{
    ipc_post_exo(HAL_CMD(HAL_GPIO, IPC_GPIO_RESET_PIN), pin, 0, 0);
}

void gpio_reset_mask(unsigned int port, unsigned int mask)
{
    ipc_post_exo(HAL_CMD(HAL_GPIO, IPC_GPIO_RESET_MASK), port, mask, 0);
}

bool gpio_get_pin(unsigned int pin)
{
    return get_exo(HAL_REQ(HAL_GPIO, IPC_GPIO_GET_PIN), pin, 0, 0);
}

unsigned int gpio_get_mask(unsigned int port, unsigned int mask)
{
    return get_exo(HAL_REQ(HAL_GPIO, IPC_GPIO_GET_MASK), port, mask, 0);
}

void gpio_set_data_out(unsigned int port, unsigned int mask)
{
    ipc_post_exo(HAL_CMD(HAL_GPIO, IPC_GPIO_SET_DATA_OUT), port, mask, 0);
}

void gpio_set_data_in(unsigned int port, unsigned int mask)
{
    ipc_post_exo(HAL_CMD(HAL_GPIO, IPC_GPIO_SET_DATA_IN), port, mask, 0);
}
