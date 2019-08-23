/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: RJ (jam_roma@yahoo.com)
*/

#include "nrf_pin.h"
#include "nrf_exo_private.h"
#include "sys_config.h"
#include <string.h>
#include "../kerror.h"

void nrf_pin_init(EXO* exo)
{
    memset(&exo->gpio, 0, sizeof (GPIO_DRV));
}

void nrf_gpio_enable_pin(GPIO_DRV* gpio, PIN pin, PIN_MODE mode, PIN_PULL pull)
{
    gpio->used_pins[pin]++;

    NRF_GPIO->PIN_CNF[pin] = (GPIO_PIN_CNF_SENSE_Disabled << GPIO_PIN_CNF_SENSE_Pos) | \
                            (GPIO_PIN_CNF_DRIVE_S0S1 << GPIO_PIN_CNF_DRIVE_Pos)  | \
                            (GPIO_PIN_CNF_PULL_Disabled << GPIO_PIN_CNF_PULL_Pos) | \
                            (GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos);

    /* set up pull up config */
    NRF_GPIO->PIN_CNF[pin] |= (pull << GPIO_PIN_CNF_PULL_Pos);

    if(PIN_MODE_INPUT == mode)
    {
        NRF_GPIO->DIRSET &= ~(1 << pin);
        NRF_GPIO->PIN_CNF[pin] |= (GPIO_PIN_CNF_DIR_Input << GPIO_PIN_CNF_DIR_Pos);
    }
    else
    {
        NRF_GPIO->DIRSET |= (1 << pin);
        NRF_GPIO->PIN_CNF[pin] |= (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos);
    }
}

void nrf_gpio_disable_pin(GPIO_DRV* gpio, PIN pin)
{
    --gpio->used_pins[pin];
}

void nrf_pin_request(EXO* exo, IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_CLOSE:
        nrf_gpio_disable_pin(&exo->gpio, (PIN)ipc->param1);
        break;
    case IPC_OPEN:
        nrf_gpio_enable_pin(&exo->gpio, (PIN)ipc->param1, ipc->param2, ipc->param3);
        break;
    default:
        kerror(ERROR_NOT_SUPPORTED);
        break;
    }
}
