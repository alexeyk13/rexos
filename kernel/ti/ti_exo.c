/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "ti_exo.h"
#include "ti_exo_private.h"
#include "ti_config.h"
#include "../kernel.h"
#include "../kstdlib.h"
#include "ti_power.h"
#include "ti_pin.h"
#include "ti_uart.h"
#include "ti_timer.h"
#include "ti_rf.h"
#include "../kerror.h"

void exodriver_post(IPC* ipc)
{
    switch (HAL_GROUP(ipc->cmd))
    {
    case HAL_PIN:
        ti_pin_request(__KERNEL->exo, ipc);
        break;
    case HAL_GPIO:
        ti_pin_gpio_request(__KERNEL->exo, ipc);
        break;
    case HAL_POWER:
        ti_power_request(__KERNEL->exo, ipc);
        break;
    case HAL_TIMER:
        ti_timer_request(__KERNEL->exo, ipc);
        break;
#if (TI_UART)
    case HAL_UART:
        ti_uart_request(__KERNEL->exo, ipc);
        break;
#endif //TI_UART
#if (TI_RF)
    case HAL_RF:
        ti_rf_request(__KERNEL->exo, ipc);
        break;
#endif //TI_RF
    default:
        kerror(ERROR_NOT_SUPPORTED);
        break;
    }
}

void exodriver_init()
{
    //ISR disabled at this point
    __KERNEL->exo = kmalloc(sizeof(EXO));
    ti_power_init(__KERNEL->exo);
    ti_pin_init(__KERNEL->exo);
    ti_timer_init(__KERNEL->exo);
#if (TI_UART)
    ti_uart_init(__KERNEL->exo);
#endif //TI_UART
#if (TI_RF)
    ti_rf_init(__KERNEL->exo);
#endif //TI_RF
}
