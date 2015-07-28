/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "lpc_power.h"
#include "lpc_core_private.h"

#define PLL_LOCK_TIMEOUT                            10000
#define IRC_VALUE                               12000000

static unsigned int const __FCLKANA[] =         {0000000, 0600000, 1050000, 1400000, 1750000, 2100000, 2400000, 2700000,
                                                 3000000, 3250000, 3500000, 3750000, 4000000, 4200000, 4400000, 4600000};

#ifdef LPC11Uxx
static inline void lpc_setup_clock()
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


#if (HSE_BYPASS)
    LPC_SYSCON->SYSOSCCTRL |= SYSCON_SYSOSCCTRL_BYPASS;
#endif
#if (HSE_VALUE >= 15000000)
    LPC_SYSCON->SYSOSCCTRL |= SYSCON_SYSOSCCTRL_FREQRANGE;
#endif
    //enable and lock PLL
    LPC_SYSCON->SYSPLLCTRL = ((PLL_M - 1) << SYSCON_SYSPLLCTRL_MSEL_POS) | ((32 - __builtin_clz(PLL_P)) << SYSCON_SYSPLLCTRL_PSEL_POS);
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

static inline unsigned int lpc_get_core_clock()
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
            pllsrc = LSE_VALUE;
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

#if (LPC_DECODE_RESET)
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
#endif //LPC_DECODE_RESET
#else //LPC18xx

//timer is not yet active on power on
static void delay_clks(unsigned int clks)
{
    int i;
    for (i = 0; i < clks; ++i)
        __NOP();
}

static inline void lpc_setup_clock()
{
    int i;
    //1. Switch to internal source
    LPC_CGU->BASE_M3_CLK = CGU_CLK_IRC;
    __NOP();
    __NOP();
    __NOP();

    //2. Turn HSE on if enabled
    LPC_CGU->XTAL_OSC_CTRL = CGU_XTAL_OSC_CTRL_ENABLE_Msk;
#if (HSE_VALUE)
#if (HSE_VALUE) > 15000000
    LPC_CGU->XTAL_OSC_CTRL = CGU_XTAL_OSC_CTRL_HF_Msk;
#endif
    LPC_CGU->XTAL_OSC_CTRL &= ~CGU_XTAL_OSC_CTRL_ENABLE_Msk;
    delay_clks(500);
#endif //HSE_VALUE

    //3. Turn PLL1 on
    LPC_CGU->PLL1_CTRL = CGU_PLL1_CTRL_PD_Msk;
    __NOP();
    __NOP();
    __NOP();

    LPC_CGU->PLL1_CTRL |= (0 << CGU_PLL1_CTRL_PSEL_Pos) | ((PLL_N - 1) << CGU_PLL1_CTRL_NSEL_Pos) | ((PLL_M - 1) << CGU_PLL1_CTRL_MSEL_Pos);
#if (HSE_VALUE)
    LPC_CGU->PLL1_CTRL |= CGU_CLK_HSE;
#else
    LPC_CGU->PLL1_CTRL |= CGU_CLK_IRC;
#endif
    LPC_CGU->PLL1_CTRL &= ~CGU_PLL1_CTRL_PD_Msk;

    //wait for PLL lock
    for (i = 0; i < PLL_LOCK_TIMEOUT; ++i)
        if (LPC_CGU->PLL1_STAT & CGU_PLL1_STAT_LOCK_Msk)
            break;
    //HSE failure? switch to IRC
    if ((LPC_CGU->PLL1_STAT & CGU_PLL1_STAT_LOCK_Msk) == 0)
    {
        LPC_CGU->PLL1_CTRL |= CGU_PLL1_CTRL_PD_Msk;
        __NOP();
        __NOP();
        __NOP();
        LPC_CGU->PLL1_CTRL &= ~CGU_PLL1_CTRL_CLK_SEL_Msk;
        LPC_CGU->PLL1_CTRL |= CGU_CLK_IRC;
        LPC_CGU->PLL1_CTRL &= ~CGU_PLL1_CTRL_PD_Msk;
        while ((LPC_CGU->PLL1_STAT & CGU_PLL1_STAT_LOCK_Msk) == 0) {}
    }
    //4. Switch to PLL
    LPC_CGU->BASE_M3_CLK = CGU_CLK_PLL1;
    delay_clks(100);
    delay_clks(1000000);
    //5. PLL direct mode
    LPC_CGU->PLL1_CTRL |= CGU_PLL1_CTRL_DIRECT_Msk;
    __NOP();
    __NOP();
    __NOP();
}

static unsigned int lpc_get_source_clock(unsigned int source);

static inline unsigned int lpc_get_pll1_clock()
{
    unsigned int pll1_out;
    unsigned int pll1_in = lpc_get_source_clock(LPC_CGU->PLL1_CTRL & CGU_PLL1_CTRL_CLK_SEL_Msk);
    pll1_out = pll1_in / (((LPC_CGU->PLL1_CTRL & CGU_PLL1_CTRL_NSEL_Msk) >> CGU_PLL1_CTRL_NSEL_Pos) + 1)
                       *  (((LPC_CGU->PLL1_CTRL & CGU_PLL1_CTRL_MSEL_Msk) >> CGU_PLL1_CTRL_MSEL_Pos) + 1);
    if ((LPC_CGU->PLL1_CTRL & CGU_PLL1_CTRL_DIRECT_Msk) == 0)
        pll1_out /= 2 * (1 << ((LPC_CGU->PLL1_CTRL & CGU_PLL1_CTRL_PSEL_Msk) >> CGU_PLL1_CTRL_PSEL_Pos));
    return pll1_out;
}

static unsigned int lpc_get_source_clock(unsigned int source)
{
    switch (source)
    {
    case CGU_CLK_LSE:
        return LSE_VALUE;
    case CGU_CLK_HSE:
        return HSE_VALUE;
    case CGU_CLK_PLL1:
        return lpc_get_pll1_clock();
    default:
        //assume IRC, all rest values are not used for configuration
        return IRC_VALUE;

    }
}

static inline unsigned int lpc_get_core_clock()
{
    return lpc_get_source_clock(LPC_CGU->BASE_M3_CLK & CGU_BASE_M3_CLK_CLK_SEL_Msk);
}

#if (LPC_DECODE_RESET)
static inline void lpc_decode_reset_reason(CORE* core)
{
    /* According to errata:
     * The CORE_RST bits in the RGU's status register do not properly indicate the cause of the core reset. When the core is reset the RGU
     * is also reset and as a result the state of these bits will always read with their default values. As a result, these status bits cannot
     * be used to determine the cause of the core reset */

    core->power.reset_reason = RESET_REASON_UNKNOWN;
    if (LPC_EVENTROUTER->HILO == 0 && LPC_EVENTROUTER->EDGE == 0)
        core->power.reset_reason = RESET_REASON_POWERON;
    else if ((LPC_EVENTROUTER->EDGE & EVENTROUTER_EDGE_RESET_E_Msk) && (LPC_EVENTROUTER->STATUS & EVENTROUTER_STATUS_RESET_ST_Msk))
        core->power.reset_reason = RESET_REASON_EXTERNAL;

    LPC_EVENTROUTER->HILO &= ~EVENTROUTER_HILO_RESET_L_Msk;
    LPC_EVENTROUTER->EDGE |= EVENTROUTER_EDGE_RESET_E_Msk;
    LPC_EVENTROUTER->CLR_STAT = EVENTROUTER_CLR_STAT_RESET_CLRST_Msk;
}
#endif //LPC_DECODE_RESET
#endif //LPC11Uxx

#if (LPC_DECODE_RESET)
RESET_REASON lpc_get_reset_reason(CORE* core)
{
    return core->power.reset_reason;
}
#endif //LPC_DECODE_RESET

void lpc_power_init(CORE *core)
{
#if (LPC_DECODE_RESET)
    lpc_decode_reset_reason(core);
#endif //LPC_DECODE_RESET
    lpc_setup_clock();
}

bool lpc_power_request(CORE* core, IPC* ipc)
{
    bool need_post = false;
    switch (HAL_ITEM(ipc->cmd))
    {
    case LPC_POWER_GET_CORE_CLOCK:
        ipc->param2 = lpc_get_core_clock();
        need_post = true;
        break;
#if (LPC_DECODE_RESET)
    case LPC_POWER_GET_RESET_REASON:
        ipc->param2 = lpc_get_reset_reason(core);
        need_post = true;
        break;
#endif //LPC_DECODE_RESET
    default:
        error(ERROR_NOT_SUPPORTED);
        need_post = true;
    }
    return need_post;
}
