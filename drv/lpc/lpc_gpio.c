/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "lpc_gpio.h"
#include "../../userspace/lpc_driver.h"
#include "lpc_config.h"
#include <stdint.h>

#define IOCON                           ((uint32_t*)(LPC_IOCON_BASE))

void lpc_gpio_enable_pin(PIN pin, unsigned int mode)
{
    if (pin >= PIN_DEFAULT)
    {
        error(ERROR_INVALID_PARAMS);
        return;
    }
    IOCON[pin] = mode & LPC_GPIO_MODE_MASK;
    if (mode & LPC_GPIO_MODE_OUT)
        LPC_GPIO->DIR[GPIO_PORT(pin)] |= 1 << GPIO_PIN(pin);
    else
        LPC_GPIO->DIR[GPIO_PORT(pin)] &= ~(1 << GPIO_PIN(pin));
}

__STATIC_INLINE void lpc_gpio_disable_pin(PIN pin)
{
    if (pin >= PIN_DEFAULT)
    {
        error(ERROR_INVALID_PARAMS);
        return;
    }
    //default state, input
    IOCON[pin] = 0;
    LPC_GPIO->DIR[GPIO_PORT(pin)] &= ~(1 << GPIO_PIN(pin));
}

void lpc_gpio_init()
{
    LPC_SYSCON->SYSAHBCLKCTRL |= (1 << SYSCON_SYSAHBCLKCTRL_GPIO_POS) | (1 << SYSCON_SYSAHBCLKCTRL_IOCON_POS);
}

bool lpc_gpio_request(IPC* ipc)
{
    bool need_post = false;
    switch (ipc->cmd)
    {
    case LPC_GPIO_ENABLE_PIN:
        lpc_gpio_enable_pin((PIN)ipc->param1, ipc->param2);
        need_post = true;
        break;
    case LPC_GPIO_DISABLE_PIN:
        lpc_gpio_disable_pin((PIN)ipc->param1);
        need_post = true;
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
        need_post = true;
        break;
    }
    return need_post;
}
