/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: Alexey E. Kramarenko (alexeyk13@yandex.ru)
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
#elif defined(STM32H7)
#define MIN_CORE                             (60 * 1000000ul)
#define MAX_CORE                             (480 * 1000000ul)
#define MAX_AHB                              (240 * 1000000ul)
#define MAX_APB2                             (125 * 1000000ul)
#define MAX_APB1                             (125 * 1000000ul)
#define HSI_VALUE                            (64 * 1000000ul)
#define CSI_VALUE                            (4 * 1000000ul)

#define MAX_FLASH_WS0                        (70 * 1000000ul)
#define MAX_FLASH_WS1                        (140 * 1000000ul)
#define MAX_FLASH_WS2                        (210 * 1000000ul)
#define MAX_FLASH_WS3                        (225 * 1000000ul)
#define MHZ_TO_HZ(x)                         (x * 1000000ul)

typedef struct{
    uint32_t core;
    uint32_t ahb;
    uint32_t apb;
    uint32_t ws0;
    uint32_t ws1;
    uint32_t ws2;
    uint32_t ws3;
}MAX_CLOCK;
//                                         core            AHB             APB             WS0            WS1            WS2              WS3
static MAX_CLOCK max_clocks[4]={{MHZ_TO_HZ(480), MHZ_TO_HZ(240), MHZ_TO_HZ(125), MHZ_TO_HZ(70), MHZ_TO_HZ(140), MHZ_TO_HZ(210), MHZ_TO_HZ(225)}, // VOS0
                                {MHZ_TO_HZ(400), MHZ_TO_HZ(200), MHZ_TO_HZ(100), MHZ_TO_HZ(70), MHZ_TO_HZ(140), MHZ_TO_HZ(210), MHZ_TO_HZ(225)},
                                {MHZ_TO_HZ(300), MHZ_TO_HZ(150), MHZ_TO_HZ(75),  MHZ_TO_HZ(55), MHZ_TO_HZ(110), MHZ_TO_HZ(165), MHZ_TO_HZ(225)},
                                {MHZ_TO_HZ(200), MHZ_TO_HZ(100), MHZ_TO_HZ(50),  MHZ_TO_HZ(45), MHZ_TO_HZ(90),  MHZ_TO_HZ(135), MHZ_TO_HZ(180)}};

#define RCC_CFGR_SWS_PLL                     RCC_CFGR_SWS_PLL1
#define RCC_CFGR_SW_PLL                     RCC_CFGR_SW_PLL1

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
#if defined(STM32H7)
    STM32_CLOCK_SOURCE_CSI,
#endif //STM32L0
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
#elif  defined (STM32H7) //STM32F0


static inline int stm32_power_get_pll_src_clock()
{
    switch(RCC->PLLCKSELR & RCC_PLLCKSELR_PLLSRC)
    {
    case RCC_PLLCKSELR_PLLSRC_CSI:
    	return CSI_VALUE;
#if (HSE_VALUE)
    case RCC_PLLCKSELR_PLLSRC_HSE:
    	return  HSE_VALUE;
#endif // HSE_VALUE
    default:
    	return HSI_VALUE;
    }
}

static inline int stm32_power_get_pll_clock(uint32_t pll_num, STM32_PLL_OUT pll_out)
{
    int pllsrc = stm32_power_get_pll_src_clock();
    uint32_t div_reg;
    int div;
    pllsrc /= ((RCC->PLLCKSELR & RCC_PLLCKSELR_DIVM1_Msk) >> RCC_PLLCKSELR_DIVM1_Pos); // input PLL

    switch(pll_num)
    {
    case 2:
        div_reg = RCC->PLL2DIVR;
        break;
    case 3:
        div_reg = RCC->PLL3DIVR;
        break;
    default:
        div_reg = RCC->PLL1DIVR;
        break;
    }
    pllsrc *= ((div_reg & RCC_PLL1DIVR_N1) >> RCC_PLL1DIVR_N1_Pos) + 1;

    switch(pll_out)
    {
    case STM32_PLL_OUT_Q:
        div = (div_reg & RCC_PLL1DIVR_Q1) >> RCC_PLL1DIVR_Q1_Pos;
        break;
    case STM32_PLL_OUT_R:
        div = (div_reg & RCC_PLL1DIVR_R1) >> RCC_PLL1DIVR_R1_Pos;
        break;
    default:
        div = (div_reg & RCC_PLL1DIVR_P1) >> RCC_PLL1DIVR_P1_Pos;
        break;
    }
    pllsrc /= div + 1;
    return pllsrc;
}

static uint32_t get_pre_divider(uint32_t div, uint32_t wide)
{
    uint32_t pos = (1 << (wide - 1));
    if(div & pos){
        div = 2 << (div & (pos - 1));
    }else
        div = 1;
    return div;
}

static inline int stm32_power_get_pll_core_clock()
{
    return stm32_power_get_pll_clock(1, STM32_PLL_OUT_P) / get_pre_divider((RCC->D1CFGR & RCC_D1CFGR_D1CPRE) >> RCC_D1CFGR_D1CPRE_Pos, 4);
}

static inline void stm32_power_set_vos(uint32_t vos)
{
    RCC->APB4ENR |= RCC_APB4ENR_SYSCFGEN;
    if(vos)
    {
        SYSCFG->PWRCR = 0;
        PWR->D3CR  = (PWR->D3CR & ~PWR_D3CR_VOS) | ((4-vos) << PWR_D3CR_VOS_Pos);
    }else{
        PWR->D3CR  |= (3 << PWR_D3CR_VOS_Pos);
        SYSCFG->PWRCR = SYSCFG_PWRCR_ODEN;
    }
    while((PWR->D3CR & PWR_D3CR_VOSRDY) == 0);
}

void stm32_power_pll2_on()
{
    if(RCC->CR & RCC_CR_PLL2RDY)
        return;
    RCC->PLL2DIVR = (PLL2_N << RCC_PLL2DIVR_N2_Pos) | (PLL2_P << RCC_PLL2DIVR_P2_Pos) | (PLL2_Q << RCC_PLL2DIVR_Q2_Pos) | (PLL2_R << RCC_PLL2DIVR_R2_Pos);
    RCC->CR |= RCC_CR_PLL2ON;
    while (!(RCC->CR & RCC_CR_PLL2RDY)) {}
}

void stm32_power_pll3_on()
{
    if(RCC->CR & RCC_CR_PLL3RDY)
        return;
    RCC->PLL3DIVR = (PLL3_N << RCC_PLL3DIVR_N3_Pos) | (PLL3_P << RCC_PLL3DIVR_P3_Pos) | (PLL3_Q << RCC_PLL3DIVR_Q3_Pos) | (PLL3_R << RCC_PLL3DIVR_R3_Pos);
    RCC->CR |= RCC_CR_PLL3ON;
    while (!(RCC->CR & RCC_CR_PLL3RDY)) {}
}

static inline bool stm32_power_pll_on(STM32_CLOCK_SOURCE_TYPE src)
{
	uint32_t reg;
	uint32_t src_clock;

    // set pll
    reg = RCC_PLLCKSELR_PLLSRC_HSI;
    if(src == STM32_CLOCK_SOURCE_CSI)
        reg = RCC_PLLCKSELR_PLLSRC_CSI;
#if (HSE_VALUE)
	else
	    if (src == STM32_CLOCK_SOURCE_HSE)
	        reg = RCC_PLLCKSELR_PLLSRC_HSE;
#endif

    RCC->PLLCKSELR =(RCC->PLLCKSELR & ~(RCC_PLLCKSELR_PLLSRC_Msk | RCC_PLLCKSELR_DIVM1_Msk | RCC_PLLCKSELR_DIVM2_Msk | RCC_PLLCKSELR_DIVM3_Msk)) |
                                       (PLL_M << RCC_PLLCKSELR_DIVM1_Pos)  | (PLL_M << RCC_PLLCKSELR_DIVM2_Pos)  | (PLL_M << RCC_PLLCKSELR_DIVM3_Pos) | reg;
    src_clock = stm32_power_get_pll_src_clock() / PLL_M;
    reg = 0;
    if(src_clock > 2000000)
        reg = 1;
    if(src_clock > 4000000)
        reg = 2;
    if(src_clock > 8000000)
        reg = 3;


    RCC->PLLCFGR = (RCC->PLLCFGR & ~(RCC_PLLCFGR_PLL1FRACEN | RCC_PLLCFGR_PLL1VCOSEL | RCC_PLLCFGR_PLL1RGE)) | (RCC_PLLCFGR_DIVP1EN | (reg << RCC_PLLCFGR_PLL1RGE_Pos));
    RCC->PLL1DIVR =((PLL_N << RCC_PLL1DIVR_N1_Pos) | (PLL_P << RCC_PLL1DIVR_P1_Pos) | (PLL_Q << RCC_PLL1DIVR_Q1_Pos) | (PLL_R << RCC_PLL1DIVR_R1_Pos));
    //turn PLL on
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY)) {}
    return true;
}


#endif // --------------------------------------

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

int get_core_clock_internal()
{
#if defined(STM32H7)
   switch (RCC->CFGR & RCC_CFGR_SWS_Msk)
#else
    switch (RCC->CFGR & RCC_CFGR_SWS)
#endif //
    {
    case RCC_CFGR_SWS_HSI:
        return HSI_VALUE;
#if defined(STM32H7)
    case RCC_CFGR_SWS_CSI:
        return CSI_VALUE;
    case RCC_CFGR_SWS_PLL:
        return stm32_power_get_pll_core_clock();
#else
    case RCC_CFGR_SWS_PLL:
        return stm32_power_get_pll_clock();
#endif // STM32H7
#if defined(STM32L0) || defined(STM32L1)
    case RCC_CFGR_SWS_MSI:
        return get_msi_clock();
        break;
#endif
    case RCC_CFGR_SWS_HSE:
        return HSE_VALUE;
    }
    return 0;
}

#if defined(STM32H7)
static unsigned int stm32_power_get_prescaller(unsigned int clock, unsigned int max, uint32_t wide)
{
    int i;
    for (i = 0; i <= 4; ++i)
        if ((clock >> i) <= max)
            break;
    return i ? i + ((1 << (wide-1)) - 1) : 0;
}

int get_ahb_clock()
{
	return get_core_clock_internal() / get_pre_divider((RCC->D1CFGR & RCC_D1CFGR_HPRE) >> RCC_D1CFGR_HPRE_Pos, 4);
}

int get_apb1_clock()
{
	return get_ahb_clock() / get_pre_divider((RCC->D2CFGR & RCC_D2CFGR_D2PPRE1) >> RCC_D2CFGR_D2PPRE1_Pos, 3);
}

int get_apb2_clock()
{
	return get_ahb_clock() / get_pre_divider((RCC->D2CFGR & RCC_D2CFGR_D2PPRE2) >> RCC_D2CFGR_D2PPRE2_Pos, 3);
}



static inline void stm32_power_set_buses_safe()
{
    FLASH->ACR = (7 << FLASH_ACR_LATENCY_Pos) | (2 << FLASH_ACR_WRHIGHFREQ_Pos);
    RCC->D1CFGR = RCC_D1CFGR_D1CPRE_DIV8 | RCC_D1CFGR_HPRE_DIV8 | RCC_D1CFGR_D1PPRE_DIV8;
    RCC->D2CFGR =RCC_D2CFGR_D2PPRE1_DIV8 | RCC_D2CFGR_D2PPRE2_DIV8;
    RCC->D3CFGR =RCC_D3CFGR_D3PPRE_DIV8;
}

static void stm32_power_set_buses(uint32_t sys_clock, uint32_t vos)
{
    uint32_t ahb_clock;
    uint32_t div, div_apb;
    vos &= 0x03;
// set AXI & AHB
    div = stm32_power_get_prescaller(sys_clock, max_clocks[vos].ahb, 4);
    ahb_clock = sys_clock / get_pre_divider(div, 4);
// set max bus dividers
    div_apb = stm32_power_get_prescaller(ahb_clock, max_clocks[vos].apb, 3);
    RCC->D1CFGR =  (RCC->D1CFGR & ~(RCC_D1CFGR_D1PPRE_Msk | RCC_D1CFGR_HPRE_Msk)) | (div << RCC_D1CFGR_HPRE_Pos) | (div_apb << RCC_D1CFGR_D1PPRE_Pos);
    RCC->D2CFGR = (div_apb << RCC_D2CFGR_D2PPRE1_Pos) | (div_apb << RCC_D2CFGR_D2PPRE2_Pos);
    RCC->D3CFGR =div_apb << RCC_D3CFGR_D3PPRE_Pos;

// set_flash_latency
	uint32_t reg = 0;
	if(ahb_clock > max_clocks[vos].ws0)
		reg = (1 << FLASH_ACR_LATENCY_Pos) | (1 << FLASH_ACR_WRHIGHFREQ_Pos);
	if(ahb_clock > max_clocks[vos].ws1)
		reg = (2 << FLASH_ACR_LATENCY_Pos) | (2 << FLASH_ACR_WRHIGHFREQ_Pos);
	if(ahb_clock > max_clocks[vos].ws2)
		reg = (3 << FLASH_ACR_LATENCY_Pos) | (2 << FLASH_ACR_WRHIGHFREQ_Pos);
	if(ahb_clock > max_clocks[vos].ws3)
		reg = (4 << FLASH_ACR_LATENCY_Pos) | (2 << FLASH_ACR_WRHIGHFREQ_Pos);

	FLASH->ACR = reg;
	while(FLASH->ACR != reg);
}

static bool stm32_power_hse_on();
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
    stm32_power_set_vos(VOS_VALUE);
    switch (src)
    {
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
            core_clock = stm32_power_get_pll_core_clock();
            break;
        }
        //follow down
    default:
        sw = RCC_CFGR_SW_HSI;
        core_clock = HSI_VALUE;
    }

    stm32_power_set_buses_safe();
    __DSB();
    __DMB();
    __NOP();
    __NOP();

    //switch to source
    RCC->CFGR |= sw;
    while (((RCC->CFGR >> RCC_CFGR_SWS_Pos) & 3) != sw) {}
    __NOP();
    __NOP();

    RCC->D1CFGR = (RCC->D1CFGR & ~RCC_D1CFGR_D1CPRE) | (0 << RCC_D1CFGR_D1CPRE_Pos);
    stm32_power_set_buses(core_clock, VOS_VALUE);
}

#else
static unsigned int stm32_power_get_bus_prescaller(unsigned int clock, unsigned int max)
{
    int i;
    for (i = 0; i <= 4; ++i)
        if ((clock >> i) <= max)
            break;
    return i ? i + 3 : 0;
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
#endif // STM32F1

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

#endif//STM32H7

#if (POWER_MANAGEMENT)
static void stm32_power_pll_off()
{
    RCC->CR &= ~RCC_CR_PLLON;
}
#endif //POWER_MANAGEMENT


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
#endif // STM32F1
#if defined(STM32H7)
    case STM32_CLOCK_PLL1_Q:
        res = stm32_power_get_pll_clock(1,  STM32_PLL_OUT_Q);
        break;
    case STM32_CLOCK_PLL2_P:
        res = stm32_power_get_pll_clock(2,  STM32_PLL_OUT_P);
        break;
    case STM32_CLOCK_PLL2_Q:
        res = stm32_power_get_pll_clock(2,  STM32_PLL_OUT_Q);
        break;
    case STM32_CLOCK_PLL2_R:
        res = stm32_power_get_pll_clock(2,  STM32_PLL_OUT_R);
        break;
    case STM32_CLOCK_PLL3_P:
        res = stm32_power_get_pll_clock(3,  STM32_PLL_OUT_P);
        break;
    case STM32_CLOCK_PLL3_Q:
        res = stm32_power_get_pll_clock(3,  STM32_PLL_OUT_Q);
        break;
    case STM32_CLOCK_PLL3_R:
        res = stm32_power_get_pll_clock(3,  STM32_PLL_OUT_R);
        break;
#endif // STM32H7
    default:
        kerror(ERROR_INVALID_PARAMS);
    }
    return res;
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
//    RCC->APB1ENR = 0;
    RCC->APB2ENR = 0;

#if defined(STM32F1) || defined(STM32L1) || defined(STM32L0) || defined(STM32F0)
    RCC->AHBENR = 0;
#else
    RCC->AHB1ENR = 0;
    RCC->AHB2ENR = 0;
    RCC->AHB3ENR = 0;
#endif

#if defined(STM32H7)
#if (EXT_VCORE)
    PWR->CR3 = PWR_CR3_BYPASS;
#else
    PWR->CR3 = PWR_CR3_LDOEN;
#endif// EXT_VCORE
    SCB_EnableICache();
#if (STM32_DCACHE_ENABLE)
    SCB_EnableDCache();
#endif //STM32_DCACHE_ENABLE

#else

#endif // STM32H7

    RCC->CFGR = 0;

#if defined(STM32F10X_CL) || defined(STM32F100)
    RCC->CFGR2 = 0;
#elif defined(STM32F0)
    RCC->CFGR2 = 0;
    RCC->CFGR3 = 0;
#endif

#if (POWER_MANAGEMENT) || (STM32_RTC_DRIVER)
#if !defined(STM32H7)
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;
#endif // STM32H7

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

#if defined(STM32H7)
static inline void stm32_power_set_mco(uint32_t mco_src, uint32_t mco_div)
{
    uint32_t div_pos, src_pos;
    uint32_t cfg;
    div_pos = RCC_CFGR_MCO1PRE_Pos;
    src_pos = RCC_CFGR_MCO1_Pos;
    if(mco_src > 0x10)
    {
        div_pos = RCC_CFGR_MCO2PRE_Pos;
        src_pos = RCC_CFGR_MCO2_Pos;
    }
    cfg = RCC->CFGR & ~((0x0f << div_pos) | (0x07 << src_pos) );
    cfg |= (mco_div & 0x0f) << div_pos;
    cfg |= (mco_src & 0x07) << src_pos;
    RCC->CFGR = cfg;
}

static inline void set_core_clock_internal(EXO* exo, uint32_t clock)
{
    uint32_t pllclk, div;
    if(clock == get_core_clock_internal() || clock > MAX_CORE || clock < MIN_CORE)
        return;

    pllclk = stm32_power_get_pll_clock(1, STM32_PLL_OUT_P) ;
    div = stm32_power_get_prescaller(pllclk, clock, 4);
    clock = stm32_power_get_pll_clock(1, STM32_PLL_OUT_P) / get_pre_divider(div, 4);

    stm32_power_set_buses_safe();
    RCC->D1CFGR = (RCC->D1CFGR & ~RCC_D1CFGR_D1CPRE) | (div << RCC_D1CFGR_D1CPRE_Pos);
    stm32_power_set_buses(clock, VOS_VALUE);
    __DSB();
    __DMB();
    stm32_timer_adjust(exo);
}

static inline void stm32_power_set_clock(EXO* exo, uint32_t type, uint32_t clock)
{
    switch (type)
    {
    case POWER_CORE_CLOCK:
        set_core_clock_internal(exo, clock);
        break;
    default:
        kerror(ERROR_NOT_SUPPORTED);
        break;
    }
}
#endif // STM32H7


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
#if defined(STM32H7)
    case STM32_POWER_SET_MCO:
        stm32_power_set_mco(ipc->param2, ipc->param3);
        break;
    case POWER_SET_CLOCK:
        stm32_power_set_clock(exo, ipc->param1, ipc->param3);
        break;
#endif // STM32H7
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
