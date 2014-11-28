/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "lpc_power.h"
#include "lpc_core_private.h"

#define IRC_VALUE                               12000000
#define PLL_LOCK_TIMEOUT                        10000

static unsigned int const __FCLKANA[] =         {0000000, 0600000, 1050000, 1400000, 1750000, 2100000, 2400000, 2700000,
                                                 3000000, 3250000, 3500000, 3750000, 4000000, 4200000, 4400000, 4600000};

void lpc_update_clock(int m, int p)
{
    int i;
    //power up IRC, HSE, SYS PLL
#if (HSE_VALUE)
    LPC_SYSCON->PDRUNCFG &= ~(SYSCON_PDRUNCFG_IRCOUT_PD | SYSCON_PDRUNCFG_IRC_PD | SYSCON_PDRUNCFG_SYSOSC_PD | SYSCON_PDRUNCFG_SYSPLL_PD);
#else
    LPC_SYSCON->PDRUNCFG &= ~(SYSCON_PDRUNCFG_IRCOUT_PD | SYSCON_PDRUNCFG_IRC_PD | SYSCON_PDRUNCFG_SYSPLL_PD);
#endif
    //switch to IRC
    LPC_SYSCON->MAINCLKSEL = SYSCON_MAINCLKSEL_IRC;
    LPC_SYSCON->MAINCLKUEN = 0;
    LPC_SYSCON->MAINCLKUEN = SYSCON_MAINCLKUEN_ENA;

    //TODO: setup HSE
    //enable and lock PLL
    LPC_SYSCON->SYSPLLCTRL = ((m - 1) << SYSCON_SYSPLLCTRL_MSEL_POS) | ((32 - __builtin_clz(p)) << SYSCON_SYSPLLCTRL_PSEL_POS);
#if (HSE_VALUE)
    LPC_SYSCON->SYSPLLCLKSEL = SYSCON_SYSPLLCLKSEL_SYSOSC;
#else
    LPC_SYSCON->SYSPLLCLKSEL = SYSCON_SYSPLLCLKSEL_IRC;
#endif
    LPC_SYSCON->SYSPLLCLKUEN = 0;
    LPC_SYSCON->SYSPLLCLKUEN = SYSCON_SYSPLLCLKUEN_ENA;
    //wait for PLL lock
    for (i = 0; i < PLL_LOCK_TIMEOUT; ++i)
    {
        if (LPC_SYSCON->SYSPLLSTAT & SYSCON_SYSPLLSTAT_LOCK)
        {
            //switch to PLL
            LPC_SYSCON->MAINCLKSEL = SYSCON_MAINCLKSEL_PLL;
            LPC_SYSCON->MAINCLKUEN = 0;
            LPC_SYSCON->MAINCLKUEN = SYSCON_MAINCLKUEN_ENA;
            return;
        }
    }
    //HSE failure? Switch to IRC
    LPC_SYSCON->SYSPLLCLKSEL = SYSCON_SYSPLLCLKSEL_IRC;
    LPC_SYSCON->SYSPLLCLKUEN = 0;
    LPC_SYSCON->SYSPLLCLKUEN = SYSCON_SYSPLLCLKUEN_ENA;
    //wait for PLL lock
    for (i = 0; i < PLL_LOCK_TIMEOUT; ++i)
    {
        if (LPC_SYSCON->SYSPLLSTAT & SYSCON_SYSPLLSTAT_LOCK)
        {
            //switch to PLL
            LPC_SYSCON->MAINCLKSEL = SYSCON_MAINCLKSEL_PLL;
            LPC_SYSCON->MAINCLKUEN = 0;
            LPC_SYSCON->MAINCLKUEN = SYSCON_MAINCLKUEN_ENA;
            return;
        }
    }
}

unsigned int lpc_get_system_clock()
{
    unsigned int pllsrc = 0;
    switch (LPC_SYSCON->MAINCLKSEL & 3)
    {
    case SYSCON_MAINCLKSEL_IRC:
        return IRC_VALUE;
        break;
    case SYSCON_MAINCLKSEL_WDT:
        return __FCLKANA[(LPC_SYSCON->WDTOSCCTRL & SYSCON_WDTOSCCTRL_FREQSEL_MASK) >> SYSCON_WDTOSCCTRL_FREQSEL_POS] /
                (2 * (1 + ((LPC_SYSCON->WDTOSCCTRL & SYSCON_WDTOSCCTRL_DIVSEL_MASK) >> SYSCON_WDTOSCCTRL_DIVSEL_POS)));
        break;
    case SYSCON_MAINCLKSEL_PLLSRC:
    case SYSCON_MAINCLKSEL_PLL:
        switch (LPC_SYSCON->SYSPLLCLKSEL & 3)
        {
        case SYSCON_SYSPLLCLKSEL_IRC:
            pllsrc = IRC_VALUE;
            break;
        case SYSCON_SYSPLLCLKSEL_SYSOSC:
            pllsrc = HSE_VALUE;
            break;
#ifdef LPC11U6x
        case SYSCON_SYSPLLCLKSEL_RTCOSC:
            pllsrc = RTC_VALUE;
            break;
#endif
        }
        if ((LPC_SYSCON->MAINCLKSEL & 3) == SYSCON_MAINCLKSEL_PLLSRC)
            return pllsrc;
        return pllsrc * (((LPC_SYSCON->SYSPLLCTRL & SYSCON_SYSPLLCTRL_MSEL_MASK) >> SYSCON_SYSPLLCTRL_MSEL_POS) + 1);
        break;
    }
    return 0;
}

RESET_REASON lpc_get_reset_reason(CORE* core)
{
    return core->power.reset_reason;
}

static inline void lpc_decode_reset_reason(CORE* core)
{
    core->power.reset_reason = RESET_REASON_UNKNOWN;
    if (LPC_SYSCON->SYSRSTSTAT & SYSCON_SYSRSTSTAT_WDT)
        core->power.reset_reason = RESET_REASON_WATCHDOG;
    else if (LPC_SYSCON->SYSRSTSTAT & SYSCON_SYSRSTSTAT_BOD)
        core->power.reset_reason = RESET_REASON_BROWNOUT;
    else if (LPC_SYSCON->SYSRSTSTAT & SYSCON_SYSRSTSTAT_SYSRST)
        core->power.reset_reason = RESET_REASON_SOFTWARE;
    else if (LPC_SYSCON->SYSRSTSTAT & SYSCON_SYSRSTSTAT_EXTRST)
        core->power.reset_reason = RESET_REASON_EXTERNAL;
    else if (LPC_SYSCON->SYSRSTSTAT & SYSCON_SYSRSTSTAT_POR)
        core->power.reset_reason = RESET_REASON_POWERON;
    LPC_SYSCON->SYSRSTSTAT = SYSCON_SYSRSTSTAT_WDT | SYSCON_SYSRSTSTAT_BOD | SYSCON_SYSRSTSTAT_SYSRST | SYSCON_SYSRSTSTAT_EXTRST | SYSCON_SYSRSTSTAT_POR;
}

void lpc_power_init(CORE *core)
{
    lpc_decode_reset_reason(core);
    lpc_update_clock(PLL_M, PLL_P);
}

bool lpc_power_request(CORE* core, IPC* ipc)
{
    bool need_post = false;
    switch (ipc->cmd)
    {
#if (SYS_INFO)
    case IPC_GET_INFO:
        //TODO:
//        lpc_power_info(core);
        need_post = true;
        break;
#endif
    case LPC_POWER_GET_SYSTEM_CLOCK:
        ipc->param1 = lpc_get_system_clock();
        need_post = true;
        break;
    case LPC_POWER_UPDATE_CLOCK:
        lpc_update_clock(ipc->param1, ipc->param2);
        need_post = true;
        break;
    case LPC_POWER_GET_RESET_REASON:
        ipc->param1 = lpc_get_reset_reason(core);
        need_post = true;
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
        need_post = true;
    }
    return need_post;
}
