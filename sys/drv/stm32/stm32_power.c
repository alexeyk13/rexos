/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "stm32_power.h"
#include "stm32_core_private.h"
#if (SYS_INFO)
#include "../../../userspace/lib/stdio.h"
#endif

#if defined(STM32F1)
#define MAX_APB2                             72000000
#define MAX_APB1                             36000000
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

#elif defined(STM32L0)
static inline void setup_pll(int mul, int div)
{
    unsigned int pow;
    for (pow = 0; mul > 4; mul /= 2, pow++) {}
    RCC->CFGR &= ~((0xf << 18) | (3 << 22) | (1 << 16));
    RCC->CFGR |= (((pow << 1) | (mul - 3)) << 18);
#if (HSE_VALUE)
    RCC->CFGR |= (((div - 1) << 22) | ((pow << 1) | (mul - 3)) << 18) | (1 << 16);
#else
    RCC->CFGR |= (((div - 1) << 22) | ((pow << 1) | (mul - 3)) << 18);
#endif
}

static inline int get_pll_clock()
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
#if defined(STM32L0)
    case RCC_CFGR_SWS_MSI:
        return 65536 * (2 << ((RCC->ICSCR >> 13) & 7));
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

#if defined(STM32F1)
int get_adc_clock()
{
    return get_apb2_clock() / ((((RCC->CFGR >> 14) & 3) + 1) * 2);
}
#endif

RESET_REASON get_reset_reason(CORE* core)
{
    return core->power.reset_reason;
}

void setup_clock(int param1, int param2, int param3)
{
    unsigned int pll_clock;
    //1. switch to internal RC(HSI/MSI) (if not already)
    if (RCC->CFGR & (3 << 2))
    {
        RCC->CFGR &= ~3;
        while (RCC->CFGR & (3 << 2)) {}
    }
    //2. try to turn HSE on (if not already, and if HSE_VALUE is set)
#if (HSE_VALUE)
    if ((RCC->CR & RCC_CR_HSEON) == 0)
    {
#if defined(STM32L0) && !(LSE_VALUE)
        RCC->CR &= ~(3 << 20);
        RCC->CR |= (30 - __builtin_clz(HSE_VALUE / 1000000)) << 20;
#endif

#if (HSE_BYPASS)
        RCC->CR |= RCC_CR_HSEON | RCC_CR_HSEBYP;
#else
        RCC->CR |= RCC_CR_HSEON;
#endif
#if defined(HSE_STARTUP_TIMEOUT)
        int i;
        for (i = 0; i < HSE_STARTUP_TIMEOUT; ++i)
            if (RCC->CR & RCC_CR_HSERDY)
                break;
#else
        while ((RCC->CR & RCC_CR_HSERDY) == 0) {}
#endif
    }
//we need to turn on HSI on STM32L0
#elif defined(STM32L0)
    if ((RCC->CR & RCC_CR_HSION) == 0)
    {
        RCC->CR |= RCC_CR_HSION;
        while ((RCC->CR & RCC_CR_HSIRDY) == 0) {}
    }
#endif //HSE_VALUE
    //3. setup pll
    RCC->CR &= ~RCC_CR_PLLON;
#if defined(STM32F10X_CL)
    setup_pll(param1, param2, param3 & 0xffff, (param3 >> 16));
#elif defined(STM32F1) || defined(STM32L0)
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
    //on STM32L0 APB1/APB2 can operates at maximum speed, save few flash bytes here
#ifndef STM32L0
    unsigned int i, bus;
    //APB2.
    for (i = 1, bus = 0; (i <= 16) && (pll_clock / i > MAX_APB2); i *= 2, ++bus) {}
    if (bus)
        RCC->CFGR |= (4 | (bus - 1)) << PPRE2;
    //APB1.
    for (; (i <= 16) && (pll_clock / i > MAX_APB1); i *= 2, ++bus) {}
    if (bus)
        RCC->CFGR |= (4 | (bus - 1)) << PPRE1;
#endif

    //5. tune flash latency
#if defined(STM32F1) && !defined(STM32F100)
    FLASH->ACR = FLASH_ACR_PRFTBE | (pll_clock / 24000000);
#elif defined(STM32F2) || defined(STM32F4)
    FLASH->ACR = FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN | (pll_clock / 30000000);
#elif defined(STM32L0)
    FLASH->ACR = FLASH_ACR_PRE_READ | (pll_clock / 16000001);
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

    RCC->CFGR = 0;
#if defined(STM32F10X_CL) || defined(STM32F100)
    RCC->CFGR2 = 0;
#endif

#if defined(STM32F10X_CL)
    setup_clock(PLL_MUL, PLL_DIV, PLL2_MUL | (PLL2_DIV << 16));
#elif defined(STM32F1) || defined(STM32L0)
    setup_clock(PLL_MUL, PLL_DIV, 0);
#elif defined(STM32F2) || defined(STM32F4)
    setup_clock(PLL_M, PLL_N, PLL_P);
#endif

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

#if (SYS_INFO)
void stm32_power_info(CORE* core)
{
    printd("Core clock: %d\n\r", get_core_clock());
    printd("AHB clock: %d\n\r", get_ahb_clock());
    printd("APB1 clock: %d\n\r", get_apb1_clock());
    printd("APB2 clock: %d\n\r", get_apb2_clock());
#if defined(STM32F1)
    printd("ADC clock: %d\n\r", get_adc_clock());
#endif
    printd("Reset reason: ");
    switch (core->power.reset_reason)
    {
    case RESET_REASON_LOW_POWER:
        printd("low power");
        break;
    case RESET_REASON_WATCHDOG:
        printd("watchdog");
        break;
    case RESET_REASON_SOFTWARE:
        printd("software");
        break;
    case RESET_REASON_POWERON:
        printd("power ON");
        break;
    case RESET_REASON_PIN_RST:
        printd("pin reset");
        break;
    default:
        printd("unknown");
    }
    printd("\n\r");
}
#endif

#if defined(STM32F1)
void dma_on(CORE* core, unsigned int index)
{
    if (core->power.dma_count[index]++ == 0)
        RCC->AHBENR |= 1 << index;
}

void dma_off(CORE* core, unsigned int index)
{
    if (--core->power.dma_count[index] == 0)
        RCC->AHBENR &= ~(1 << index);
}
#endif

#if defined(STM32F1)
void stm32_usb_power_on()
{
    int core;
    core = get_core_clock();
    if (core == 72000000)
        RCC->CFGR &= ~(1 << 22);
    else if (core == 48000000)
        RCC->CFGR |= 1 << 22;
    else
    {
        error(ERROR_NOT_SUPPORTED);
        return;
    }
#if defined(STM32F10X_CL)
    //enable clock
    RCC->AHBENR |= RCC_AHBENR_OTGFSEN;
    //F100, F101, F103
#elif defined(STM32F1)
    RCC->APB1ENR |= RCC_APB1ENR_USBEN;
#endif
}

void stm32_usb_power_off()
{
#if defined(STM32F10X_CL)
    //disable clock
    RCC->AHBENR &= ~RCC_AHBENR_OTGFSEN;
    //F100, F101, F103
#elif defined(STM32F1)
    RCC->APB1ENR &= ~RCC_APB1ENR_USBEN;
#endif
}
#endif

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

void stm32_power_init(CORE* core)
{
    core->power.write_count = 0;
#if defined(STM32F1)
    core->power.dma_count[0] = core->power.dma_count[1] = 0;
    decode_reset_reason(core);
#endif

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

#if defined(STM32F10X_CL)
    setup_clock(PLL_MUL, PLL_DIV, PLL2_MUL | (PLL2_DIV << 16));
#elif defined(STM32F1) || defined(STM32L0)
    setup_clock(PLL_MUL, PLL_DIV, 0);
#elif defined(STM32F2) || defined(STM32F4)
    setup_clock(PLL_M, PLL_N, PLL_P);
#endif


}

bool stm32_power_request(CORE* core, IPC* ipc)
{
    bool need_post = false;
    switch (ipc->cmd)
    {
#if (SYS_INFO)
    case IPC_GET_INFO:
        stm32_power_info(core);
        need_post = true;
        break;
#endif
    case STM32_POWER_GET_CLOCK:
        ipc->param1 = get_clock(ipc->param1);
        need_post = true;
        break;
    case STM32_POWER_UPDATE_CLOCK:
        update_clock(ipc->param1, ipc->param2, ipc->param3);
        need_post = true;
        break;
    case STM32_POWER_GET_RESET_REASON:
        ipc->param1 = get_reset_reason(core);
        need_post = true;
        break;
#if defined(STM32F1)
    case STM32_POWER_DMA_ON:
        dma_on(core, ipc->param1);
        need_post = true;
        break;
    case STM32_POWER_DMA_OFF:
        dma_off(core, ipc->param1);
        need_post = true;
        break;
#endif //STM32F1
#if defined(STM32F1)
    case STM32_POWER_USB_ON:
        stm32_usb_power_on();
        need_post = true;
        break;
    case STM32_POWER_USB_OFF:
        stm32_usb_power_off(core);
        need_post = true;
        break;
#endif //STM32F1
    default:
        ipc_set_error(ipc, ERROR_NOT_SUPPORTED);
        need_post = true;
    }
    return need_post;
}
