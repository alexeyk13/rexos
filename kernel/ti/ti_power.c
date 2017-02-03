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

void ti_power_domain_on(EXO* exo, POWER_DOMAIN domain)
{
    if (exo->power.power_domain_used[domain]++ == 0)
    {
        switch (domain)
        {
        case POWER_DOMAIN_PERIPH:
            PRCM->PDCTL0PERIPH = PRCM_PDCTL0PERIPH_ON;
            while ((PRCM->PDSTAT0PERIPH & PRCM_PDSTAT0PERIPH_ON) == 0) {}
            break;
        case POWER_DOMAIN_SERIAL:
            PRCM->PDCTL0SERIAL = PRCM_PDCTL0SERIAL_ON;
            while ((PRCM->PDSTAT0SERIAL & PRCM_PDSTAT0SERIAL_ON) == 0) {}
            break;
        default:
            break;
        }
    }
}

void ti_power_domain_off(EXO* exo, POWER_DOMAIN domain)
{
    if (--exo->power.power_domain_used[domain] == 0)
    {
        switch (domain)
        {
        case POWER_DOMAIN_PERIPH:
            PRCM->PDCTL0PERIPH = 0;
            while (PRCM->PDSTAT0PERIPH & PRCM_PDSTAT0PERIPH_ON) {}
            break;
        case POWER_DOMAIN_SERIAL:
            PRCM->PDCTL0SERIAL = 0;
            while (PRCM->PDSTAT0SERIAL & PRCM_PDSTAT0SERIAL_ON) {}
            break;
        default:
            break;
        }
    }
}

void ti_power_init(EXO* exo)
{
    //switch to DC/DC
    AON_SYSCTL->PWRCTL |= AON_SYSCTL_PWRCTL_DCDC_ACTIVE | AON_SYSCTL_PWRCTL_DCDC_EN;

    exo->power.power_domain_used[POWER_DOMAIN_PERIPH] = exo->power.power_domain_used[POWER_DOMAIN_SERIAL] = 0;
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

