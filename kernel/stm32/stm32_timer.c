/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "stm32_timer.h"
#include "sys_config.h"
#include "../../userspace/stm32/stm32_driver.h"
#include "stm32_power.h"
#include "stm32_exo_private.h"
#include "../kerror.h"
#include "../ksystime.h"
#include "../kirq.h"
#include "../../userspace/power.h"
#include <string.h>

#define APB1                                    (unsigned int*)((unsigned int)RCC_BASE + offsetof(RCC_TypeDef, APB1ENR))
#define APB2                                    (unsigned int*)((unsigned int)RCC_BASE + offsetof(RCC_TypeDef, APB2ENR))

typedef unsigned int*                           uint_p;
typedef TIM_TypeDef*                            TIM_TypeDef_P;

#if defined (STM32F1)
#define TIMERS_COUNT                            17
const TIM_TypeDef_P TIMER_REGS[TIMERS_COUNT] =  {TIM1, TIM2, TIM3, TIM4, TIM5, TIM6, TIM7, TIM8, TIM9, TIM10, TIM11, TIM12, TIM13, TIM14, TIM15, TIM16, TIM17};
const int TIMER_VECTORS[TIMERS_COUNT] =         {25,   28,   29,   30,   50,   54,   55,   44,   24,   25,    26,    43,    44,    45,    24,    25,    26};
const int TIMER_POWER_BIT[TIMERS_COUNT] =       {11,   0,    1,    2,    3,    4,    5,    13,   19,   20,    21,    6,     7,     8,     16,    17,    18};
const uint_p TIMER_POWER_PORT[TIMERS_COUNT] =   {APB2, APB1, APB1, APB1, APB1, APB1, APB1, APB2, APB2, APB2,  APB2,  APB1,  APB1,  APB1,  APB2,  APB2,  APB2};

#elif defined (STM32F2) || defined (STM32F4)
#define TIMERS_COUNT                            14
const TIM_TypeDef_P TIMER_REGS[TIMERS_COUNT] =  {TIM1, TIM2, TIM3, TIM4, TIM5, TIM6, TIM7, TIM8, TIM9, TIM10, TIM11, TIM12, TIM13, TIM14};
const int TIMER_VECTORS[TIMERS_COUNT] =         {25,   28,   29,   30,   50,   54,   55,   44,   24,   25,    26,    43,    44,    45};
const int TIMER_POWER_BIT[TIMERS_COUNT] =       {0,    0,    1,    2,    3,    4,    5,    1,    16,   17,    18,    6,     7,     8};
const uint_p TIMER_POWER_PORT[TIMERS_COUNT] =   {APB2, APB1, APB1, APB1, APB1, APB1, APB1, APB2, APB2, APB2,  APB2,  APB1,  APB1,  APB1};
#elif defined(STM32L0)
#define TIMERS_COUNT                            4
const TIM_TypeDef_P TIMER_REGS[TIMERS_COUNT] =  {TIM2, TIM6, TIM21, TIM22};
const int TIMER_VECTORS[TIMERS_COUNT] =         {15,   17,   20,    22};
const int TIMER_POWER_BIT[TIMERS_COUNT] =       {0,    4,    2,     5};
const uint_p TIMER_POWER_PORT[TIMERS_COUNT] =   {APB1, APB1, APB2,  APB2};
#elif defined(STM32L1)
#define TIMERS_COUNT                            9
const TIM_TypeDef_P TIMER_REGS[TIMERS_COUNT] =  {TIM2, TIM3, TIM4, TIM5, TIM6, TIM7, TIM9, TIM10, TIM11};
const int TIMER_VECTORS[TIMERS_COUNT] =         {28,   29,   30,   46,   43,   44,   25,   26,    27};
const int TIMER_POWER_BIT[TIMERS_COUNT] =       {0,    1,    2,    3,    4,    5,    2,    3,     4};
const uint_p TIMER_POWER_PORT[TIMERS_COUNT] =   {APB1, APB1, APB1, APB1, APB1, APB1, APB2, APB2,  APB2};

#elif defined (STM32F0)
#define TIMERS_COUNT                            TIM_MAX

//ALL have TIM1, TIM3
#ifndef TIM2
#define TIM2                                    0
#endif
#ifndef TIM6
#define TIM6                                    0
#endif
#ifndef TIM7
#define TIM7                                    0
#endif
#ifndef TIM14
#define TIM14                                   0
#endif
#ifndef TIM15
#define TIM15                                   0
#endif
#ifndef TIM16
#define TIM16                                   0
#endif
#ifndef TIM17
#define TIM17                                   0
#endif

const TIM_TypeDef_P TIMER_REGS[TIMERS_COUNT] =  {TIM1, TIM2, TIM3, TIM6, TIM7, TIM14, TIM15, TIM16, TIM17};
const int TIMER_VECTORS[TIMERS_COUNT] =         {13,   15,   16,     17,   18,    19,    20,    21,    22};
const int TIMER_POWER_BIT[TIMERS_COUNT] =       {11,   0,    1,       4,    5,     8,    16,    17,    18};
const uint_p TIMER_POWER_PORT[TIMERS_COUNT] =   {APB2, APB1, APB1, APB1, APB1,  APB1,  APB2,  APB2,  APB2};
#endif

void stm32_timer_open(EXO* exo, TIMER_NUM num, unsigned int flags)
{
    //power up
    *(TIMER_POWER_PORT[num]) |= 1 << TIMER_POWER_BIT[num];
    TIMER_REGS[num]->CR1 |= TIM_CR1_URS;
    //one-pulse mode
    if (flags & TIMER_ONE_PULSE)
        TIMER_REGS[num]->CR1 |= TIM_CR1_OPM;
    if (flags & TIMER_IRQ_ENABLE)
        TIMER_REGS[num]->DIER |= TIM_DIER_UIE;
    if (flags & STM32_TIMER_DMA_ENABLE)
        TIMER_REGS[num]->DIER |= TIM_DIER_UDE;
    if (flags & TIMER_EXT_CLOCK)
        TIMER_REGS[num]->SMCR |= 1 << 14;

#if defined(STM32F1) || defined(STM32F2) || defined(STM32F4)
    if (num == TIM_1 || num == TIM_10)
    {
        if (exo->timer.shared1++ == 0)
        {
            if (flags & TIMER_IRQ_ENABLE)
            {
                NVIC_EnableIRQ(TIMER_VECTORS[num]);
                NVIC_SetPriority(TIMER_VECTORS[num], TIMER_IRQ_PRIORITY_VALUE(flags));
            }
        }
    }
    else if (num == TIM_8 || num == TIM_13)
    {
        if (exo->timer.shared8++ == 0)
        {
            if (flags & TIMER_IRQ_ENABLE)
            {
                NVIC_EnableIRQ(TIMER_VECTORS[num]);
                NVIC_SetPriority(TIMER_VECTORS[num], TIMER_IRQ_PRIORITY_VALUE(flags));
            }
        }
    }
    else
#endif
        if (flags & TIMER_IRQ_ENABLE)
        {
            NVIC_EnableIRQ(TIMER_VECTORS[num]);
            NVIC_SetPriority(TIMER_VECTORS[num], TIMER_IRQ_PRIORITY_VALUE(flags));
        }
}

void stm32_timer_close(EXO* exo, TIMER_NUM num)
{
    //disable timer
    TIMER_REGS[num]->CR1 &= ~TIM_CR1_CEN;

    //disable IRQ
#if defined(STM32F1) || defined(STM32F2) || defined(STM32F4)
    if (num == TIM_1 || num == TIM_10)
    {
        if (--exo->timer.shared1== 0)
            NVIC_DisableIRQ(TIMER_VECTORS[num]);
    }
    else if (num == TIM_8 || num == TIM_13)
    {
        if (--exo->timer.shared8 == 0)
            NVIC_DisableIRQ(TIMER_VECTORS[num]);
    }
    else
#endif
        NVIC_DisableIRQ(TIMER_VECTORS[num]);

    //power down
    *(TIMER_POWER_PORT[num]) &= ~(1 << TIMER_POWER_BIT[num]);
}

static unsigned int stm32_timer_get_clock(EXO* exo, TIMER_NUM num)
{
    unsigned int ahb, apb;
    ahb = stm32_power_get_clock_inside(exo, POWER_BUS_CLOCK);
    if (TIMER_POWER_PORT[num] == APB2)
        apb = stm32_power_get_clock_inside(exo, STM32_CLOCK_APB2);
    else
        apb = stm32_power_get_clock_inside(exo, STM32_CLOCK_APB1);
    if (ahb != apb)
        apb <<= 1;
    return apb;
}

static void stm32_timer_setup_clk(TIMER_NUM num, unsigned int clk)
{
    unsigned int psc, cnt;
    psc = clk / 0x8000;
    if (!psc)
        psc = 1;
    cnt = clk / psc;
    if (cnt < 2)
        cnt = 2;
    if (cnt > 0x10000)
        cnt = 0x10000;

    TIMER_REGS[num]->PSC = psc - 1;
    TIMER_REGS[num]->ARR = cnt - 1;
}

static inline void stm32_timer_setup_hz(EXO* exo, TIMER_NUM num, unsigned int hz)
{
    unsigned int clk;
    clk = stm32_timer_get_clock(exo, num) / hz;
    stm32_timer_setup_clk(num, clk);
}

static inline void stm32_timer_setup_us(EXO* exo, TIMER_NUM num, unsigned int us)
{
    unsigned int clk;
    clk = stm32_timer_get_clock(exo, num) / 1000000 * us;
    stm32_timer_setup_clk(num, clk);
}

void stm32_timer_start(EXO* exo, TIMER_NUM num, TIMER_VALUE_TYPE value_type, unsigned int value)
{
    switch (value_type)
    {
    case TIMER_VALUE_HZ:
        stm32_timer_setup_hz(exo, num, value);
        break;
    case TIMER_VALUE_US:
        stm32_timer_setup_us(exo, num, value);
        break;
    default:
        stm32_timer_setup_clk(num, value);
    }

    TIMER_REGS[num]->CNT = 0;
    TIMER_REGS[num]->EGR = TIM_EGR_UG;
    TIMER_REGS[num]->CR1 |= TIM_CR1_CEN;
}

void stm32_timer_stop(TIMER_NUM num)
{
    TIMER_REGS[num]->CR1 &= ~TIM_CR1_CEN;
    TIMER_REGS[num]->SR &= ~TIM_SR_UIF;
}

#if (TIMER_IO)
static inline void stm32_timer_setup_channel(int num, int channel, TIMER_CHANNEL_TYPE type, unsigned int value)
{
    uint16_t ccmr, ccer;
    //disable capture/compare
    TIMER_REGS[num]->CCER &= ~(0xf << (channel * 4));
    if (channel < TIM_CHANNEL3)
        TIMER_REGS[num]->CCMR1 &= ~(0xff << (channel * 8));
    else
        TIMER_REGS[num]->CCMR2 &= ~(0xff << ((channel - 2) * 8));
    if (type != TIMER_CHANNEL_GENERAL)
    {
        ccmr = 0;
        ccer = 0;
        switch (type)
        {
        case TIMER_CHANNEL_INPUT_RISING:
            ccmr = (1 << 0);
            break;
        case TIMER_CHANNEL_INPUT_FALLING:
            ccmr = (1 << 0);
            ccer = (1 << 1);
            break;
        case TIMER_CHANNEL_INPUT_RISING_FALLING:
            ccmr = (1 << 0);
            ccer = (1 << 1) | (1 << 3);
            break;
        case TIMER_CHANNEL_PWM:
            ccmr = (6 << 4) | (1 << 3);
            break;
        default:
            break;
        }

        switch (channel) {
        case TIM_CHANNEL1:
            TIMER_REGS[num]->CCR1 = value;
            break;
        case TIM_CHANNEL2:
            TIMER_REGS[num]->CCR2 = value;
            break;
        case TIM_CHANNEL3:
            TIMER_REGS[num]->CCR3 = value;
            break;
        case TIM_CHANNEL4:
            TIMER_REGS[num]->CCR4 = value;
            break;
        default:
            break;
        }
        if (channel < TIM_CHANNEL3)
            TIMER_REGS[num]->CCMR1 |= ccmr << (channel * 8);
        else
            TIMER_REGS[num]->CCMR2 |= ccmr << ((channel - 2) * 8);
        TIMER_REGS[num]->CCER |= (ccer | (1 << 0)) << (channel * 4);
    }
}
#endif //TIMER_IO

void hpet_isr(int vector, void* param)
{
    TIMER_REGS[HPET_TIMER]->SR &= ~TIM_SR_UIF;
    ksystime_hpet_timeout();
}

void hpet_start(unsigned int value, void* param)
{
    EXO* exo = (EXO*)param;
    //find near prescaller
    stm32_timer_start(exo, HPET_TIMER, TIMER_VALUE_CLK, value * exo->timer.hpet_uspsc);
}

void hpet_stop(void* param)
{
    stm32_timer_stop(HPET_TIMER);
}

unsigned int hpet_elapsed(void* param)
{
    EXO* exo = (EXO*)param;
    return (((TIMER_REGS[HPET_TIMER]->PSC) + 1)/ exo->timer.hpet_uspsc) * ((TIMER_REGS[HPET_TIMER]->CNT) + 1);
}

#if !(STM32_RTC_DRIVER)
void second_pulse_isr(int vector, void* param)
{
    TIMER_REGS[SECOND_PULSE_TIMER]->SR &= ~TIM_SR_UIF;
    ksystime_second_pulse();
}
#endif //STM32_RTC_DRIVER

#if (POWER_MANAGEMENT)
void stm32_timer_pm_event(EXO* exo)
{
    exo->timer.hpet_uspsc = stm32_timer_get_clock(exo, HPET_TIMER) / 1000000;
#if !(STM32_RTC_DRIVER)
    stm32_timer_stop(SECOND_PULSE_TIMER);
    stm32_timer_start(exo, SECOND_PULSE_TIMER, TIMER_VALUE_HZ, 1);
#endif //STM32_RTC_DRIVER
}
#endif //POWER_MANAGEMENT

void stm32_timer_init(EXO* exo)
{
#if defined(STM32F1) || defined(STM32F2) || defined(STM32F4)
    exo->timer.shared1 = exo->timer.shared8 = 0;
#endif

    //setup HPET
    kirq_register(KERNEL_HANDLE, TIMER_VECTORS[HPET_TIMER], hpet_isr, (void*)exo);
    exo->timer.hpet_uspsc = stm32_timer_get_clock(exo, HPET_TIMER) / 1000000;
    stm32_timer_open(exo, HPET_TIMER, TIMER_ONE_PULSE | TIMER_IRQ_ENABLE | (13 << TIMER_IRQ_PRIORITY_POS));
    CB_SVC_TIMER cb_svc_timer;
    cb_svc_timer.start = hpet_start;
    cb_svc_timer.stop = hpet_stop;
    cb_svc_timer.elapsed = hpet_elapsed;
    ksystime_hpet_setup(&cb_svc_timer, exo);
#if !(STM32_RTC_DRIVER)
    kirq_register(KERNEL_HANDLE, TIMER_VECTORS[SECOND_PULSE_TIMER], second_pulse_isr, (void*)exo);
    stm32_timer_open(exo, SECOND_PULSE_TIMER, TIMER_IRQ_ENABLE | (13 << TIMER_IRQ_PRIORITY_POS));
    stm32_timer_start(exo, SECOND_PULSE_TIMER, TIMER_VALUE_HZ, 1);
#endif //STM32_RTC_DRIVER
}

void stm32_timer_request(EXO* exo, IPC* ipc)
{
    TIMER_NUM num = (TIMER_NUM)ipc->param1;
    if (num >= TIMERS_COUNT)
    {
        kerror(ERROR_INVALID_PARAMS);
        return;
    }
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_OPEN:
        stm32_timer_open(exo, num, ipc->param2);
        break;
    case IPC_CLOSE:
        stm32_timer_close(exo, num);
        break;
    case TIMER_START:
        stm32_timer_start(exo, num, (TIMER_VALUE_TYPE)ipc->param2, ipc->param3);
        break;
    case TIMER_STOP:
        stm32_timer_stop(num);
        break;
#if (TIMER_IO)
    case TIMER_SETUP_CHANNEL:
        stm32_timer_setup_channel(num, TIMER_CHANNEL_VALUE(ipc->param2), TIMER_CHANNEL_TYPE_VALUE(ipc->param2), ipc->param3);
        break;
#endif //TIMER_IO
    default:
        kerror(ERROR_NOT_SUPPORTED);
        break;
    }
}
