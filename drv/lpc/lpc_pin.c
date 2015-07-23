/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "lpc_pin.h"
#include "../../userspace/lpc/lpc_driver.h"
#include "lpc_config.h"
#include <stdint.h>

#ifdef LPC11Uxx
#define IOCON                           ((uint32_t*)(LPC_IOCON_BASE))
#else
#define SCU                             ((uint32_t*)(LPC_SCU_BASE))
#endif //LPC11Uxx

void lpc_pin_enable(PIN pin, unsigned int mode)
{
    if (pin >= PIN_MAX)
    {
        error(ERROR_INVALID_PARAMS);
        return;
    }
#ifdef LPC11Uxx
    IOCON[pin] = mode;
#else //LPC18xx
    SCU[pin] = mode;
#endif //LPC11Uxx
}

__STATIC_INLINE void lpc_pin_disable(PIN pin)
{
    if (pin >= PIN_MAX)
    {
        error(ERROR_INVALID_PARAMS);
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

bool lpc_pin_request(IPC* ipc)
{
    bool need_post = false;
    switch (HAL_ITEM(ipc->cmd))
    {
    case LPC_PIN_ENABLE:
        lpc_pin_enable((PIN)ipc->param1, ipc->param2);
        need_post = true;
        break;
    case LPC_PIN_DISABLE:
        lpc_pin_disable((PIN)ipc->param1);
        need_post = true;
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
        need_post = true;
        break;
    }
    return need_post;
}
