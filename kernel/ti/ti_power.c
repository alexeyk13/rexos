/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "ti_power.h"
#include "ti_exo_private.h"
#include "ti_config.h"
#include "../../userspace/ti/ti.h"
#include "../../userspace/error.h"
#include "../../userspace/process.h"

unsigned int ti_power_get_core_clock()
{
    return 48000000;
}

unsigned int ti_power_get_clock(POWER_CLOCK_TYPE clock_type)
{
    switch (clock_type)
    {
    case POWER_CORE_CLOCK:
        return ti_power_get_core_clock();
    default:
        error(ERROR_NOT_SUPPORTED);
        return 0;
    }
}

void ti_power_init(EXO* exo)
{
    //switch to DC/DC
    AON_SYSCTL->PWRCTL |= AON_SYSCTL_PWRCTL_DCDC_ACTIVE | AON_SYSCTL_PWRCTL_DCDC_EN;

    //TODO: depending on request
    //turn on PERIPH power domain
    PRCM->PDCTL0PERIPH = PRCM_PDCTL0PERIPH_ON;
    while ((PRCM->PDSTAT0PERIPH & PRCM_PDSTAT0PERIPH_ON) == 0) {}

    //TODO: move to pin
    //gate clock for GPIO
    PRCM->GPIOCLKGR = PRCM_GPIOCLKGR_CLK_EN;
    PRCM->CLKLOADCTL = PRCM_CLKLOADCTL_LOAD;
    while ((PRCM->CLKLOADCTL & PRCM_CLKLOADCTL_LOAD_DONE) == 0) {}

#if (TI_UART)
    //turn on SERIAL power domain
    PRCM->PDCTL0SERIAL = PRCM_PDCTL0SERIAL_ON;
    while ((PRCM->PDSTAT0SERIAL & PRCM_PDSTAT0SERIAL_ON) == 0) {}
#endif //TI_UART
}

void ti_power_request(EXO* exo, IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
    case POWER_GET_CLOCK:
        ipc->param2 = ti_power_get_clock(ipc->param1);
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
    }
}

