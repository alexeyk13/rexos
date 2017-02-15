/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "ti_rf.h"
#include "ti_exo_private.h"
#include "ti_rf_private.h"
#include "../../userspace/process.h"
#include "../../userspace/error.h"

void ti_rf_init(EXO* exo)
{
    exo->rf.active = false;
}

static inline void ti_rf_open(EXO* exo)
{
    if (exo->rf.active)
        return;

    //power up RFC domain
    PRCM->PDCTL1RFC = PRCM_PDCTL1RFC_ON;
    while ((PRCM->PDSTAT1RFC & PRCM_PDSTAT1RFC_ON) == 0) {}

    //gate clock for RFC
    PRCM->RFCCLKG = PRCM_RFCCLKG_CLK_EN;
    PRCM->CLKLOADCTL = PRCM_CLKLOADCTL_LOAD;
    while ((PRCM->CLKLOADCTL & PRCM_CLKLOADCTL_LOAD_DONE) == 0) {}

    //enable clock for RFC modules
    RFC_PWR->PWMCLKEN = RFC_PWR_PWMCLKEN_RFCTRC | RFC_PWR_PWMCLKEN_FSCA | RFC_PWR_PWMCLKEN_PHA    | RFC_PWR_PWMCLKEN_RAT |
                        RFC_PWR_PWMCLKEN_RFERAM | RFC_PWR_PWMCLKEN_RFE  | RFC_PWR_PWMCLKEN_MDMRAM | RFC_PWR_PWMCLKEN_MDM |
                        RFC_PWR_PWMCLKEN_CPERAM | RFC_PWR_PWMCLKEN_CPR  | RFC_PWR_PWMCLKEN_RFC;

    //TODO ping here:
    printk("ping\n");
    RFC_DBELL->CMDR = (RF_CMD_PING << 16) | (1 << 0);
    while ((RFC_DBELL->CMDSTA & 0xff) == 0) {}
    printk("pong! %#x\n", RFC_DBELL->CMDSTA);
}

static inline void ti_rf_close(EXO* exo)
{
    //TODO:
}

void ti_rf_request(EXO* exo, IPC* ipc)
{
    if (HAL_ITEM(ipc->cmd) == IPC_OPEN)
        ti_rf_open(exo);
    else
    {
        if (!exo->rf.active)
        {
            error(ERROR_NOT_ACTIVE);
            return;
        }
        switch (HAL_ITEM(ipc->cmd))
        {
        case IPC_OPEN:
            printk("open!\n");
            ti_rf_open(exo);
            break;
        case IPC_CLOSE:
            ti_rf_close(exo);
            break;
        default:
            error(ERROR_NOT_SUPPORTED);
        }
    }
}
