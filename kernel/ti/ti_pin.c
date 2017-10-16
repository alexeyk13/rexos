/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "ti_pin.h"
#include "ti_exo_private.h"
#include "ti_power.h"
#include "../../userspace/ti/ti.h"
#include "../kerror.h"
#include "../../userspace/process.h"
#include "../../userspace/gpio.h"

void ti_pin_enable(EXO* exo, DIO dio, unsigned int mode, bool output)
{
    if (dio >= DIO_MAX)
    {
        kerror(ERROR_INVALID_PARAMS);
        return;
    }
    //enable gating
    if (exo->pin.pin_used++ == 0)
    {
        ti_power_domain_on(exo, POWER_DOMAIN_PERIPH);

        //gate clock for GPIO
        PRCM->GPIOCLKGR = PRCM_GPIOCLKGR_CLK_EN;
        PRCM->CLKLOADCTL = PRCM_CLKLOADCTL_LOAD;
        while ((PRCM->CLKLOADCTL & PRCM_CLKLOADCTL_LOAD_DONE) == 0) {}
    }
    IOC->IOCFG[dio] = mode;
    if (output)
        GPIO->DOE |= (1 << dio);
}

void ti_pin_disable(EXO* exo, DIO dio)
{
    if (dio >= DIO_MAX)
    {
        kerror(ERROR_INVALID_PARAMS);
        return;
    }
    GPIO->DOE &= ~(1 << dio);
    IOC->IOCFG[dio] = IOC_IOCFG_PULL_CTL_DIS;

    //disable gating
    if (--exo->pin.pin_used == 0)
    {
        //gate clock for GPIO
        PRCM->GPIOCLKGR &= ~PRCM_GPIOCLKGR_CLK_EN;
        PRCM->CLKLOADCTL = PRCM_CLKLOADCTL_LOAD;
        while ((PRCM->CLKLOADCTL & PRCM_CLKLOADCTL_LOAD_DONE) == 0) {}

        ti_power_domain_off(exo, POWER_DOMAIN_PERIPH);
    }
}

void ti_gpio_enable_pin(EXO* exo, DIO dio, GPIO_MODE mode)
{
    switch (mode)
    {
    case GPIO_MODE_IN_FLOAT:
        ti_pin_enable(exo, dio, IOC_IOCFG_IE | IOC_IOCFG_PULL_CTL_DIS | IOC_IOCFG_SLEW_RED | IOC_IOCFG_PORT_ID_GPIO, false);
        break;
    case GPIO_MODE_IN_PULLUP:
        ti_pin_enable(exo, dio, IOC_IOCFG_IE | IOC_IOCFG_PULL_CTL_UP | IOC_IOCFG_SLEW_RED | IOC_IOCFG_PORT_ID_GPIO, false);
        break;
    case GPIO_MODE_IN_PULLDOWN:
        ti_pin_enable(exo, dio, IOC_IOCFG_IE | IOC_IOCFG_PULL_CTL_DOWN | IOC_IOCFG_SLEW_RED | IOC_IOCFG_PORT_ID_GPIO, false);
        break;
    //GPIO_MODE_OUT:
    default:
        ti_pin_enable(exo, dio, IOC_IOCFG_PULL_CTL_DIS | IOC_IOCFG_SLEW_RED | IOC_IOCFG_PORT_ID_GPIO, true);
    }
}

void ti_gpio_set_pin(DIO dio)
{
    if (dio >= DIO_MAX)
    {
        kerror(ERROR_INVALID_PARAMS);
        return;
    }
    GPIO->DOUTSET = (1 << dio);
}

void ti_gpio_set_mask(unsigned int port, unsigned int mask)
{
    GPIO->DOUTSET = mask;
}

void ti_gpio_reset_pin(DIO dio)
{
    if (dio >= DIO_MAX)
    {
        kerror(ERROR_INVALID_PARAMS);
        return;
    }
    GPIO->DOUTCLR = (1 << dio);
}

void ti_gpio_reset_mask(unsigned int port, unsigned int mask)
{
    GPIO->DOUTCLR = mask;
}

bool ti_gpio_get_pin(DIO dio)
{
    if (dio >= DIO_MAX)
    {
        kerror(ERROR_INVALID_PARAMS);
        return false;
    }
    return (GPIO->DIN >> dio) & 1;
}

unsigned int ti_gpio_get_mask(unsigned int port, unsigned int mask)
{
    return GPIO->DIN & mask;
}

void ti_gpio_set_data_out(unsigned int port, unsigned int mask)
{
    GPIO->DOE |= mask;
}

void ti_gpio_set_data_in(unsigned int port, unsigned int mask)
{
    GPIO->DOE &= ~mask;
}

void ti_pin_init(EXO* exo)
{
    exo->pin.pin_used = 0;
}

void ti_pin_request(EXO* exo, IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_OPEN:
        ti_pin_enable(exo, (DIO)ipc->param1, ipc->param2, ipc->param3);
        break;
    case IPC_CLOSE:
        ti_pin_disable(exo, (DIO)ipc->param1);
    default:
        kerror(ERROR_NOT_SUPPORTED);
        break;
    }
}

void ti_pin_gpio_request(EXO* exo, IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_GPIO_ENABLE_PIN:
        ti_gpio_enable_pin(exo, (DIO)ipc->param1, (GPIO_MODE)ipc->param2);
        break;
    case IPC_GPIO_DISABLE_PIN:
        ti_pin_disable(exo, (DIO)ipc->param1);
        break;
    case IPC_GPIO_SET_PIN:
        ti_gpio_set_pin((DIO)ipc->param1);
        break;
    case IPC_GPIO_SET_MASK:
        ti_gpio_set_mask(ipc->param1, ipc->param2);
        break;
    case IPC_GPIO_RESET_PIN:
        ti_gpio_reset_pin((DIO)ipc->param1);
        break;
    case IPC_GPIO_RESET_MASK:
        ti_gpio_reset_mask(ipc->param1, ipc->param2);
        break;
    case IPC_GPIO_GET_PIN:
        ipc->param2 = ti_gpio_get_pin((DIO)ipc->param1);
        break;
    case IPC_GPIO_GET_MASK:
        ipc->param2 = ti_gpio_get_mask(ipc->param1, ipc->param2);
        break;
    case IPC_GPIO_SET_DATA_OUT:
        ti_gpio_set_data_out(ipc->param1, ipc->param2);
        break;
    case IPC_GPIO_SET_DATA_IN:
        ti_gpio_set_data_in(ipc->param1, ipc->param2);
        break;
    default:
        kerror(ERROR_NOT_SUPPORTED);
        break;
    }
}
