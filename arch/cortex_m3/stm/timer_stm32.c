/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "timer_stm32.h"
#include "arch.h"
#include "error.h"
#include "types.h"
#include "../../../kernel/kernel.h"
#include "../../../userspace/timer.h"

#if defined(STM32F1)
#include "rcc_stm32f2.h"
#elif defined(STM32F2)
#include "rcc_stm32f2.h"
#elif defined(STM32F4)
#include "rcc_stm32f4.h"
#endif

typedef TIM_TypeDef* TIM_TypeDef_P;

const TIM_TypeDef_P _TIMER[] =                        {TIM1, TIM2, TIM3, TIM4, TIM5, TIM6, TIM7, TIM8, TIM9, TIM10, TIM11, TIM12, TIM13, TIM14};

#if defined (STM32F10X_LD)
const IRQn_Type TIMER_VECTORS[]    =                {TIM1_UP_IRQn, TIM2_IRQn, TIM3_IRQn, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#elif defined (STM32F10X_LD_VL)
//1 == 15 share
const IRQn_Type TIMER_VECTORS[]    =                {TIM1_UP_TIM16_IRQn, TIM2_IRQn, TIM3_IRQn, 0, 0, TIM6_DAC_IRQn, TIM7_IRQn, 0, 0, 0, 0, 0, 0, 0,
                                                             TIM1_BRK_TIM15_IRQn, TIM1_UP_TIM16_IRQn, TIM1_TRG_COM_TIM17_IRQn};
#elif defined (STM32F10X_MD)
const IRQn_Type TIMER_VECTORS[]    =                {TIM1_UP_IRQn, TIM2_IRQn, TIM3_IRQn, TIM4_IRQn, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

#elif defined (STM32F10X_MD_VL)
//1 == 15 share
const IRQn_Type TIMER_VECTORS[]    =                {TIM1_UP_TIM16_IRQn, TIM2_IRQn, TIM3_IRQn, TIM4_IRQn, 0, TIM6_DAC_IRQn, TIM7_IRQn,
                                                             0, 0, 0, 0, 0, 0, 0,    TIM1_BRK_TIM15_IRQn, TIM1_UP_TIM16_IRQn, TIM1_TRG_COM_TIM17_IRQn};
#elif defined (STM32F10X_HD_VL)
//1 == 15 share
const IRQn_Type TIMER_VECTORS[]    =                {TIM1_UP_TIM16_IRQn, TIM2_IRQn, TIM3_IRQn, TIM4_IRQn, TIM5_IRQn, TIM6_DAC_IRQn, TIM7_IRQn,
                                                             0, 0, 0, 0, TIM12_IRQHandler, TIM13_IRQHandler, TIM14_IRQHandler,    TIM1_BRK_TIM15_IRQn, TIM1_UP_TIM16_IRQn, TIM1_TRG_COM_TIM17_IRQn};
#elif defined (STM32F10X_CL)
const IRQn_Type TIMER_VECTORS[]    =                {TIM1_UP_IRQn, TIM2_IRQn, TIM3_IRQn, TIM4_IRQn, TIM5_IRQn, TIM6_IRQn, TIM7_IRQn, 0,
                                                             0, 0, 0, 0, 0, 0, 0, 0, 0};
#elif defined (STM32F10X_HD)
const IRQn_Type TIMER_VECTORS[]    =                {TIM1_UP_IRQn, TIM2_IRQn, TIM3_IRQn, TIM4_IRQn, TIM5_IRQn, TIM6_IRQn, TIM7_IRQn, TIM8_UP_IRQn,
                                                             0, 0, 0, 0, 0, 0, 0, 0, 0};
#elif defined (STM32F10X_XL)
//1 == 10, 8 == 13
const IRQn_Type TIMER_VECTORS[]    =                {TIM1_UP_TIM10_IRQn, TIM2_IRQn, TIM3_IRQn, TIM4_IRQn, TIM5_IRQn, TIM6_IRQn, TIM7_IRQn,
                                                             TIM8_UP_TIM13_IRQn, TIM1_BRK_TIM9_IRQn, TIM1_UP_TIM10_IRQn,    TIM1_TRG_COM_TIM11_IRQn,
                                                             TIM8_BRK_TIM12_IRQn, TIM8_UP_TIM13_IRQn, TIM8_TRG_COM_TIM14_IRQn, 0, 0, 0};
#elif defined(STM32F2)
//1 == 10, 8 == 13
const IRQn_Type TIMER_VECTORS[] =                {TIM1_UP_TIM10_IRQn, TIM2_IRQn, TIM3_IRQn, TIM4_IRQn, TIM5_IRQn, TIM6_DAC_IRQn, TIM7_IRQn, TIM8_UP_TIM13_IRQn,
                                                             TIM1_BRK_TIM9_IRQn, TIM1_UP_TIM10_IRQn, TIM1_TRG_COM_TIM11_IRQn, TIM8_BRK_TIM12_IRQn, TIM8_UP_TIM13_IRQn,
                                                             TIM8_TRG_COM_TIM14_IRQn};
#endif

#ifndef RCC_APB1ENR_TIM4EN
#define RCC_APB1ENR_TIM4EN        0
#endif

#ifndef RCC_APB1ENR_TIM5EN
#define RCC_APB1ENR_TIM5EN        0
#endif

#ifndef RCC_APB1ENR_TIM6EN
#define RCC_APB1ENR_TIM6EN        0
#endif

#ifndef RCC_APB1ENR_TIM7EN
#define RCC_APB1ENR_TIM7EN        0
#endif

#ifndef RCC_APB2ENR_TIM8EN
#define RCC_APB2ENR_TIM8EN        0
#endif

#ifndef RCC_APB2ENR_TIM9EN
#define RCC_APB2ENR_TIM9EN        0
#endif

#ifndef RCC_APB2ENR_TIM10EN
#define RCC_APB2ENR_TIM10EN    0
#endif

#ifndef RCC_APB2ENR_TIM11EN
#define RCC_APB2ENR_TIM11EN    0
#endif

#ifndef RCC_APB1ENR_TIM12EN
#define RCC_APB1ENR_TIM12EN    0
#endif

#ifndef RCC_APB1ENR_TIM13EN
#define RCC_APB1ENR_TIM13EN    0
#endif

#ifndef RCC_APB1ENR_TIM14EN
#define RCC_APB1ENR_TIM14EN    0
#endif

#ifndef RCC_APB2ENR_TIM15EN
#define RCC_APB2ENR_TIM15EN    0
#endif

#ifndef RCC_APB2ENR_TIM16EN
#define RCC_APB2ENR_TIM16EN    0
#endif

#ifndef RCC_APB2ENR_TIM17EN
#define RCC_APB2ENR_TIM17EN    0
#endif

const uint32_t     RCC_TIMER[] =                        {RCC_APB2ENR_TIM1EN, RCC_APB1ENR_TIM2EN, RCC_APB1ENR_TIM3EN, RCC_APB1ENR_TIM4EN, RCC_APB1ENR_TIM5EN, RCC_APB1ENR_TIM6EN, RCC_APB1ENR_TIM7EN,
                                                            RCC_APB2ENR_TIM8EN, RCC_APB2ENR_TIM9EN, RCC_APB2ENR_TIM10EN, RCC_APB2ENR_TIM11EN, RCC_APB1ENR_TIM12EN, RCC_APB1ENR_TIM13EN, RCC_APB1ENR_TIM14EN,
                                                            RCC_APB2ENR_TIM15EN, RCC_APB2ENR_TIM16EN, RCC_APB2ENR_TIM17EN};

#define SYS_TIMER_RTC                            RTC_0
#define SYS_TIMER_HPET                            TIM_4
#define SYS_TIMER_PRIORITY                        10
#define SYS_TIMER_SOFT_RTC                        1
#define SYS_TIMER_SOFT_RTC_TIMER                TIM_7

#if defined(STM32F10X_LD_VL) || defined(STM32F10X_MD_VL) || defined(STM32F10X_HD_VL)
void TIM1_UP_TIM16_IRQHandler(void)
{
    if (TIM1->SR & TIM_SR_UIF)
    {
        _TIMER[TIM_1]->SR &= ~TIM_SR_UIF;
        if (__KERNEL->timer_handlers[TIM_1])
            __KERNEL->timer_handlers[TIM_1](TIM_1);
    }
    else
    {
        _TIMER[TIM_16]->SR &= ~TIM_SR_UIF;
        if (__KERNEL->timer_handlers[TIM_16])
            __KERNEL->timer_handlers[TIM_16](TIM_16);
    }
}
#elif defined(STM32F10X_LD) || defined(STM32F10X_MD) || defined(STM32F10X_HD) || defined(STM32F10X_CL)
void TIM1_UP_IRQHandler(void)
{
    _TIMER[TIM_1]->SR &= ~TIM_SR_UIF;
    if (__KERNEL->timer_handlers[TIM_1])
        __KERNEL->timer_handlers[TIM_1](TIM_1);
}
#else
void TIM1_UP_TIM10_IRQHandler(void)
{
    if (TIM1->SR & TIM_SR_UIF)
    {
        _TIMER[TIM_1]->SR &= ~TIM_SR_UIF;
        if (__KERNEL->timer_handlers[TIM_1])
            __KERNEL->timer_handlers[TIM_1](TIM_1);
    }
    else
    {
        _TIMER[TIM_10]->SR &= ~TIM_SR_UIF;
        if (__KERNEL->timer_handlers[TIM_10])
            __KERNEL->timer_handlers[TIM_10](TIM_10);
    }
}
#endif

void TIM2_IRQHandler(void)
{
    _TIMER[TIM_2]->SR &= ~TIM_SR_UIF;
    if (__KERNEL->timer_handlers[TIM_2])
        __KERNEL->timer_handlers[TIM_2](TIM_2);
}

void TIM3_IRQHandler(void)
{
    _TIMER[TIM_3]->SR &= ~TIM_SR_UIF;
    if (__KERNEL->timer_handlers[TIM_3])
        __KERNEL->timer_handlers[TIM_3](TIM_3);
}

#if !defined(STM32F10X_LD) && !defined(STM32F10X_LD_VL)

void TIM4_IRQHandler(void)
{
    _TIMER[TIM_4]->SR &= ~TIM_SR_UIF;
    if (__KERNEL->timer_handlers[TIM_4])
        __KERNEL->timer_handlers[TIM_4](TIM_4);
}

#if !defined(STM32F10X_MD) && !defined(STM32F10X_MD_VL)
void TIM5_IRQHandler(void)
{
    _TIMER[TIM_5]->SR &= ~TIM_SR_UIF;
    if (__KERNEL->timer_handlers[TIM_5])
        __KERNEL->timer_handlers[TIM_5](TIM_5);
}
#endif //!defined(STM32F10X_MD) && !defined(STM32F10X_MD_VL)
#endif //!defined(STM32F10X_LD) && !defined(STM32F10X_LD_VL)


#if !defined(STM32F10X_LD) && !defined(STM32F10X_MD)

#if defined(STM32F10X_HD) || defined(STM32F10X_XL) || defined(STM32F10X_CL)
void TIM6_IRQHandler(void)
#else
void TIM6_DAC_IRQHandler(void)
#endif
{
    _TIMER[TIM_6]->SR &= ~TIM_SR_UIF;
    if (__KERNEL->timer_handlers[TIM_6])
        __KERNEL->timer_handlers[TIM_6](TIM_6);
}

#endif //!defined(STM32F10X_LD) && !defined(STM32F10X_MD)

void TIM7_IRQHandler(void)
{
    _TIMER[TIM_7]->SR &= ~TIM_SR_UIF;
    if (__KERNEL->timer_handlers[TIM_7])
        __KERNEL->timer_handlers[TIM_7](TIM_7);
}

#if defined(STM32F10X_HD)
void TIM8_UP_IRQHandler(void)
{
    _TIMER[TIM_8]->SR &= ~TIM_SR_UIF;
    if (__KERNEL->timer_handlers[TIM_8])
        __KERNEL->timer_handlers[TIM_8](TIM_8);
}
#elif defined(STM32F10X_XL) || defined(STM32F2)
void TIM8_UP_TIM13_IRQHandler(void)
{
    if (TIM8->SR & TIM_SR_UIF)
    {
        _TIMER[TIM_8]->SR &= ~TIM_SR_UIF;
        if (__KERNEL->timer_handlers[TIM_8])
            __KERNEL->timer_handlers[TIM_8](TIM_8);
    }
    else
    {
        _TIMER[TIM_13]->SR &= ~TIM_SR_UIF;
        if (__KERNEL->timer_handlers[TIM_13])
            __KERNEL->timer_handlers[TIM_13](TIM_13);
    }
}

void TIM1_BRK_TIM9_IRQHandler(void)
{
    _TIMER[TIM_9]->SR &= ~TIM_SR_UIF;
    if (__KERNEL->timer_handlers[TIM_9])
        __KERNEL->timer_handlers[TIM_9](TIM_9);
}

void TIM1_TRG_COM_TIM11_IRQHandler(void)
{
    _TIMER[TIM_11]->SR &= ~TIM_SR_UIF;
    if (__KERNEL->timer_handlers[TIM_11])
        __KERNEL->timer_handlers[TIM_11](TIM_11);
}


void TIM8_BRK_TIM12_IRQHandler(void)
{
    _TIMER[TIM_12]->SR &= ~TIM_SR_UIF;
    if (__KERNEL->timer_handlers[TIM_12])
        __KERNEL->timer_handlers[TIM_12](TIM_12);
}

void TIM8_TRG_COM_TIM14_IRQHandler(void)
{
    _TIMER[TIM_14]->SR &= ~TIM_SR_UIF;
    if (__KERNEL->timer_handlers[TIM_14])
        __KERNEL->timer_handlers[TIM_14](TIM_14);
}

#endif


#if defined(STM32F10X_HD_VL)
void TIM12_IRQHandler(void)
{
    _TIMER[TIM_12]->SR &= ~TIM_SR_UIF;
    if (__KERNEL->timer_handlers[TIM_12])
        __KERNEL->timer_handlers[TIM_12](TIM_12);
}

void TIM13_IRQHandler(void)
{
    _TIMER[TIM_13]->SR &= ~TIM_SR_UIF;
    if (__KERNEL->timer_handlers[TIM_13])
        __KERNEL->timer_handlers[TIM_13](TIM_14);
}

void TIM14_IRQHandler(void)
{
    _TIMER[TIM_14]->SR &= ~TIM_SR_UIF;
    if (__KERNEL->timer_handlers[TIM_14])
        __KERNEL->timer_handlers[TIM_14](TIM_14);
}
#endif

#if defined(STM32F10X_LD_VL) || defined(STM32F10X_MD_VL) || defined(STM32F10X_HD_VL)
void TIM1_BRK_TIM15_IRQHandler(void)
{
    _TIMER[TIM_15]->SR &= ~TIM_SR_UIF;
    if (__KERNEL->timer_handlers[TIM_15])
        __KERNEL->timer_handlers[TIM_15](TIM_15);
}

void TIM1_TRG_COM_TIM17_IRQHandler(void)
{
    _TIMER[TIM_17]->SR &= ~TIM_SR_UIF;
    if (__KERNEL->timer_handlers[TIM_17])
        __KERNEL->timer_handlers[TIM_17](TIM_17);
}
#endif

void timer_enable(TIMER_CLASS timer, UTIMER_HANDLER handler, int priority, unsigned int flags)
{
    if ((1 << timer) & TIMERS_MASK)
    {
        __KERNEL->timer_handlers[timer] = handler;
        //power up
        switch (timer)
        {
        case TIM_1:
        case TIM_8:
        case TIM_9:
        case TIM_10:
        case TIM_11:
            RCC->APB2ENR |= RCC_TIMER[timer];
            break;
        default:
            RCC->APB1ENR |= RCC_TIMER[timer];
        }

        //one-pulse mode
        _TIMER[timer]->CR1 |= TIM_CR1_URS;
        if (flags & TIMER_FLAG_ONE_PULSE_MODE)
            _TIMER[timer]->CR1 |= TIM_CR1_OPM;
        _TIMER[timer]->DIER |= TIM_DIER_UIE;

        //enable IRQ
        if (timer == TIM_1 || timer == TIM_10)
        {
            if (__KERNEL->shared1++ == 0)
                NVIC_EnableIRQ(TIMER_VECTORS[timer]);
        }
        else if (timer == TIM_8 || timer == TIM_13)
        {
            if (__KERNEL->shared8++ == 0)
                NVIC_EnableIRQ(TIMER_VECTORS[timer]);
        }
        else
            NVIC_EnableIRQ(TIMER_VECTORS[timer]);
        NVIC_SetPriority(TIMER_VECTORS[timer], priority);
    }
    else
        error(ERROR_NOT_SUPPORTED);
}

void timer_disable(TIMER_CLASS timer)
{
    if ((1 << timer) & TIMERS_MASK)
    {
        _TIMER[timer]->CR1 &= ~TIM_CR1_CEN;
        //disable IRQ
        if (timer == TIM_1 || timer == TIM_10)
        {
            if (--__KERNEL->shared1== 0)
                NVIC_DisableIRQ(TIMER_VECTORS[timer]);
        }
        else if (timer == TIM_8 || timer == TIM_13)
        {
            if (--__KERNEL->shared8 == 0)
                NVIC_DisableIRQ(TIMER_VECTORS[timer]);
        }
        else
            NVIC_DisableIRQ(TIMER_VECTORS[timer]);

        //power down
        switch (timer)
        {
        case TIM_1:
        case TIM_8:
        case TIM_9:
        case TIM_10:
        case TIM_11:
        case TIM_15:
        case TIM_16:
        case TIM_17:
            RCC->APB2ENR &= ~RCC_TIMER[timer];
            break;
        default:
            RCC->APB1ENR &= ~RCC_TIMER[timer];
        }
        __KERNEL->timer_handlers[timer] = NULL;
    }
    else
        error(ERROR_NOT_SUPPORTED);
}

void timer_start(TIMER_CLASS timer, unsigned int time_us)
{
    uint32_t timer_freq;
    switch (timer)
    {
    case TIM_1:
    case TIM_8:
    case TIM_9:
    case TIM_10:
    case TIM_11:
    case TIM_15:
    case TIM_16:
    case TIM_17:
        timer_freq = __KERNEL->apb2_freq;
        break;
    default:
        timer_freq = __KERNEL->apb1_freq;
    }
    if (timer_freq != __KERNEL->ahb_freq)
        timer_freq = timer_freq * 2;

    //find prescaller
    unsigned int psc = 1; //1us
    unsigned int count = time_us;
    while (count > 0xffff)
    {
        psc <<= 1;
        count >>= 1;
    }
    psc *= timer_freq / 1000000; //1us

    _TIMER[timer]->PSC = psc - 1;
    _TIMER[timer]->ARR = count - 1;
    _TIMER[timer]->CNT = 0;

    _TIMER[timer]->EGR = TIM_EGR_UG;
    _TIMER[timer]->CR1 |= TIM_CR1_CEN;
}

void timer_stop(TIMER_CLASS timer)
{
    _TIMER[timer]->CR1 &= ~TIM_CR1_CEN;
    _TIMER[timer]->SR &= ~TIM_SR_UIF;
}

unsigned int timer_elapsed(TIMER_CLASS timer)
{
    uint32_t timer_freq;
    switch (timer)
    {
    case TIM_1:
    case TIM_8:
    case TIM_9:
    case TIM_10:
    case TIM_11:
    case TIM_15:
    case TIM_16:
    case TIM_17:
        timer_freq = __KERNEL->apb2_freq;
        break;
    default:
        timer_freq = __KERNEL->apb1_freq;
    }
    if (timer_freq !=__KERNEL->ahb_freq)
        timer_freq = timer_freq * 2;
    return (((_TIMER[timer]->PSC) + 1)/ (timer_freq / 1000000)) * ((_TIMER[timer]->CNT) + 1);
}

void second_pulse_isr(TIMER_CLASS timer)
{
    if (__KERNEL->killme == 0)
        timer_second_pulse();
}

void hpet_isr(TIMER_CLASS timer)
{
    if (__KERNEL->killme == 0)
        timer_hpet_timeout();
}

void hpet_start(unsigned int value)
{
    timer_start(SYS_TIMER_HPET, value);
}

void hpet_stop()
{
    timer_stop(SYS_TIMER_HPET);
}

unsigned int hpet_elapsed()
{
    return timer_elapsed(SYS_TIMER_HPET);
}

void timer_init_hw()
{
    __KERNEL->killme = 1;
    timer_enable(SYS_TIMER_HPET, hpet_isr, SYS_TIMER_PRIORITY, TIMER_FLAG_ONE_PULSE_MODE);
    CB_SVC_TIMER cb_svc_timer;
    cb_svc_timer.start = hpet_start;
    cb_svc_timer.stop = hpet_stop;
    cb_svc_timer.elapsed = hpet_elapsed;
    timer_setup(&cb_svc_timer);
#if (SYS_TIMER_SOFT_RTC)
    timer_enable(SYS_TIMER_SOFT_RTC, second_pulse_isr, SYS_TIMER_PRIORITY, 0);
    timer_start(SYS_TIMER_SOFT_RTC, 1000000);
#else
    rtc_enable_second_tick(SYS_TIMER_RTC, timer_second_pulse, SYS_TIMER_PRIORITY);
#endif //SYS_TIMER_SOFT_RTC
    __KERNEL->killme = 0;
}
