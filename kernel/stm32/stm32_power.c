/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2018, Alexey Kramarenko
    All rights reserved.
*/

#include "stm32_power.h"
#include "stm32_exo_private.h"
#include "stm32_rtc.h"
#include "stm32_timer.h"
#include "stm32_config.h"
#include "sys_config.h"
#include "../kerror.h"

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
#elif defined(STM32L1)
#define MAX_AHB                              32000000
#define MAX_APB2                             32000000
#define MAX_APB1                             32000000
#define HSI_VALUE                            16000000
#elif defined(STM32F0)
#define MAX_APB2                             0
#define MAX_APB1                             48000000
#define HSI_VALUE                            8000000
#endif

#if defined(STM32F1) || defined(STM32L1) || defined(STM32L0) || defined(STM32F0)
#define PPRE1_POS                            8
#define PPRE2_POS                            11
#elif defined(STM32F0)
#define PPRE1_POS                            8
#elif defined(STM32F2) || defined(STM32F4)
#define PPRE1_POS                            10
#define PPRE2_POS                            13
#endif

#ifndef RCC_CSR_WDGRSTF
#define RCC_CSR_WDGRSTF RCC_CSR_IWDGRSTF
#endif

#ifndef RCC_CSR_PADRSTF
#define RCC_CSR_PADRSTF RCC_CSR_PINRSTF
#endif

typedef enum {
    //only if HSE value is set
    STM32_CLOCK_SOURCE_HSE = 0,
    STM32_CLOCK_SOURCE_HSI,
#if defined(STM32L0) || defined(STM32L1)
    STM32_CLOCK_SOURCE_MSI,
#endif //STM32L0
    STM32_CLOCK_SOURCE_PLL,
} STM32_CLOCK_SOURCE_TYPE;

#if defined(STM32F100)
static bool stm32_power_pll_on(STM32_CLOCK_SOURCE_TYPE src)
{
    RCC->CFGR &= ~(0xf << 18);
    RCC->CFGR2 &= ~0xf;
    RCC->CFGR2 |= (PLL_DIV - 1) << 0;
    RCC->CFGR |= ((PLL_MUL - 2) << 18);

#if (HSE_VALUE)
    if (src == STM32_CLOCK_SOURCE_HSE)
        RCC->CFGR |= (1 << 16);
    else
#endif
        RCC->CFGR &= ~(1 << 16);
    //turn PLL on
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY)) {}
    return true;
}

static inline int stm32_power_get_pll_clock()
{
    int pllsrc = HSI_VALUE / 2;
#if (HSE_VALUE)
    if (RCC->CFGR & (1 << 16))
        pllsrc = HSE_VALUE / ((RCC->CFGR2 & 0xf) + 1);
#endif
    return pllsrc * (((RCC->CFGR >> 18) & 0xf) + 2);
}

#elif defined(STM32F10X_CL)
static bool stm32_power_pll_on(STM32_CLOCK_SOURCE_TYPE src)
{
    RCC->CFGR &= ~(0xf << 18);
    RCC->CFGR2 &= ~(0xf | (1 << 16));
#if (PLL2_DIV) && (PLL2_MUL)
    RCC->CFGR2 |= ((PLL2_DIV - 1) << 4) | ((PLL2_MUL - 2) << 8);
    //turn PLL2 ON
    RCC->CR |= RCC_CR_PLL2ON;
    while (!(RCC->CR & RCC_CR_PLL2RDY)) {}
    RCC->CFGR2 |= 1 << 16;
#endif //PLL2_DIV && PLL2_MUL
    RCC->CFGR2 |= (PLL_DIV - 1) << 0;
    RCC->CFGR |= (PLL_MUL - 2) << 18;

#if (HSE_VALUE)
    if (src == STM32_CLOCK_SOURCE_HSE)
        RCC->CFGR |= (1 << 16);
    else
#endif
        RCC->CFGR &= ~(1 << 16);
    //turn PLL on
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY)) {}
    return true;
}

static inline int stm32_power_get_pll_clock()
{
    int pllsrc = HSI_VALUE / 2;
    int pllmul;
#if (HSE_VALUE)
    int preddivsrc = HSE_VALUE;
    int pllmul2;
    if (RCC->CFGR & (1 << 16))
    {
        if (RCC->CFGR2 & (1 << 16))
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
static bool stm32_power_pll_on(STM32_CLOCK_SOURCE_TYPE src)
{
    RCC->CFGR &= ~((0xf << 18) | (1 << 17));
    RCC->CFGR |= ((PLL_MUL - 2) << 18);
    RCC->CFGR |= ((PLL_DIV - 1) << 17);

#if (HSE_VALUE)
    if (src == STM32_CLOCK_SOURCE_HSE)
        RCC->CFGR |= (1 << 16);
    else
#endif
        RCC->CFGR &= ~(1 << 16);
    //turn PLL on
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY)) {}
    return true;
}

static inline int stm32_power_get_pll_clock()
{
    int pllsrc = HSI_VALUE / 2;
#if (HSE_VALUE)
    if (RCC->CFGR & (1 << 16))
        pllsrc = HSE_VALUE / (((RCC->CFGR >> 17) & 0x1) + 1);
#endif
    return pllsrc * (((RCC->CFGR >> 18) & 0xf) + 2);
}

#elif defined(STM32L0) || defined(STM32L1)

static void stm32_power_set_vrange(int vrange)
{
    // wait stable
    while((PWR->CSR & PWR_CSR_VOSF) != 0);
    PWR->CR = ((PWR->CR) & ~(3 << 11)) | ((vrange & 3) << 11);
    // wait stable
    while((PWR->CSR & PWR_CSR_VOSF) != 0);
}

static inline void stm32_power_hsi_on()
{
    RCC->CR |= RCC_CR_HSION;
    while ((RCC->CR & RCC_CR_HSIRDY) == 0) {}
}

#if (POWER_MANAGEMENT)
#if (HSE_VALUE)
    //
#else
#if defined(STM32L0) || defined(STM32L1)
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
#endif //STM32L0 || STM32L1
#endif // HSI || MSI
#endif //POWER_MANAGEMENT


static bool stm32_power_pll_on(STM32_CLOCK_SOURCE_TYPE src)
{
    unsigned int pow, mul;
    mul = PLL_MUL;
    for (pow = 0; mul > 4; mul /= 2, pow++) {}

    RCC->CFGR &= ~((0xf << 18) | (3 << 22));
    RCC->CFGR |= (((pow << 1) | (mul - 3)) << 18);
    RCC->CFGR |= (((PLL_DIV - 1) << 22) | ((pow << 1) | (mul - 3)) << 18);

    stm32_power_set_vrange(1);

#if (HSE_VALUE)
    if (src == STM32_CLOCK_SOURCE_HSE)
        RCC->CFGR |= (1 << 16);
    else
#endif
    {
        stm32_power_hsi_on();
        RCC->CFGR &= ~(1 << 16);
    }
    //turn PLL on
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY)) {}
    return true;
}

static int get_msi_clock()
{
    return 65536 * (1 << ((RCC->ICSCR >> 13) & 7));
}

static inline int stm32_power_get_pll_clock()
{
    unsigned int pllsrc = HSI_VALUE;
    unsigned int mul = (1 << ((RCC->CFGR >> 19) & 7)) * (3 + ((RCC->CFGR >> 18) & 1));
    unsigned int div = ((RCC->CFGR >> 22) & 3) + 1;

#if (HSE_VALUE)
    if (RCC->CFGR & (1 << 16))
        pllsrc = HSE_VALUE;
#endif
    return pllsrc * mul / div;
}

#elif defined (STM32F2) || defined (STM32F4)
static inline bool stm32_power_pll_on(STM32_CLOCK_SOURCE_TYPE src)
{
    RCC->PLLCFGR &= ~((0x3f << 0) | (0x1ff << 6) | (3 << 16));
    RCC->PLLCFGR = (PLL_M << 0) | (PLL_N << 6) | (((PLL_P >> 1) -1) << 16);

#if (HSE_VALUE)
    if (src == STM32_CLOCK_SOURCE_HSE)
        RCC->PLLCFGR |= (1 << 22);
    else
#endif
        RCC->PLLCFGR &= ~(1 << 22);
    //turn PLL on
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY)) {}
    return true;
}

static inline int stm32_power_get_pll_clock()
{
    int pllsrc = HSI_VALUE;
#if (HSE_VALUE)
    if (RCC->PLLCFGR & (1 << 22))
        pllsrc = HSE_VALUE;
#endif
    return pllsrc / (RCC->PLLCFGR & 0x3f) * ((RCC->PLLCFGR  >> 6) & 0x1ff) / ((((RCC->PLLCFGR  >> 16) & 0x3) + 1) << 1);
}

#elif defined (STM32F0)
static bool stm32_power_pll_on(STM32_CLOCK_SOURCE_TYPE src)
{
    RCC->CFGR &= ~((0xf << 18) || (0x03 << 15)); //PLLMUL & PLLSRC
    RCC->CFGR |= (PLL_MUL - 2) << 18;
    RCC->CFGR2 = (PLL_DIV - 1) & 0xf;

    //Actually there is NO PLL_SRC0 bit on 072 at least.
    if (src == STM32_CLOCK_SOURCE_HSE)
        RCC->CFGR |= (0x02 << 15);
    else
#if defined(STM32F042) || defined(STM32F072) || defined(STM32F092)
        RCC->CFGR |= (0x01 << 15);
#endif
    //turn PLL on
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY)) {}
    return true;
}

static inline int stm32_power_get_pll_clock()
{
#if defined(STM32F042) || defined(STM32F072) || defined(STM32F092)
    int pllsrc = HSI_VALUE;
#else
    int pllsrc = HSI_VALUE / 2;
#endif
#if (HSE_VALUE)
    if (RCC->CFGR & (1 << 16))
        pllsrc = HSE_VALUE;
#endif
    return (pllsrc / ((RCC->CFGR2 & 0xf) + 1)) * (((RCC->CFGR >> 18) & 0xf) + 2);
}
#endif //STM32F0

int get_core_clock_internal()
{
    switch (RCC->CFGR & (3 << 2))
    {
    case RCC_CFGR_SWS_HSI:
        return HSI_VALUE;
        break;
#if defined(STM32L0) || defined(STM32L1)
    case RCC_CFGR_SWS_MSI:
        return get_msi_clock();
        break;
#endif
    case RCC_CFGR_SWS_HSE:
        return HSE_VALUE;
        break;
    case RCC_CFGR_SWS_PLL:
        return stm32_power_get_pll_clock();
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
    return get_core_clock_internal() / div;
}

#if (MAX_APB2)
int get_apb2_clock()
{
    int div = 1;
    if (RCC->CFGR & (1 << (PPRE2_POS + 2)))
        div = 1 << (((RCC->CFGR >> PPRE2_POS) & 3) + 1);
    return get_ahb_clock() /div;
}
#endif //MAX_APB2

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

#if (HSE_VALUE)
static bool stm32_power_hse_on()
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
                return true;
        return false;
#else
        while ((RCC->CR & RCC_CR_HSERDY) == 0) {}
#endif //HSE_STARTUP_TIMEOUT
    }
    return true;
}

#if (POWER_MANAGEMENT)
static void stm32_power_hse_off()
{
    RCC->CR &= ~RCC_CR_HSEON;
}
#endif //POWER_MANAGEMENT
#endif //HSE_VALUE

#if (POWER_MANAGEMENT)
static void stm32_power_pll_off()
{
    RCC->CR &= ~RCC_CR_PLLON;
}
#endif //POWER_MANAGEMENT

static unsigned int stm32_power_get_bus_prescaller(unsigned int clock, unsigned int max)
{
    int i;
    for (i = 0; i <= 4; ++i)
        if ((clock >> i) <= max)
            break;
    return i ? i + 3 : 0;
}

unsigned int stm32_power_get_clock(int type)
{
    unsigned int res = 0;
    switch (type)
    {
    case POWER_CORE_CLOCK:
        res = get_core_clock_internal();
        break;
    case POWER_BUS_CLOCK:
        res = get_ahb_clock();
        break;
    case STM32_CLOCK_APB2:
#if (MAX_APB2)
        res = get_apb2_clock();
        break;
#endif //MAX_APB2
    case STM32_CLOCK_APB1:
        res = get_apb1_clock();
        break;
#if defined(STM32F1)
    case STM32_CLOCK_ADC:
        res = get_adc_clock();
        break;
#endif
    default:
        kerror(ERROR_INVALID_PARAMS);
    }
    return res;
}

static void stm32_power_set_clock_source(STM32_CLOCK_SOURCE_TYPE src)
{
    unsigned int sw, core_clock, pll_src;
    pll_src = STM32_CLOCK_SOURCE_HSI;

#if (HSE_VALUE)
    if ((src == STM32_CLOCK_SOURCE_HSE) && !stm32_power_hse_on())
        src = STM32_CLOCK_SOURCE_HSI;
    if ((src == STM32_CLOCK_SOURCE_PLL) && stm32_power_hse_on())
        pll_src = STM32_CLOCK_SOURCE_HSE;
#endif //HSE_VALUE

    switch (src)
    {
#if defined (STM32L0) || defined (STM32L1)
    case STM32_CLOCK_SOURCE_MSI:
        sw = RCC_CFGR_SW_MSI;
        core_clock = get_msi_clock();
        break;
#endif
#if (HSE_VALUE)
    case STM32_CLOCK_SOURCE_HSE:
        sw = RCC_CFGR_SW_HSE;
        core_clock = HSE_VALUE;
        break;
#endif
    case STM32_CLOCK_SOURCE_PLL:
        if (stm32_power_pll_on(pll_src))
        {
            sw = RCC_CFGR_SW_PLL;
            core_clock = stm32_power_get_pll_clock();
            break;
        }
        //follow down
    default:
        sw = RCC_CFGR_SW_HSI;
        core_clock = HSI_VALUE;
    }

    //setup bases
    //AHB. Can operates at maximum clock
#if (MAX_APB2)
    //APB2
    RCC->CFGR = (RCC->CFGR & ~(7 << PPRE2_POS)) | (stm32_power_get_bus_prescaller(core_clock, MAX_APB2) << PPRE2_POS);
#endif //MAX_APB2
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
#elif defined(STM32L1)
    FLASH->ACR |= FLASH_ACR_ACC64;
    FLASH->ACR |= FLASH_ACR_PRFTEN;

    unsigned int range = ((PWR->CR >> 11) & 3);
    if( (range == 3 && core_clock > 2000000) ||
        (range == 2 && core_clock > 8000000) ||
        (range == 1 && core_clock > 16000000))
    {
        FLASH->ACR |= FLASH_ACR_LATENCY;
    }
    else
        FLASH->ACR &= ~FLASH_ACR_LATENCY;

#elif defined(STM32F0)
    FLASH->ACR = FLASH_ACR_PRFTBE | ((core_clock / 24000000) << 0);
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
}

#if (POWER_MANAGEMENT)
static void stm32_power_update_core_clock(EXO* exo, STM32_CLOCK_SOURCE_TYPE src)
{
    __disable_irq();
    //turn off all peripheral clocking
    unsigned int apb1 = RCC->APB1ENR;
    RCC->APB1ENR = 0;
    unsigned int apb2 = RCC->APB2ENR;
    RCC->APB2ENR = 0;
#if defined(STM32F1) || defined(STM32L1) || defined(STM32L0)
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

    stm32_power_set_clock_source(src);

    //restore buses
#if defined(STM32F1) || defined(STM32L1) || defined(STM32L0)
    RCC->AHBENR = ahb;
#elif defined (STM32F2) || defined(STM32F4)
    RCC->AHB1ENR = ahb1;
    RCC->AHB2ENR = ahb2;
    RCC->AHB3ENR = ahb3;
#endif
    RCC->APB1ENR = apb1;
    RCC->APB2ENR = apb2;

    stm32_timer_pm_event(exo);
    __enable_irq();
}

static inline void stm32_power_lo(EXO* exo)
{
#if defined (STM32L0) || defined(STM32L1)
    stm32_power_update_core_clock(exo, STM32_CLOCK_SOURCE_MSI);
#else
    stm32_power_update_core_clock(exo, STM32_CLOCK_SOURCE_HSI);
#endif

    stm32_power_pll_off();
#if (HSE_VALUE)
    stm32_power_hse_off();
#else
#if defined(STM32L0) || defined(STM32L1)
    stm32_power_hsi_off();
#endif //STM32L0
#endif //HSE_VALUE

#if defined(STM32L0) || defined(STM32L1)
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
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
#if (STANDBY_WKUP)
    PWR->CSR |= PWR_CSR_EWUP;
#endif //STANDBY_WKUP
    PWR->CR |= PWR_CR_CWUF;
    PWR->CR |= PWR_CR_PDDS;
    __WFI();
}

static inline void stm32_power_set_mode(EXO* exo, POWER_MODE mode)
{
    switch (mode)
    {
    case POWER_MODE_HIGH:
        stm32_power_update_core_clock(exo, STM32_CLOCK_SOURCE_PLL);
        break;
    case POWER_MODE_LOW:
        stm32_power_lo(exo);
        break;
    case POWER_MODE_STANDY:
        stm32_power_down();
        break;
    default:
        kerror(ERROR_NOT_SUPPORTED);
    }
}

#endif //POWER_MANAGEMENT

#if (STM32_DECODE_RESET)
void decode_reset_reason(EXO* exo)
//static inline void decode_reset_reason(EXO* exo)
{
#if !(POWER_MANAGEMENT) && !(STM32_RTC_DRIVER)
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;
#endif //!(POWER_MANAGEMENT) && !(STM32_RTC_DRIVER)
    if ((PWR->CSR & (PWR_CSR_WUF | PWR_CSR_SBF)) == (PWR_CSR_WUF | PWR_CSR_SBF))
        exo->power.reset_reason = RESET_REASON_WAKEUP;
    else
    {
        exo->power.reset_reason = RESET_REASON_UNKNOWN;
        if (RCC->CSR & RCC_CSR_LPWRRSTF)
            exo->power.reset_reason = RESET_REASON_LOW_POWER;
        else if (RCC->CSR & (RCC_CSR_WWDGRSTF | RCC_CSR_WDGRSTF))
            exo->power.reset_reason = RESET_REASON_WATCHDOG;
        else if (RCC->CSR & RCC_CSR_SFTRSTF)
            exo->power.reset_reason = RESET_REASON_SOFTWARE;
        else if (RCC->CSR & RCC_CSR_PORRSTF)
            exo->power.reset_reason = RESET_REASON_POWERON;
        else if (RCC->CSR & RCC_CSR_PADRSTF)
            exo->power.reset_reason = RESET_REASON_PIN_RST;
#if defined(STM32L0) || defined(STM32F0)
        else if (RCC->CSR & RCC_CSR_OBL)
            exo->power.reset_reason = RESET_REASON_OPTION_BYTES;
#endif //STM32L0
    }
    PWR->CR |= PWR_CR_CWUF | PWR_CR_CSBF;
    RCC->CSR |= RCC_CSR_RMVF;
#if !(POWER_MANAGEMENT) && !(STM32_RTC_DRIVER)
    RCC->APB1ENR &= ~RCC_APB1ENR_PWREN;
#endif //!(POWER_MANAGEMENT) && !(STM32_RTC_DRIVER)
}
#endif //STM32_DECODE_RESET

void stm32_power_init(EXO* exo)
{
    RCC->APB1ENR = 0;
    RCC->APB2ENR = 0;

#if defined(STM32F1) || defined(STM32L1) || defined(STM32L0) || defined(STM32F0)
    RCC->AHBENR = 0;
#else
    RCC->AHB1ENR = 0;
    RCC->AHB2ENR = 0;
    RCC->AHB3ENR = 0;
#endif

    RCC->CFGR = 0;

#if defined(STM32F10X_CL) || defined(STM32F100)
    RCC->CFGR2 = 0;
#elif defined(STM32F0)
    RCC->CFGR2 = 0;
    RCC->CFGR3 = 0;
#endif

#if (POWER_MANAGEMENT) || (STM32_RTC_DRIVER)
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;

#if defined(STM32L0) || defined(STM32L1)
#if (HSE_RTC_DIV)
    uint8_t value;
    uint32_t div = HSE_RTC_DIV;
    for (value = 0; div > 2; div >>= 1, value++) {}

#if defined(STM32L0)
    RCC->CR |= (value << 20);
#else
    RCC->CR |= (value << 29);
#endif

#endif // HSE_RTC_DIV
#endif // STM32L0 || STM32L1

#endif //(POWER_MANAGEMENT) || (STM32_RTC_DRIVER)
#if (POWER_MANAGEMENT)
#if (SYSCFG_ENABLED)
#if defined(STM32F1)
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;
    AFIO->MAPR = STM32F1_MAPR;
#else
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
#endif
#endif //SYSCFG_ENABLED
#if defined (STM32L0) || defined(STM32F03x) || defined(STM32F04x) || defined(STM32F05x)
    PWR->CSR &= ~(PWR_CSR_EWUP1 | PWR_CSR_EWUP2);
#elif defined (STM32L1)
    PWR->CSR &= ~(PWR_CSR_EWUP1 | PWR_CSR_EWUP2 | PWR_CSR_EWUP3);
#elif defined(STM32F07x) || defined(STM32F09x)
    PWR->CSR &= ~(PWR_CSR_EWUP1 | PWR_CSR_EWUP2 | PWR_CSR_EWUP3 | PWR_CSR_EWUP4 | PWR_CSR_EWUP5 | PWR_CSR_EWUP6 | PWR_CSR_EWUP7 | PWR_CSR_EWUP8);
#else
    PWR->CSR &= ~PWR_CSR_EWUP;
#endif //STM32L0
#endif //POWER_MANAGEMENT

#if (STM32_DECODE_RESET)
    decode_reset_reason(exo);
#endif //STM32_DECODE_RESET

    stm32_power_set_clock_source(STM32_CLOCK_SOURCE_PLL);
}

void stm32_power_request(EXO* exo, IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
    case POWER_GET_CLOCK:
        ipc->param2 = stm32_power_get_clock(ipc->param1);
        break;
#if (STM32_DECODE_RESET)
    case STM32_POWER_GET_RESET_REASON:
        ipc->param2 = exo->power.reset_reason;
        break;
#endif //STM32_DECODE_RESET
#if (POWER_MANAGEMENT)
    case POWER_SET_MODE:
        //no return
        stm32_power_set_mode(exo, ipc->param1);
        break;
#endif //POWER_MANAGEMENT
    default:
        kerror(ERROR_NOT_SUPPORTED);
    }
}
