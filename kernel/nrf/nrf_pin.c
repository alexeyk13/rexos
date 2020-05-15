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

typedef volatile uint32_t* REG_PTR;

static const  REG_PTR PIN_PERIF_TBL[]={
        &NRF_UART0->PSELRXD, &NRF_UART0->PSELTXD, &NRF_UART0->PSELRTS, &NRF_UART0->PSELCTS,
        &NRF_SPIM0->PSEL.SCK, &NRF_SPIM0->PSEL.MOSI, &NRF_SPIM0->PSEL.MISO,
        &NRF_SPIM1->PSEL.SCK, &NRF_SPIM1->PSEL.MOSI, &NRF_SPIM1->PSEL.MISO,
        &NRF_SPIM2->PSEL.SCK, &NRF_SPIM2->PSEL.MOSI, &NRF_SPIM2->PSEL.MISO,

        &NRF_PWM0->PSEL.OUT[0], &NRF_PWM0->PSEL.OUT[1], &NRF_PWM0->PSEL.OUT[2], &NRF_PWM0->PSEL.OUT[3],
        &NRF_PWM1->PSEL.OUT[0], &NRF_PWM1->PSEL.OUT[1], &NRF_PWM1->PSEL.OUT[2], &NRF_PWM1->PSEL.OUT[3],
        &NRF_PWM2->PSEL.OUT[0], &NRF_PWM2->PSEL.OUT[1], &NRF_PWM2->PSEL.OUT[2], &NRF_PWM2->PSEL.OUT[3],

};

#define PIN_PERIF_TBL_LEN   (sizeof(PIN_PERIF_TBL)/sizeof(uint32_t*))

void nrf_pin_init(EXO* exo)
{
    memset(&exo->gpio, 0, sizeof (GPIO_DRV));
}

void nrf_gpio_enable_pin(GPIO_DRV* gpio, PIN pin, PIN_MODE mode, uint32_t mode2)
{
    switch (mode)
    {
    case PIN_MODE_GENERAL:
        NRF_GPIO->PIN_CNF[pin] = mode2;
        break;
    case PIN_MODE_PERIF:
        if(mode2 < PIN_PERIF_TBL_LEN)
            *(PIN_PERIF_TBL[mode2]) = pin;
        else
            return;
        break;
    default:
        return;
    }

    gpio->used_pins[pin]++;
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
