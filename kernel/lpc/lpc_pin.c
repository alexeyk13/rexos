/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "lpc_pin.h"
#include "../../userspace/lpc/lpc_driver.h"
#include "../kerror.h"
#include "lpc_config.h"
#include <stdint.h>

#ifdef LPC11Uxx
#define IOCON                           ((uint32_t*)(LPC_IOCON_BASE))
#else
#define SCU                             ((uint32_t*)(LPC_SCU_BASE))
#endif //LPC11Uxx

void lpc_pin_enable(unsigned int pin, unsigned int mode)
{
    if (pin >= PIN_MAX)
    {
        kerror(ERROR_INVALID_PARAMS);
        return;
    }
#ifdef LPC11Uxx
    IOCON[pin] = mode;
#else //LPC18xx
    SCU[pin] = mode;
#endif //LPC11Uxx
}

void lpc_pin_disable(unsigned int pin)
{
    if (pin >= PIN_MAX)
    {
        kerror(ERROR_INVALID_PARAMS);
        return;
    }
    //default state, input
#ifdef LPC11Uxx
    IOCON[pin] = 0;
#else //LPC18xx
    SCU[pin] = 0;
#endif //LPC11Uxx
}

void lpc_pin_init()
{
#ifdef LPC11Uxx
    LPC_SYSCON->SYSAHBCLKCTRL |= (1 << SYSCON_SYSAHBCLKCTRL_GPIO_POS) | (1 << SYSCON_SYSAHBCLKCTRL_IOCON_POS);
#endif
    //all clocks on 18xx are enabled by default
}

void lpc_pin_request(IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_OPEN:
        lpc_pin_enable(ipc->param1, ipc->param2);
        break;
    case IPC_CLOSE:
        lpc_pin_disable(ipc->param1);
        break;
    default:
        kerror(ERROR_NOT_SUPPORTED);
        break;
    }
}
