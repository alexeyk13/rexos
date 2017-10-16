/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "lpc_power.h"
#include "lpc_exo_private.h"
#include "sys_config.h"
#include "../kerror.h"

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

static inline unsigned int lpc_power_get_core_clock_inside()
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
static inline void lpc_decode_reset_reason(EXO* exo)
{
    exo->power.reset_reason = RESET_REASON_UNKNOWN;
    if (LPC_SYSCON->SYSRSTSTAT & SYSCON_SYSRSTSTAT_WDT)
        exo->power.reset_reason = RESET_REASON_WATCHDOG;
    else if (LPC_SYSCON->SYSRSTSTAT & SYSCON_SYSRSTSTAT_BOD)
        exo->power.reset_reason = RESET_REASON_BROWNOUT;
    else if (LPC_SYSCON->SYSRSTSTAT & SYSCON_SYSRSTSTAT_SYSRST)
        exo->power.reset_reason = RESET_REASON_SOFTWARE;
    else if (LPC_SYSCON->SYSRSTSTAT & SYSCON_SYSRSTSTAT_EXTRST)
        exo->power.reset_reason = RESET_REASON_EXTERNAL;
    else if (LPC_SYSCON->SYSRSTSTAT & SYSCON_SYSRSTSTAT_POR)
        exo->power.reset_reason = RESET_REASON_POWERON;
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

static void lpc_power_switch_core_clock(unsigned int divider)
{
#if (POWER_MID_DIVIDER)
    //switch to MID clock frequency
    LPC_CGU->IDIVB_CTRL = (LPC_CGU->IDIVB_CTRL & ~CGU_IDIVB_CTRL_IDIV_Msk) | ((POWER_MID_DIVIDER - 1) << CGU_IDIVB_CTRL_IDIV_Pos);
    //need to wait 50us before switching to HI/LO freq.
    delay_clks(90 * 50);
#endif //POWER_MID_DIVIDER
    LPC_CGU->IDIVB_CTRL = (LPC_CGU->IDIVB_CTRL & ~CGU_IDIVB_CTRL_IDIV_Msk) | ((divider - 1) << CGU_IDIVB_CTRL_IDIV_Pos);
    //Wait till stabilization
    __NOP();
    __NOP();
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

#if (PLL_P == 1)
    LPC_CGU->PLL1_CTRL |= CGU_PLL1_CTRL_DIRECT_Msk;
#else
    LPC_CGU->PLL1_CTRL |= (30 - __builtin_clz(PLL_P)) << CGU_PLL1_CTRL_PSEL_Pos;
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

    //4. configure IDIVB divider
    LPC_CGU->IDIVB_CTRL = CGU_IDIVB_CTRL_PD_Msk;
    LPC_CGU->IDIVB_CTRL |= ((POWER_LO_DIVIDER - 1) << CGU_IDIVB_CTRL_IDIV_Pos) | CGU_IDIVB_CTRL_AUTOBLOCK_Msk | CGU_CLK_PLL1;
    LPC_CGU->IDIVB_CTRL &= ~CGU_IDIVB_CTRL_PD_Msk;
    //5. connect M3 clock to IDIVB
    LPC_CGU->BASE_M3_CLK = CGU_CLK_IDIVB;
    __NOP();
    __NOP();
    __NOP();
    __NOP();

    //6. Tune flash accelerator.
    LPC_CREG->FLASHCFGA = CREG_FLASHCFG_MAGIC_Msk | (8 << CREG_FLASHCFG_FLASHTIM_Pos) | CREG_FLASHCFG_POW_Msk;
    LPC_CREG->FLASHCFGB = CREG_FLASHCFG_MAGIC_Msk | (8 << CREG_FLASHCFG_FLASHTIM_Pos) | CREG_FLASHCFG_POW_Msk;

    //7. Power down unused hardware
    LPC_CGU->IDIVA_CTRL = CGU_IDIVA_CTRL_PD_Msk;
    LPC_CGU->IDIVC_CTRL = CGU_IDIVC_CTRL_PD_Msk;
    LPC_CGU->IDIVD_CTRL = CGU_IDIVD_CTRL_PD_Msk;
    LPC_CGU->IDIVE_CTRL = CGU_IDIVE_CTRL_PD_Msk;
    LPC_CGU->BASE_USB0_CLK = CGU_BASE_USB0_CLK_PD_Msk;
    LPC_CGU->BASE_SPIFI_CLK = CGU_BASE_SPIFI_CLK_PD_Msk;
    LPC_CGU->BASE_SDIO_CLK = CGU_BASE_SDIO_CLK_PD_Msk;
    LPC_CGU->BASE_SSP0_CLK = CGU_BASE_SSP0_CLK_PD_Msk;
    LPC_CGU->BASE_SSP1_CLK = CGU_BASE_SSP1_CLK_PD_Msk;
    LPC_CGU->BASE_UART0_CLK = CGU_BASE_UART0_CLK_PD_Msk;
    LPC_CGU->BASE_UART1_CLK = CGU_BASE_UART1_CLK_PD_Msk;
    LPC_CGU->BASE_UART2_CLK = CGU_BASE_UART2_CLK_PD_Msk;
    LPC_CGU->BASE_UART3_CLK = CGU_BASE_UART3_CLK_PD_Msk;
    LPC_CGU->BASE_OUT_CLK = CGU_BASE_OUT_CLK_PD_Msk;
    LPC_CGU->BASE_CGU_OUT0_CLK = CGU_BASE_CGU_OUT0_CLK_PD_Msk;
    LPC_CGU->BASE_CGU_OUT1_CLK = CGU_BASE_CGU_OUT1_CLK_PD_Msk;

#if defined(LPC183x) || defined(LPC185x)
    LPC_CGU->BASE_USB1_CLK = CGU_BASE_USB1_CLK_PD_Msk;
    LPC_CGU->BASE_PHY_RX_CLK = CGU_BASE_PHY_RX_CLK_PD_Msk;
    LPC_CGU->BASE_PHY_TX_CLK = CGU_BASE_PHY_TX_CLK_PD_Msk;
#endif //defined(LPC183x) || defined(LPC185x)

#if defined(LPC185x)
    LPC_CGU->BASE_LCD_CLK = CGU_BASE_LCD_CLK_PD_Msk;
#endif //defined(LPC183x) || defined(LPC185x)
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
    case CGU_CLK_IRC:
        return IRC_VALUE;
    case CGU_CLK_LSE:
        return LSE_VALUE;
    case CGU_CLK_HSE:
        return HSE_VALUE;
    case CGU_CLK_PLL1:
        return lpc_get_pll1_clock();
    case CGU_CLK_IDIVA:
    case CGU_CLK_IDIVB:
    case CGU_CLK_IDIVC:
    case CGU_CLK_IDIVD:
    case CGU_CLK_IDIVE:
        return lpc_get_source_clock(CGU_IDIVx_CTRL[(source - CGU_CLK_IDIVA) >> CGU_CLK_CLK_SEL_POS] & CGU_IDIVx_CTRL_CLK_SEL_Msk) /
                (((CGU_IDIVx_CTRL[(source - CGU_CLK_IDIVA) >> CGU_CLK_CLK_SEL_POS] & CGU_IDIVx_CTRL_IDIV_Msk) >> CGU_IDIVx_CTRL_IDIV_Pos) + 1);
    default:
        kerror(ERROR_NOT_SUPPORTED);
        return 0;

    }
}

unsigned int lpc_power_get_core_clock_inside()
{
    return lpc_get_source_clock(LPC_CGU->BASE_M3_CLK & CGU_BASE_M3_CLK_CLK_SEL_Msk);
}

#if (LPC_DECODE_RESET)
static inline void lpc_decode_reset_reason(EXO* exo)
{
    /* According to errata:
     * The CORE_RST bits in the RGU's status register do not properly indicate the cause of the core reset. When the core is reset the RGU
     * is also reset and as a result the state of these bits will always read with their default values. As a result, these status bits cannot
     * be used to determine the cause of the core reset */

    exo->power.reset_reason = RESET_REASON_UNKNOWN;
    if (LPC_EVENTROUTER->HILO == 0 && LPC_EVENTROUTER->EDGE == 0)
        exo->power.reset_reason = RESET_REASON_POWERON;
    else if ((LPC_EVENTROUTER->EDGE & EVENTROUTER_EDGE_RESET_E_Msk) && (LPC_EVENTROUTER->STATUS & EVENTROUTER_STATUS_RESET_ST_Msk))
        exo->power.reset_reason = RESET_REASON_EXTERNAL;

    LPC_EVENTROUTER->HILO &= ~EVENTROUTER_HILO_RESET_L_Msk;
    LPC_EVENTROUTER->EDGE |= EVENTROUTER_EDGE_RESET_E_Msk;
    LPC_EVENTROUTER->CLR_STAT = EVENTROUTER_CLR_STAT_RESET_CLRST_Msk;
}
#endif //LPC_DECODE_RESET

#if (POWER_MANAGEMENT)
static inline void lpc_power_hi(EXO* exo)
{
    lpc_timer_suspend(exo);
    lpc_power_switch_core_clock(POWER_HI_DIVIDER);
    lpc_timer_adjust(exo);
}

static inline void lpc_power_lo(EXO* exo)
{
    lpc_timer_suspend(exo);
    lpc_power_switch_core_clock(POWER_LO_DIVIDER);
    lpc_timer_adjust(exo);
}

static inline void lpc_power_set_mode(EXO* exo, POWER_MODE mode)
{
    switch (mode)
    {
    case POWER_MODE_HIGH:
        lpc_power_hi(exo);
        break;
    case POWER_MODE_LOW:
        lpc_power_lo(exo);
        break;
    default:
        kerror(ERROR_NOT_SUPPORTED);
    }
}
#endif //(POWER_MANAGEMENT)

#endif //LPC11Uxx

#if (LPC_DECODE_RESET)
RESET_REASON lpc_get_reset_reason(EXO* exo)
{
    return exo->power.reset_reason;
}
#endif //LPC_DECODE_RESET

unsigned int lpc_power_get_clock_inside(POWER_CLOCK_TYPE clock_type)
{
    switch (clock_type)
    {
#ifdef LPC18xx
    case POWER_BUS_CLOCK:
        return lpc_get_pll1_clock();
#endif //LPC18xx
    case POWER_CORE_CLOCK:
        return lpc_power_get_core_clock_inside();
    default:
        kerror(ERROR_NOT_SUPPORTED);
        return 0;
    }
}

void lpc_power_init(EXO* exo)
{
#if (LPC_DECODE_RESET)
    lpc_decode_reset_reason(exo);
#endif //LPC_DECODE_RESET
    lpc_setup_clock();
}

void lpc_power_request(EXO* exo, IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
    case POWER_GET_CLOCK:
        ipc->param2 = lpc_power_get_clock_inside(ipc->param1);
        break;
#if (LPC_DECODE_RESET)
    case LPC_POWER_GET_RESET_REASON:
        ipc->param2 = lpc_get_reset_reason(exo);
        break;
#endif //LPC_DECODE_RESET
#if (POWER_MANAGEMENT)
    case POWER_SET_MODE:
        //no return
        lpc_power_set_mode(exo, ipc->param1);
        break;
#endif //POWER_MANAGEMENT
    default:
        kerror(ERROR_NOT_SUPPORTED);
    }
}

