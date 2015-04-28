/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "stm32_power.h"
#include "stm32_core_private.h"
#include "stm32_rtc.h"
#include "stm32_timer.h"
#include "stm32_config.h"
#include "sys_config.h"

#if defined(STM32F1)
#define MAX_APB2                             72000000
#define MAX_APB1                             36000000
#define MAX_ADC                              14000000
#elif defined(STM32F2)
#define MAX_APB2                             60000000
#define MAX_APB1                             30000000
#elif defined(STM32F401) || defined(STM32F411)
#define MAX_APB2                             100000000
#define MAX_APB1                             50000000
#elif defined(STM32F405) || defined(STM32F407) || defined(STM32F415) || defined(STM32F417)
#define MAX_APB2                             84000000
#define MAX_APB1                             42000000
#elif defined(STM32F427) || defined(STM32F429) || defined(STM32F437) || defined(STM32F439)
#define MAX_APB2                             90000000
#define MAX_APB1                             45000000
#elif defined(STM32L0)
#define MAX_APB2                             32000000
#define MAX_APB1                             32000000
#define HSI_VALUE                            16000000
#endif

#if defined(STM32F1) || defined(STM32L0)
#define PPRE1_POS                            8
#define PPRE2_POS                            11
#define PLLSRC_BIT                           (1 << 16)
#define PLLSRC_REG                           RCC->CFGR
#elif defined(STM32F2) || defined(STM32F4)
#define PPRE1_POS                            10
#define PPRE2_POS                            13
#define PLLSRC_BIT                           (1 << 22)
#define PLLSRC_REG                           RCC->PLLCFGR
#endif

#ifndef RCC_CSR_WDGRSTF
#define RCC_CSR_WDGRSTF RCC_CSR_IWDGRSTF
#endif

#ifndef RCC_CSR_PADRSTF
#define RCC_CSR_PADRSTF RCC_CSR_PINRSTF
#endif

#if defined(STM32F100)
static inline void setup_pll()
{
    RCC->CFGR &= ~(0xf << 18);
    RCC->CFGR2 &= ~0xf;
    RCC->CFGR2 |= (PLL_DIV - 1) << 0;
    RCC->CFGR |= ((PLL_MUL - 2) << 18);
}

static inline int get_pll_clock()
{
    int pllsrc = HSI_VALUE / 2;
#if (HSE_VALUE)
    if (PLLSRC_REG & PLLSRC_BIT)
        pllsrc = HSE_VALUE / ((RCC->CFGR2 & 0xf) + 1);
#endif
    return pllsrc * (((RCC->CFGR >> 18) & 0xf) + 2);
}

#elif defined(STM32F10X_CL)
static inline void setup_pll()
{
    RCC->CR &= ~RCC_CR_PLL2ON;
    RCC->CFGR &= ~(0xf << 18);
    RCC->CFGR2 &= ~(0xf | PLLSRC_BIT);
#if (PLL2_DIV) && (PLL2_MUL)
    RCC->CFGR2 |= ((PLL2_DIV - 1) << 4) | ((PLL2_MUL - 2) << 8);
    //turn PLL2 ON
    RCC->CR |= RCC_CR_PLL2ON;
    while (!(RCC->CR & RCC_CR_PLL2RDY)) {}
    RCC->CFGR2 |= PLLSRC_BIT;
#endif //PLL2_DIV && PLL2_MUL
    RCC->CFGR2 |= (PLL_DIV - 1) << 0;
    RCC->CFGR |= (PLL_MUL - 2) << 18;
}

static inline int get_pll_clock()
{
    int pllsrc = HSI_VALUE / 2;
    int pllmul;
#if (HSE_VALUE)
    int preddivsrc = HSE_VALUE;
    int pllmul2;
    if (PLLSRC_REG & PLLSRC_BIT)
    {
        if (RCC->CFGR2 & PLLSRC_BIT)
        {
            pllmul2 = ((RCC->CFGR2 >> 8) & 0xf) + 2;
            if (pllmul2 == PLL2_MUL_20)
                pllmul2 = 20;
            preddivsrc = HSE_VALUE / (((RCC->CFGR2 >> 4) & 0xf) + 1) * pllmul2;
        }
        pllsrc = preddivsrc / ((RCC->CFGR2 & 0xf) + 1);
    }
#endif
    pllmul = ((RCC->CFGR >> 18) & 0xf) + 2;
    if (pllmul == PLL_MUL_6_5)
        return pllsrc / 10 * 65;
    else
        return pllsrc * pllmul;
}

#elif defined (STM32F1)
static inline void setup_pll()
{
    RCC->CFGR &= ~((0xf << 18) | (1 << 17));
    RCC->CFGR |= ((PLL_MUL - 2) << 18);
    RCC->CFGR |= ((PLL_DIV - 1) << 17);
}

static inline int get_pll_clock()
{
    int pllsrc = HSI_VALUE / 2;
#if (HSE_VALUE)
    if (PLLSRC_REG & PLLSRC_BIT)
        pllsrc = HSE_VALUE / (((RCC->CFGR >> 17) & 0x1) + 1);
#endif
    return pllsrc * (((RCC->CFGR >> 18) & 0xf) + 2);
}

#elif defined(STM32L0)
static inline void setup_pll()
{
    unsigned int pow, mul;
    mul = PLL_MUL;
    for (pow = 0; mul > 4; mul /= 2, pow++) {}
    RCC->CFGR &= ~((0xf << 18) | (3 << 22));
    RCC->CFGR |= (((pow << 1) | (mul - 3)) << 18);
    RCC->CFGR |= (((PLL_DIV - 1) << 22) | ((pow << 1) | (mul - 3)) << 18);
}

static inline int get_pll_clock()
{
    unsigned int pllsrc = HSI_VALUE;
    unsigned int mul = (1 << ((RCC->CFGR >> 19) & 7)) * (3 + ((RCC->CFGR >> 18) & 1));
    unsigned int div = ((RCC->CFGR >> 22) & 3) + 1;

#if (HSE_VALUE)
    if (PLLSRC_REG & PLLSRC_BIT)
        pllsrc = HSE_VALUE;
#endif
    return pllsrc * mul / div;
}

#elif defined (STM32F2) || defined (STM32F4)
static inline void setup_pll()
{
    RCC->PLLCFGR &= ~((0x3f << 0) | (0x1ff << 6) | (3 << 16));
    RCC->PLLCFGR = (PLL_M << 0) | (PLL_N << 6) | (((PLL_P >> 1) -1) << 16);
}

static inline int get_pll_clock()
{
    int pllsrc = HSI_VALUE;
#if (HSE_VALUE)
    if (PLLSRC_REG & PLLSRC_BIT)
        pllsrc = HSE_VALUE;
#endif
    return pllsrc / (RCC->PLLCFGR & 0x3f) * ((RCC->PLLCFGR  >> 6) & 0x1ff) / ((((RCC->PLLCFGR  >> 16) & 0x3) + 1) << 1);
}
#endif

#if defined(STM32L0)
static int get_msi_clock()
{
    return 65536 * (1 << ((RCC->ICSCR >> 13) & 7));
}
#endif

int get_core_clock()
{
    switch (RCC->CFGR & (3 << 2))
    {
    case RCC_CFGR_SWS_HSI:
        return HSI_VALUE;
        break;
#if defined(STM32L0)
    case RCC_CFGR_SWS_MSI:
        return get_msi_clock();
        break;
#endif
    case RCC_CFGR_SWS_HSE:
        return HSE_VALUE;
        break;
    case RCC_CFGR_SWS_PLL:
        return get_pll_clock();
    }
    return 0;
}

int get_ahb_clock()
{
    int div = 1;
    if (RCC->CFGR & (1 << 7))
        div = 1 << (((RCC->CFGR >> 4) & 7) + 1);
    if (div >= 32)
        div <<= 1;
    return get_core_clock() / div;
}

int get_apb2_clock()
{
    int div = 1;
    if (RCC->CFGR & (1 << (PPRE2_POS + 2)))
        div = 1 << (((RCC->CFGR >> PPRE2_POS) & 3) + 1);
    return get_ahb_clock() /div;
}

int get_apb1_clock()
{
    int div = 1;
    if (RCC->CFGR & (1 << (PPRE1_POS + 2)))
        div = 1 << (((RCC->CFGR >> PPRE1_POS) & 3) + 1);
    return get_ahb_clock() / div;
}

#if defined(STM32F1)
int get_adc_clock()
{
    return get_apb2_clock() / ((((RCC->CFGR >> 14) & 3) + 1) * 2);
}
#endif

#if (STM32_DECODE_RESET)
RESET_REASON get_reset_reason(CORE* core)
{
    return core->power.reset_reason;
}
#endif //STM32_DECODE_RESET

static inline void power_dma_on(CORE* core, STM32_DMA dma)
{
    if (core->power.dma_count[dma]++ == 0)
        RCC->AHBENR |= 1 << dma;
}

static inline void power_dma_off(CORE* core, STM32_DMA dma)
{
    if (--core->power.dma_count[dma] == 0)
        RCC->AHBENR &= ~(1 << dma);
}

#if (HSE_VALUE)
static void stm32_power_hse_on()
{
    if ((RCC->CR & RCC_CR_HSEON) == 0)
    {
#if defined(STM32L0) && !(LSE_VALUE)
        RCC->CR &= ~(3 << 20);
        RCC->CR |= (30 - __builtin_clz(HSE_VALUE / 1000000)) << 20;
#endif //STM32L0 && !LSE_VALUE

#if (HSE_BYPASS)
        RCC->CR |= RCC_CR_HSEON | RCC_CR_HSEBYP;
#else
        RCC->CR |= RCC_CR_HSEON;
#endif //HSE_BYPAS
#if defined(HSE_STARTUP_TIMEOUT)
        int i;
        for (i = 0; i < HSE_STARTUP_TIMEOUT; ++i)
            if (RCC->CR & RCC_CR_HSERDY)
                break;
#else
        while ((RCC->CR & RCC_CR_HSERDY) == 0) {}
#endif //HSE_STARTUP_TIMEOUT
    }
}

static void stm32_power_hse_off()
{
    RCC->CR &= ~RCC_CR_HSEON;
}
#endif //HSE_VALUE

static void stm32_power_pll_on(STM32_CLOCK_SOURCE_TYPE src)
{
    RCC->CR &= ~RCC_CR_PLLON;
    setup_pll();
#if (HSE_VALUE)
    if (src == STM32_CLOCK_SOURCE_HSE)
        PLLSRC_REG |= PLLSRC_BIT;
    else
#endif
        PLLSRC_REG &= ~PLLSRC_BIT;
    //turn PLL on
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY)) {}
}

static void stm32_power_pll_off()
{
    RCC->CR &= ~RCC_CR_PLLON;
}

static unsigned int stm32_power_get_bus_prescaller(unsigned int clock, unsigned int max)
{
    int i;
    for (i = 0; i <= 4; ++i)
        if ((clock >> i) <= max)
            break;
    return i ? i + 3 : 0;
}

static void stm32_power_set_core_clock(CORE* core, STM32_CLOCK_SOURCE_TYPE src)
{
    unsigned int sw, core_clock;
    //turn off all peripheral clocking
    unsigned int apb1 = RCC->APB1ENR;
    RCC->APB1ENR = 0;
    unsigned int apb2 = RCC->APB2ENR;
    RCC->APB2ENR = 0;
#if defined(STM32F1) || defined(STM32L0)
    unsigned int ahb = RCC->AHBENR;
    RCC->AHBENR = 0;
#elif defined (STM32F2) || defined(STM32F4)
    unsigned int ahb1 = RCC->AHB1ENR;
    RCC->AHB1ENR = 0;
    unsigned int ahb2 = RCC->AHB2ENR;
    RCC->AHB2ENR = 0;
    unsigned int ahb3 = RCC->AHB3ENR;
    RCC->AHB3ENR = 0;
#endif
    __NOP();
    __NOP();

    //switch to internal source first. Which is slowest and always available
    RCC->CFGR &= ~(3 << 0);
    while ((RCC->CFGR >> 2) & 3) {}
    __NOP();
    __NOP();

    switch (src)
    {
#if (HSE_VALUE)
    case STM32_CLOCK_SOURCE_HSE:
        sw = RCC_CFGR_SW_HSE;
        core_clock = HSE_VALUE;
        break;
#endif
#if defined (STM32L0)
    case STM32_CLOCK_SOURCE_MSI:
        sw = RCC_CFGR_SW_MSI;
        core_clock = get_msi_clock();
        break;
#endif
    case STM32_CLOCK_SOURCE_PLL:
        sw = RCC_CFGR_SW_PLL;
        core_clock = get_pll_clock();
        break;
    default:
        sw = RCC_CFGR_SW_HSI;
        core_clock = HSI_VALUE;
    }

    //setup bases
    //AHB. Can operates at maximum clock
    //APB2
    RCC->CFGR = (RCC->CFGR & ~(7 << PPRE2_POS)) | (stm32_power_get_bus_prescaller(core_clock, MAX_APB2) << PPRE2_POS);
    //APB1
    RCC->CFGR = (RCC->CFGR & ~(7 << PPRE1_POS)) | (stm32_power_get_bus_prescaller(core_clock, MAX_APB1) << PPRE1_POS);

#if defined(STM32F1)
#if (STM32_ADC_DRIVER)
    //APB2 can operates at maximum in F1
    unsigned int adc_psc = (((core_clock / MAX_ADC) + 1) & ~1) >> 1;
    if (adc_psc)
        --adc_psc;
    RCC->CFGR = (RCC->CFGR & ~(3 << 14)) | (adc_psc << 14);
#endif // STM32_ADC_DRIVER
#endif //STM32F1

    //tune flash latency
#if defined(STM32F1) && !defined(STM32F100)
    FLASH->ACR = FLASH_ACR_PRFTBE | ((core_clock - 1) / 24000000);
#elif defined(STM32F2) || defined(STM32F4)
    FLASH->ACR = FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN | ((core_clock - 1) / 30000000);
#elif defined(STM32L0)
    FLASH->ACR = FLASH_ACR_PRE_READ | ((core_clock - 1) / 16000000);
#endif
    __DSB();
    __DMB();
    __NOP();
    __NOP();

    //switch to source
    RCC->CFGR |= sw;
    while (((RCC->CFGR >> 2) & 3) != sw) {}
    __NOP();
    __NOP();

    //restore buses
#if defined(STM32F1) || defined(STM32L0)
    RCC->AHBENR = ahb;
#elif defined (STM32F2) || defined(STM32F4)
    RCC->AHB1ENR = ahb1;
    RCC->AHB2ENR = ahb2;
    RCC->AHB3ENR = ahb3;
#endif
    RCC->APB1ENR = apb1;
    RCC->APB2ENR = apb2;
}

unsigned int get_clock(STM32_POWER_CLOCKS type)
{
    unsigned int res = 0;
    switch (type)
    {
    case STM32_CLOCK_CORE:
        res = get_core_clock();
        break;
    case STM32_CLOCK_AHB:
        res = get_ahb_clock();
        break;
    case STM32_CLOCK_APB1:
        res = get_apb1_clock();
        break;
    case STM32_CLOCK_APB2:
        res = get_apb2_clock();
        break;
#if defined(STM32F1)
    case STM32_CLOCK_ADC:
        res = get_adc_clock();
        break;
#endif
    default:
        error(ERROR_INVALID_PARAMS);
    }
    return res;
}

#if defined (STM32L0)
static void stm32_power_set_vrange(int vrange)
{
    PWR->CR = ((PWR->CR) & ~(3 << 27)) | ((vrange & 3) << 27);
    __NOP();
    __NOP();
}

static inline void stm32_power_hsi_on()
{
    RCC->CR |= RCC_CR_HSION;
    while ((RCC->CR & RCC_CR_HSIRDY) == 0) {}
}

static inline void stm32_power_hsi_off()
{
    RCC->CR &= ~RCC_CR_HSION;
}

static void stm32_power_msi_set_range(unsigned int range)
{
    RCC->ICSCR = ((RCC->ICSCR) & ~(7 << 13)) | ((range & 7) << 13);
    __NOP();
    __NOP();
}
#endif //STM32L0

static void stm32_power_enable_pll(CORE* core)
{
#if defined (STM32L0)
    //switch to 1.8V
    stm32_power_set_vrange(1);
#endif //STM32L0
#if (HSE_VALUE)
    stm32_power_hse_on();
    stm32_power_pll_on(STM32_CLOCK_SOURCE_HSE);
#else
#if defined(STM32L0)
    stm32_power_hsi_on();
#endif //STM32L0
    stm32_power_pll_on(STM32_CLOCK_SOURCE_HSI);
#endif //HSE_VALUE
}

#if (POWER_MANAGEMENT_SUPPORT)
static void stm32_power_update_core_clock(CORE* core, STM32_CLOCK_SOURCE_TYPE cst)
{
    __disable_irq();
    stm32_power_set_core_clock(core, cst);
    stm32_timer_pm_event(core);
    __enable_irq();
}

static void stm32_power_hi(CORE* core)
{
    stm32_power_enable_pll(core);
    stm32_power_update_core_clock(core, STM32_CLOCK_SOURCE_PLL);
}

static inline void stm32_power_lo(CORE* core)
{
#if defined (STM32L0)
    stm32_power_update_core_clock(core, STM32_CLOCK_SOURCE_MSI);
#else
    stm32_power_update_core_clock(core, STM32_CLOCK_SOURCE_HSI);
#endif

    stm32_power_pll_off();
#if (HSE_VALUE)
    stm32_power_hse_off();
#else
#if defined(STM32L0)
    stm32_power_hsi_off();
#endif //STM32L0
#endif //HSE_VALUE

#if defined (STM32L0)
    stm32_power_set_vrange(3);
#endif //STM32L0
}

static inline void stm32_power_down()
{
    __disable_irq();
#if (STM32_RTC_DRIVER)
    //disable RTC wakeup
    stm32_rtc_disable();
#endif //STM32_RTC_DRIVER

    //sleep deep
    SCB->SCR |= (1 << 2);
    PWR->CR |= PWR_CR_PDDS;
    PWR->CR |= PWR_CR_CWUF;
    __WFI();
}

static inline void stm32_power_set_mode(CORE* core, POWER_MODE mode)
{
    switch (mode)
    {
    case POWER_MODE_HIGH:
        stm32_power_hi(core);
        break;
    case POWER_MODE_LOW:
        stm32_power_lo(core);
        break;
    case POWER_MODE_STANDY:
        stm32_power_down();
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
    }
}

#endif //POWER_MANAGEMENT_SUPPORT

static inline void set_clock_source(CORE* core, STM32_CLOCK_SOURCE_TYPE cst, unsigned int value, bool on)
{
    switch (cst)
    {
    case STM32_CLOCK_SOURCE_DMA:
        on ? power_dma_on(core, value) : power_dma_off(core, value);
        break;
#if (POWER_MANAGEMENT_SUPPORT)
    case STM32_CLOCK_SOURCE_CORE:
        stm32_power_update_core_clock(core, value);
        break;
#endif //POWER_MANAGEMENT_SUPPORT
#if (HSE_VALUE)
    case STM32_CLOCK_SOURCE_HSE:
        on ? stm32_power_hse_on() : stm32_power_hse_off();
        break;
#endif //HSE_VALUE
#if defined (STM32L0)
    case STM32_CLOCK_SOURCE_MSI:
        stm32_power_msi_set_range(value);
        break;
#endif //STM32L0
    case STM32_CLOCK_SOURCE_PLL:
        on ? stm32_power_pll_on(value) : stm32_power_pll_off();
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
        break;
    }
}

#if (STM32_DECODE_RESET)
static inline void decode_reset_reason(CORE* core)
{
    core->power.reset_reason = RESET_REASON_UNKNOWN;
    if (RCC->CSR & RCC_CSR_LPWRRSTF)
        core->power.reset_reason = RESET_REASON_LOW_POWER;
    else if (RCC->CSR & (RCC_CSR_WWDGRSTF | RCC_CSR_WDGRSTF))
        core->power.reset_reason = RESET_REASON_WATCHDOG;
    else if (RCC->CSR & RCC_CSR_SFTRSTF)
        core->power.reset_reason = RESET_REASON_SOFTWARE;
    else if (RCC->CSR & RCC_CSR_PORRSTF)
        core->power.reset_reason = RESET_REASON_POWERON;
    else if (RCC->CSR & RCC_CSR_PADRSTF)
        core->power.reset_reason = RESET_REASON_PIN_RST;
    RCC->CSR |= RCC_CSR_RMVF;
}
#endif //STM32_DECODE_RESET

void stm32_power_init(CORE* core)
{
    int i;
#if (POWER_MANAGEMENT_SUPPORT)
    //clear power down flags
    PWR->CR |= PWR_CR_CSBF | PWR_CR_CWUF;
#if defined (STM32L0)
    PWR->CSR &= ~(PWR_CSR_EWUP2 | PWR_CSR_EWUP2);
#else
    PWR->CSR &= ~PWR_CSR_EWUP;
#endif //STM32L0
#endif //POWER_MANAGEMENT_SUPPORT

    core->power.write_count = 0;
    for (i = 0; i < DMA_COUNT; ++i)
        core->power.dma_count[i] = 0;
#if (STM32_DECODE_RESET)
#if defined(STM32F1)
    decode_reset_reason(core);
#endif
#endif //STM32_DECODE_RESET

    RCC->APB1ENR = 0;
    RCC->APB2ENR = 0;

#if defined(STM32F1) || defined(STM32L0)
    RCC->AHBENR = 0;
#elif defined(STM32F2) || defined(STM32F4)
    RCC->AHB1ENR = 0;
    RCC->AHB2ENR = 0;
    RCC->AHB3ENR = 0;
#endif

    RCC->CFGR = 0;
#if defined(STM32F10X_CL) || defined(STM32F100)
    RCC->CFGR2 = 0;
#endif

#if (POWER_MANAGEMENT_SUPPORT) && (LOW_POWER_ON_STARTUP)
#if defined (STM32L0)
    stm32_power_msi_set_range(MSI_RANGE);
    stm32_power_set_vrange(3);
#endif //STM32L0
#else
    stm32_power_enable_pll(core);
    stm32_power_set_core_clock(core, STM32_CLOCK_SOURCE_PLL);
#endif
}

bool stm32_power_request(CORE* core, IPC* ipc)
{
    bool need_post = false;
    switch (ipc->cmd)
    {
    case STM32_POWER_GET_CLOCK:
        ipc->param2 = get_clock(ipc->param1);
        need_post = true;
        break;
    case STM32_POWER_SET_CLOCK_SOURCE:
        set_clock_source(core, (STM32_CLOCK_SOURCE_TYPE)ipc->param1, ipc->param2, ipc->param3);
        need_post = true;
        break;
#if (STM32_DECODE_RESET)
    case STM32_POWER_GET_RESET_REASON:
        ipc->param2 = get_reset_reason(core);
        need_post = true;
        break;
#endif //STM32_DECODE_RESET
#if (POWER_MANAGEMENT_SUPPORT)
    case POWER_SET_MODE:
        //no return
        stm32_power_set_mode(core, ipc->param1);
        need_post = true;
        break;
#endif //POWER_MANAGEMENT_SUPPORT
    default:
        error(ERROR_NOT_SUPPORTED);
        need_post = true;
    }
    return need_post;
}
