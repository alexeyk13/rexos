/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "stm32_power.h"
#include "../../userspace/lib/stdio.h"
#include "../../sys/sys_call.h"
#include "../../userspace/core/stm32.h"
#include "stm32_config.h"

#if defined(STM32F1)
#define MAX_APB2                             72000000
#define MAX_APB1                             36000000
#define MAX_ADC1                             14000000
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
#endif

#if defined(STM32F1)
#define PPRE1                                8
#define PPRE2                                11
#elif defined(STM32F2) || defined(STM32F4)
#define PPRE1                                10
#define PPRE2                                13
#endif

#ifndef RCC_CSR_WDGRSTF
#define RCC_CSR_WDGRSTF RCC_CSR_IWDGRSTF
#endif

#ifndef RCC_CSR_PADRSTF
#define RCC_CSR_PADRSTF RCC_CSR_PINRSTF
#endif

#if defined(STM32F100)
static inline void setup_pll(int mul, int div)
{
    RCC->CFGR &= ~((0xf << 18) | (1 << 16));
    RCC->CFGR2 &= ~0xf;
    RCC->CFGR2 |= (div - 1) << 0;
    RCC->CFGR |= ((mul - 2) << 18);
#if (HSE_VALUE)
    if (RCC->CR & RCC_CR_HSEON)
        RCC->CFGR |= (1 << 16);
#endif
}

static inline int get_pll_clock()
{
    int pllsrc = HSI_VALUE / 2;
#if (HSE_VALUE)
    if (RCC->CFGR & (1 << 16))
        pllsrc = HSE_VALUE / ((RCC->CFGR2 & 0xf) + 1);
#endif
    return pllsrc * (((RCC->CFGR >> 18) & 0xf) + 2);
}

#elif defined(STM32F10X_CL)
static inline void setup_pll(int mul, int div, int pll2_mul, int pll2_div)
{
    RCC->CR &= ~RCC_CR_PLL2ON;
    RCC->CFGR &= ~((0xf << 18) | (1 << 16));
    RCC->CFGR2 &= ~(0xf | (1 << 16));
    if (pll2_mul && pll2_div)
    {
        RCC->CFGR2 |= ((pll2_div - 1) << 4) | ((pll2_mul - 2) << 8);
        //turn PLL2 ON
        RCC->CR |= RCC_CR_PLL2ON;
        while (!(RCC->CR & RCC_CR_PLL2RDY)) {}
        RCC->CFGR2 |= (1 << 16);
    }
    RCC->CFGR2 |= (div - 1) << 0;
    RCC->CFGR |= ((mul - 2) << 18);
#if (HSE_VALUE)
    if (RCC->CR & RCC_CR_HSEON)
        RCC->CFGR |= (1 << 16);
#endif
}

static inline int get_pll_clock()
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
static inline void setup_pll(int mul, int div)
{
    RCC->CFGR &= ~((0xf << 18) | (1 << 16) | (1 << 17));
    RCC->CFGR |= ((mul - 2) << 18);
#if (HSE_VALUE)
    if (RCC->CR & RCC_CR_HSEON)
        RCC->CFGR |= (1 << 16) | ((div - 1)  << 17);
#endif
}

static inline int get_pll_clock()
{
    int pllsrc = HSI_VALUE / 2;
#if (HSE_VALUE)
    if (RCC->CFGR & (1 << 16))
        pllsrc = HSE_VALUE / (((RCC->CFGR >> 17) & 0x1) + 1);
#endif
    return pllsrc * (((RCC->CFGR >> 18) & 0xf) + 2);
}

#elif defined (STM32F2) || defined (STM32F4)
static inline void setup_pll(int m, int n, int p)
{
    RCC->PLLCFGR &= ~((0x3f << 0) | (0x1ff << 6) | (3 << 16) | (1 << 22));
    RCC->PLLCFGR = (m << 0) | (n << 6) | (((p >> 1) -1) << 16);
#if (HSE_VALUE)
    if (RCC->CR & RCC_CR_HSEON)
        RCC->PLLCFGR = (1 << 22);
#endif
}

static inline int get_pll_clock()
{
    int pllsrc = HSI_VALUE;
#if (HSE_VALUE)
    if (RCC->CFGR & (1 << 22))
        pllsrc = HSE_VALUE;
#endif
    return pllsrc / (RCC->PLLCFGR & 0x3f) * ((RCC->PLLCFGR  >> 6) & 0x1ff) / ((((RCC->PLLCFGR  >> 16) & 0x3) + 1) << 1);
}
#endif

int get_core_clock()
{
    switch (RCC->CFGR & (3 << 2))
    {
    case RCC_CFGR_SWS_HSI:
        return HSI_VALUE;
        break;
    case RCC_CFGR_SWS_HSE:
#if (HSE_VALUE)
        return HSE_VALUE;
#endif
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
    if (RCC->CFGR & (1 << (PPRE2 + 2)))
        div = 1 << (((RCC->CFGR >> PPRE2) & 3) + 1);
    return get_ahb_clock() /div;
}

int get_apb1_clock()
{
    int div = 1;
    if (RCC->CFGR & (1 << (PPRE1 + 2)))
        div = 1 << (((RCC->CFGR >> PPRE1) & 3) + 1);
    return get_ahb_clock() / div;
}

#if (ADC_DRIVER)
int get_adc_clock()
{
#if defined(STM32F1)
    return get_apb2_clock() / ((((RCC->CFGR >> 14) & 3) + 1) * 2);
#endif
}

#endif

RESET_REASON get_reset_reason()
{
    RESET_REASON res = RESET_REASON_UNKNOWN;
    if (RCC->CSR & RCC_CSR_LPWRRSTF)
        res = RESET_REASON_LOW_POWER;
    else if (RCC->CSR & (RCC_CSR_WWDGRSTF | RCC_CSR_WDGRSTF))
        res = RESET_REASON_WATCHDOG;
    else if (RCC->CSR & RCC_CSR_SFTRSTF)
        res = RESET_REASON_SOFTWARE;
    else if (RCC->CSR & RCC_CSR_PORRSTF)
        res = RESET_REASON_POWERON;
    else if (RCC->CSR & RCC_CSR_PADRSTF)
        res = RESET_REASON_PIN_RST;
    return res;
}

void setup_clock(int param1, int param2, int param3)
{
    int i, bus, pll_clock;
    //1. switch to HSI (if not already)
    if (((RCC->CFGR >> 2) & 3) != RCC_CFGR_SW_HSI)
    {
        RCC->CFGR &= ~3;
        while ((RCC->CFGR & (3 << 2)) != RCC_CFGR_SWS_HSI) {}
    }
    //2. try to turn HSE on (if not already, and if HSE_VALUE is set)
#if (HSE_VALUE)
    if ((RCC->CR & RCC_CR_HSEON) == 0)
    {
        RCC->CR |= RCC_CR_HSEON;
        for (i = 0; i < HSE_STARTUP_TIMEOUT; ++i)
            if (RCC->CR & RCC_CR_HSERDY)
                break;
    }
#endif
    //3. setup pll
    RCC->CR &= ~RCC_CR_PLLON;
#if defined(STM32F10X_CL)
    setup_pll(param1, param2, param3 & 0xffff, (param3 >> 16));
#elif defined(STM32F1)
    setup_pll(param1, param2);
#elif defined(STM32F2) || defined(STM32F4)
    setup_pll(param1, param2, param3);
#endif
    //turn PLL on
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY)) {}
    pll_clock = get_pll_clock();

    //4. setup bases
    RCC->CFGR &= ~((0xf << 4) | (0x7 << PPRE1) | (0x7 << PPRE2));
    //AHB. Can operates at maximum clock
    //APB2.
    for (i = 1, bus = 0; (i <= 16) && (pll_clock / i > MAX_APB2); i *= 2, ++bus) {}
    if (bus)
        RCC->CFGR |= (4 | (bus - 1)) << PPRE2;
    //APB1.
    for (; (i <= 16) && (pll_clock / i > MAX_APB1); i *= 2, ++bus) {}
    if (bus)
        RCC->CFGR |= (4 | (bus - 1)) << PPRE1;

    //5. tune flash latency
#if defined(STM32F1) && !defined(STM32F100)
    FLASH->ACR = FLASH_ACR_PRFTBE | (pll_clock / 24000000);
#elif defined(STM32F2) || defined(STM32F4)
    FLASH->ACR = FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN | (pll_clock / 30000000);
#endif
    //6. switch to PLL
    RCC->CFGR |= RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & (3 << 2)) != RCC_CFGR_SWS_PLL) {}
}

void update_clock(int param1, int param2, int param3)
{
    unsigned int apb1 = RCC->APB1ENR;
    RCC->APB1ENR = 0;
    unsigned int apb2 = RCC->APB2ENR;
    RCC->APB2ENR = 0;
#if defined(STM32F1)
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

    RCC->CFGR = 0;
#if defined(STM32F10X_CL) || defined(STM32F100)
    RCC->CFGR2 = 0;
#endif

#if defined(STM32F10X_CL)
    setup_clock(PLL_MUL, PLL_DIV, PLL2_MUL | (PLL2_DIV << 16));
#elif defined(STM32F1)
    setup_clock(PLL_MUL, PLL_DIV);
#elif defined(STM32F2) || defined(STM32F4)
    setup_clock(PLL_M, PLL_N, PLL_P);
#endif

#if defined(STM32F1)
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
#if (ADC_DRIVER)
    case STM32_CLOCK_ADC:
        res = get_adc_clock();
        break;
#endif //ADC_DRIVER
    default:
        error(ERROR_INVALID_PARAMS);
    }
    return res;
}

#if (SYS_INFO)
void stm32_power_info()
{
    printf("Core clock: %d\n\r", get_core_clock());
    printf("AHB clock: %d\n\r", get_ahb_clock());
    printf("APB1 clock: %d\n\r", get_apb1_clock());
    printf("APB2 clock: %d\n\r", get_apb2_clock());
#if (ADC_DRIVER)
    printf("ADC clock: %d\n\r", get_adc_clock());
#endif
    printf("Reset reason: ");
    switch (get_reset_reason())
    {
    case RESET_REASON_LOW_POWER:
        printf("low power");
        break;
    case RESET_REASON_WATCHDOG:
        printf("watchdog");
        break;
    case RESET_REASON_SOFTWARE:
        printf("software");
        break;
    case RESET_REASON_POWERON:
        printf("power ON");
        break;
    case RESET_REASON_PIN_RST:
        printf("pin reset");
        break;
    default:
        printf("unknown");
    }
    printf("\n\r");
}
#endif

void backup_on(CORE* core)
{
    if (core->backup_count++ == 0)
        //enable POWER and BACKUP interface
        RCC->APB1ENR |= RCC_APB1ENR_PWREN | RCC_APB1ENR_BKPEN;
}

void backup_off(CORE* core)
{
    if (--core->backup_count == 0)
        //enable POWER and BACKUP interface
        RCC->APB1ENR &= ~(RCC_APB1ENR_PWREN | RCC_APB1ENR_BKPEN);
}

void backup_write_enable(CORE* core)
{
    if (core->backup_count)
    {
        if (core->write_count++ == 0)
            PWR->CR |= PWR_CR_DBP;
    }
}

void backup_write_protect(CORE* core)
{
    if (core->backup_count)
    {
        if (--core->write_count == 0)
            PWR->CR &= ~PWR_CR_DBP;
    }
}

#if (ADC_DRIVER)
void stm32_adc_on()
{
#if defined(STM32F1)
    int apb2, psc;
    apb2 = get_apb2_clock();
    for(psc = 2; psc < 8 && apb2 / psc > MAX_ADC1; psc += 2) {}
    RCC->CFGR &= ~(3 << 14);
    RCC->CFGR |= (psc / 2 - 1) << 14;
    //enable clock
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
#endif
}

void stm32_adc_off()
{
#if defined(STM32F1)
    RCC->APB2ENR &= ~RCC_APB2ENR_ADC1EN;
#endif
}
#endif //ADC_DRIVER

void stm32_power_init(CORE* core)
{
    core->backup_count = core->write_count = 0;

    RCC->APB1ENR = 0;
    RCC->APB2ENR = 0;

#if defined(STM32F1)
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

#if defined(STM32F10X_CL)
    setup_clock(PLL_MUL, PLL_DIV, PLL2_MUL | (PLL2_DIV << 16));
#elif defined(STM32F1)
    setup_clock(PLL_MUL, PLL_DIV);
#elif defined(STM32F2) || defined(STM32F4)
    setup_clock(PLL_M, PLL_N, PLL_P);
#endif
}
