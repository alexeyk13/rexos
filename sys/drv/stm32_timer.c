/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "stm32_timer.h"
#include "error.h"
#include "types.h"
#include "../../kernel/kernel.h"
#include "../../userspace/timer.h"
#include "../../userspace/irq.h"

typedef TIM_TypeDef*                            TIM_TypeDef_P;

#define APB1                                    (unsigned int*)((unsigned int)RCC_BASE + offsetof(RCC_TypeDef, APB1ENR))
#define APB2                                    (unsigned int*)((unsigned int)RCC_BASE + offsetof(RCC_TypeDef, APB2ENR))

typedef unsigned int*                           uint_p;

#if defined (STM32F1)
#define TIMERS_COUNT                            17
const TIM_TypeDef_P TIMER_REGS[TIMERS_COUNT] =  {TIM1, TIM2, TIM3, TIM4, TIM5, TIM6, TIM7, TIM8, TIM9, TIM10, TIM11, TIM12, TIM13, TIM14, TIM15, TIM16, TIM17};
const int TIMER_VECTORS[TIMERS_COUNT] =         {25,   28,   29,   30,   50,   54,   55,   44,   24,   25,    26,    43,    44,    45,    24,    25,    26};
const int TIMER_POWER_BIT[TIMERS_COUNT] =                 {11,   0,    1,    2,    3,    4,    5,    13,   19,   20,    21,    6,     7,     8,     16,    17,    18};
const uint_p TIMER_POWER_PORT[TIMERS_COUNT] =   {APB2, APB1, APB1, APB1, APB1, APB1, APB1, APB2, APB2, APB2,  APB2,  APB1,  APB1,  APB1,  APB2,  APB2,  APB2};
#elif defined (STM32F2) || defined (STM32F4)
#define TIMERS_COUNT                            14
const TIM_TypeDef_P TIMER_REGS[TIMERS_COUNT] =  {TIM1, TIM2, TIM3, TIM4, TIM5, TIM6, TIM7, TIM8, TIM9, TIM10, TIM11, TIM12, TIM13, TIM14};
const int TIMER_VECTORS[TIMERS_COUNT] =         {25,   28,   29,   30,   50,   54,   55,   44,   24,   25,    26,    43,    44,    45};
const int TIMER_POWER_BIT[TIMERS_COUNT] =       {0,    0,    1,    2,    3,    4,    5,    1,    16,   17,    18,    6,     7,     8};
const uint_p TIMER_POWER_PORT[TIMERS_COUNT] =   {APB2, APB1, APB1, APB1, APB1, APB1, APB1, APB2, APB2, APB2,  APB2,  APB1,  APB1,  APB1};
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


#define SYS_TIMER_RTC                            RTC_0
#define SYS_TIMER_HPET                            TIM_4
#define SYS_TIMER_PRIORITY                        10
#define SYS_TIMER_SOFT_RTC                        1

void timer_enable(TIMERS timer, int priority, unsigned int flags)
{
//    if ((1 << timer) & TIMERS_MASK)
    {
        //power up
        *(TIMER_POWER_PORT[timer]) |= 1 << TIMER_POWER_BIT[timer];
        //one-pulse mode
        TIMER_REGS[timer]->CR1 |= TIM_CR1_URS;
        if (flags & TIMER_FLAG_ONE_PULSE_MODE)
            TIMER_REGS[timer]->CR1 |= TIM_CR1_OPM;
        TIMER_REGS[timer]->DIER |= TIM_DIER_UIE;

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
        NVIC_SetPriority(TIMER_VECTORS[timer], 15);
    }
//    else
//        error(ERROR_NOT_SUPPORTED);
}

void timer_disable(TIMERS timer)
{
    //    if ((1 << timer) & TIMERS_MASK)
    {
        TIMER_REGS[timer]->CR1 &= ~TIM_CR1_CEN;
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
        *(TIMER_POWER_PORT[timer]) &= ~(1 << TIMER_POWER_BIT[timer]);
    }
//    else
//        error(ERROR_NOT_SUPPORTED);
}

void timer_start(TIMERS timer, unsigned int time_us)
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

    TIMER_REGS[timer]->PSC = psc - 1;
    TIMER_REGS[timer]->ARR = count - 1;
    TIMER_REGS[timer]->CNT = 0;

    TIMER_REGS[timer]->EGR = TIM_EGR_UG;
    TIMER_REGS[timer]->CR1 |= TIM_CR1_CEN;
}

void timer_stop(TIMERS timer)
{
    TIMER_REGS[timer]->CR1 &= ~TIM_CR1_CEN;
    TIMER_REGS[timer]->SR &= ~TIM_SR_UIF;
}

unsigned int timer_elapsed(TIMERS timer)
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
    return (((TIMER_REGS[timer]->PSC) + 1)/ (timer_freq / 1000000)) * ((TIMER_REGS[timer]->CNT) + 1);
}

void second_pulse_isr(int vector, void* param)
{
    TIM2->SR &= ~TIM_SR_UIF;
    timer_second_pulse();
}

void hpet_isr(int vector, void* param)
{
    TIM4->SR &= ~TIM_SR_UIF;
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
    irq_register(TIMER_VECTORS[TIM_2], second_pulse_isr, NULL);
    irq_register(TIMER_VECTORS[TIM_4], hpet_isr, NULL);

    timer_enable(SYS_TIMER_HPET, SYS_TIMER_PRIORITY, TIMER_FLAG_ONE_PULSE_MODE);
    CB_SVC_TIMER cb_svc_timer;
    cb_svc_timer.start = hpet_start;
    cb_svc_timer.stop = hpet_stop;
    cb_svc_timer.elapsed = hpet_elapsed;
    timer_setup(&cb_svc_timer);
#if (SYS_TIMER_SOFT_RTC)
    timer_enable(SYS_TIMER_SOFT_RTC, SYS_TIMER_PRIORITY, 0);
    timer_start(SYS_TIMER_SOFT_RTC, 1000000);
#else
    rtc_enable_second_tick(SYS_TIMER_RTC, timer_second_pulse, SYS_TIMER_PRIORITY);
#endif //SYS_TIMER_SOFT_RTC
}
