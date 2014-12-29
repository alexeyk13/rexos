/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "stm32_timer.h"
#include "../../../userspace/stm32_driver.h"
#include "stm32_power.h"
#include "stm32_core_private.h"
#include "../../../userspace/error.h"
#include "../../../userspace/timer.h"
#include "../../../userspace/irq.h"
#include <string.h>
#if (SYS_INFO)
#include "../../../userspace/stdio.h"
#endif

#define APB1                                    (unsigned int*)((unsigned int)RCC_BASE + offsetof(RCC_TypeDef, APB1ENR))
#define APB2                                    (unsigned int*)((unsigned int)RCC_BASE + offsetof(RCC_TypeDef, APB2ENR))

typedef unsigned int*                           uint_p;

#if defined (STM32F1)
#define TIMERS_COUNT                            17
const TIM_TypeDef_P TIMER_REGS[TIMERS_COUNT] =  {TIM1, TIM2, TIM3, TIM4, TIM5, TIM6, TIM7, TIM8, TIM9, TIM10, TIM11, TIM12, TIM13, TIM14, TIM15, TIM16, TIM17};
const int TIMER_VECTORS[TIMERS_COUNT] =         {25,   28,   29,   30,   50,   54,   55,   44,   24,   25,    26,    43,    44,    45,    24,    25,    26};
const int TIMER_POWER_BIT[TIMERS_COUNT] =       {11,   0,    1,    2,    3,    4,    5,    13,   19,   20,    21,    6,     7,     8,     16,    17,    18};
const uint_p TIMER_POWER_PORT[TIMERS_COUNT] =   {APB2, APB1, APB1, APB1, APB1, APB1, APB1, APB2, APB2, APB2,  APB2,  APB1,  APB1,  APB1,  APB2,  APB2,  APB2};
const PIN TIMER_EXT_PINS[TIMERS_COUNT][6] =     {
                                                    //TIM_1
                                                    {A8, E9, PIN_UNUSED, A9, E11, PIN_UNUSED},
                                                    //TIM_2
                                                    {A0, A15, PIN_UNUSED, A1, B3, PIN_UNUSED},
                                                    //TIM_3
                                                    {A6, B4, C6, A7, B5, C7},
                                                    //TIM_4
                                                    {B6, D12, PIN_UNUSED, B7, D13, PIN_UNUSED},
                                                    //TIM_5
                                                    {A0, PIN_UNUSED, PIN_UNUSED, A1, PIN_UNUSED, PIN_UNUSED},
                                                    //TIM_6
                                                    {PIN_UNUSED, PIN_UNUSED, PIN_UNUSED, PIN_UNUSED, PIN_UNUSED, PIN_UNUSED},
                                                    //TIM_7
                                                    {PIN_UNUSED, PIN_UNUSED, PIN_UNUSED, PIN_UNUSED, PIN_UNUSED, PIN_UNUSED},
                                                    //TIM_8
                                                    {C6, PIN_UNUSED, PIN_UNUSED, C7, PIN_UNUSED, PIN_UNUSED},
                                                    //TIM_9
                                                    {A2, E5, PIN_UNUSED, A3, E6, PIN_UNUSED},
                                                    //TIM_10
                                                    {B8, F6, PIN_UNUSED, PIN_UNUSED, PIN_UNUSED, PIN_UNUSED},
                                                    //TIM_11
                                                    {B9, F7, PIN_UNUSED, PIN_UNUSED, PIN_UNUSED, PIN_UNUSED},
                                                    //TIM_12
#ifdef STM32F10X_LD_VL
                                                    {C4, B12, PIN_UNUSED, C5, B13, PIN_UNUSED},
#else
                                                    {B14, PIN_UNUSED, PIN_UNUSED, B15, PIN_UNUSED, PIN_UNUSED},
#endif
                                                    //TIM_13
                                                    {A6, F8, PIN_UNUSED, PIN_UNUSED, PIN_UNUSED, PIN_UNUSED},
                                                    //TIM_14
                                                    {A7, F9, PIN_UNUSED, F9, PIN_UNUSED, PIN_UNUSED},
                                                    //TIM_15
                                                    {A2, B14, PIN_UNUSED, A3, B15, PIN_UNUSED},
                                                    //TIM_16
                                                    {B8, A6, PIN_UNUSED, PIN_UNUSED, PIN_UNUSED, PIN_UNUSED},
                                                    //TIM_17
                                                    {B9, A7, PIN_UNUSED, PIN_UNUSED, PIN_UNUSED, PIN_UNUSED}
                                                };
const uint32_t TIMER_REMAP[TIMERS_COUNT][2] =   {
                                                    //TIM_1
                                                    {3 << 6, 0},
                                                    //TIM_2
                                                    {3 << 8, 0},
                                                    //TIM_3
                                                    {2 << 10, 3 << 10},
                                                    //TIM_4
                                                    {1 << 12, 0},
                                                    //TIM_5
                                                    {0, 0},
                                                    //TIM_6
                                                    {0, 0},
                                                    //TIM_7
                                                    {0, 0},
                                                    //TIM_8
                                                    {0, 0},
                                                    //TIM_9
                                                    {1 << 5, 0},
                                                    //TIM_10
                                                    {1 << 6, 0},
                                                    //TIM_11
                                                    {1 << 7, 0},
                                                    //TIM_12
                                                    {1 << 12, 0},
                                                    //TIM_13
                                                    {1 << 8, 0},
                                                    //TIM_14
                                                    {1 << 9, 0},
                                                    //TIM_15
                                                    {1 << 0, 0},
                                                    //TIM_16
                                                    {1 << 1, 0},
                                                    //TIM_17
                                                    {1 << 2, 0}
                                                };
const uint32_t TIMER_REMAP_MASK[TIMERS_COUNT] = {3 << 6, 3 << 8, 3 << 10, 1 << 12, 0, 0, 0, 0, 1 << 5, 1 << 6, 1 << 7, 1 << 12, 1 << 8, 1 << 9, 1 << 0, 1 << 1, 1 << 2};

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
const PIN TIMER_EXT_PINS[TIMERS_COUNT][6] =     {
                                                    //TIM_2
                                                    {A0, A5, A15, PIN_UNUSED, PIN_UNUSED, PIN_UNUSED},
                                                    //TIM_6
                                                    {PIN_UNUSED, PIN_UNUSED, PIN_UNUSED, PIN_UNUSED, PIN_UNUSED, PIN_UNUSED},
                                                    //TIM_21
                                                    {A2, B13, PIN_UNUSED, A3, PIN_UNUSED, PIN_UNUSED},
                                                    //TIM_22
                                                    {A6, C6, B4, PIN_UNUSED, PIN_UNUSED, PIN_UNUSED}
                                                };
#endif

void stm32_timer_enable(CORE *core, TIMER_NUM num, unsigned int flags)
{
    if (num >= TIMERS_COUNT)
    {
        error(ERROR_NOT_SUPPORTED);
        return;
    }
    //power up
    *(TIMER_POWER_PORT[num]) |= 1 << TIMER_POWER_BIT[num];
    TIMER_REGS[num]->CR1 |= TIM_CR1_URS;
    //one-pulse mode
    if (flags & TIMER_FLAG_ONE_PULSE_MODE)
        TIMER_REGS[num]->CR1 |= TIM_CR1_OPM;
    if (flags & TIMER_FLAG_ENABLE_IRQ)
        TIMER_REGS[num]->DIER |= TIM_DIER_UIE;

#if defined(STM32F1) || defined(STM32F2) || defined(STM32F4)
    if (num == TIM_1 || num == TIM_10)
    {
        if (core->timer.shared1++ == 0)
        {
            if (flags & TIMER_FLAG_ENABLE_IRQ)
            {
                NVIC_EnableIRQ(TIMER_VECTORS[num]);
                NVIC_SetPriority(TIMER_VECTORS[num], flags >> TIMER_FLAG_PRIORITY);
            }
        }
    }
    else if (num == TIM_8 || num == TIM_13)
    {
        if (core->timer.shared8++ == 0)
        {
            if (flags & TIMER_FLAG_ENABLE_IRQ)
            {
                NVIC_EnableIRQ(TIMER_VECTORS[num]);
                NVIC_SetPriority(TIMER_VECTORS[num], flags >> TIMER_FLAG_PRIORITY);
            }
        }
    }
    else
#endif
        if (flags & TIMER_FLAG_ENABLE_IRQ)
        {
            NVIC_EnableIRQ(TIMER_VECTORS[num]);
            NVIC_SetPriority(TIMER_VECTORS[num], flags >> TIMER_FLAG_PRIORITY);
        }
}

void stm32_timer_disable(CORE *core, TIMER_NUM num)
{
    if (num >= TIMERS_COUNT)
    {
        error(ERROR_NOT_SUPPORTED);
        return;
    }
    //disable timer
    TIMER_REGS[num]->CR1 &= ~TIM_CR1_CEN;

    //disable IRQ
#if defined(STM32F1) || defined(STM32F2) || defined(STM32F4)
    if (num == TIM_1 || num == TIM_10)
    {
        if (--core->timer.shared1== 0)
            NVIC_DisableIRQ(TIMER_VECTORS[num]);
    }
    else if (num == TIM_8 || num == TIM_13)
    {
        if (--core->timer.shared8 == 0)
            NVIC_DisableIRQ(TIMER_VECTORS[num]);
    }
    else
#endif
        NVIC_DisableIRQ(TIMER_VECTORS[num]);

    //power down
    *(TIMER_POWER_PORT[num]) &= ~(1 << TIMER_POWER_BIT[num]);
}

void stm32_timer_enable_ext_clock(CORE *core, TIMER_NUM num, PIN pin, unsigned int flags)
{
    int i, channel;
    for (i = 0; i < 6; ++i)
    {
        if (TIMER_EXT_PINS[num][i] == pin)
        {
#if defined (STM32F1)
            //map AFIO
            if (i % 3)
            {
                stm32_gpio_request_inside(core, STM32_GPIO_ENABLE_AFIO, 0, 0, 0);
                if (num <= TIM_8)
                    AFIO->MAPR |= TIMER_REMAP[num][(i % 3) - 1];
                else
                    AFIO->MAPR2|= TIMER_REMAP[num][(i % 3) - 1];
            }
#endif
            channel = i / 3;
            stm32_timer_enable(core, num, flags);
            //enable pin, decode pullup/down
            switch (flags & TIMER_FLAG_PULL_MASK)
            {
#if defined(STM32F1)
            case TIMER_FLAG_PULLUP:
                stm32_gpio_request_inside(core, STM32_GPIO_ENABLE_PIN, pin, STM32_GPIO_MODE_INPUT_PULL, false);
                break;
            case TIMER_FLAG_PULLDOWN:
                stm32_gpio_request_inside(core, STM32_GPIO_ENABLE_PIN, pin, STM32_GPIO_MODE_INPUT_PULL, true);
                break;
            default:
                stm32_gpio_request_inside(core, STM32_GPIO_ENABLE_PIN, pin, STM32_GPIO_MODE_INPUT_FLOAT, false);
#elif  defined(STM32F2) || defined(STM32F4)
            case TIMER_FLAG_PULLUP:
                stm32_gpio_request_inside(core, STM32_GPIO_ENABLE_PIN, pin, STM32_GPIO_MODE_INPUT | GPIO_SPEED_LOW | GPIO_PUPD_PULLUP, AF0);
                break;
            case TIMER_FLAG_PULLDOWN:
                stm32_gpio_request_inside(core, STM32_GPIO_ENABLE_PIN, pin, STM32_GPIO_MODE_INPUT | GPIO_SPEED_LOW | GPIO_PUPD_PULLDOWN, AF0);
                break;
            default:
                stm32_gpio_request_inside(core, STM32_GPIO_ENABLE_PIN, pin, STM32_GPIO_MODE_INPUT | GPIO_SPEED_LOW | GPIO_PUPD_NO_PULLUP, AF0);
#elif  defined(STM32L0)
            case TIMER_FLAG_PULLUP:
                stm32_gpio_request_inside(core, STM32_GPIO_ENABLE_PIN, pin, STM32_GPIO_MODE_INPUT | GPIO_SPEED_VERY_LOW | GPIO_PUPD_PULLUP, AF0);
                break;
            case TIMER_FLAG_PULLDOWN:
                stm32_gpio_request_inside(core, STM32_GPIO_ENABLE_PIN, pin, STM32_GPIO_MODE_INPUT | GPIO_SPEED_VERY_LOW | GPIO_PUPD_PULLDOWN, AF0);
                break;
            default:
                stm32_gpio_request_inside(core, STM32_GPIO_ENABLE_PIN, pin, STM32_GPIO_MODE_INPUT | GPIO_SPEED_VERY_LOW | GPIO_PUPD_NO_PULLUP, AF0);
#endif
            }
            //map to input, no filter
            TIMER_REGS[num]->CCMR1 = 1 << (8 * (channel));
            //decode rising/falling
            TIMER_REGS[num]->CCER &= ~(0xf << (channel * 4));
            switch (flags & TIMER_FLAG_EDGE_MASK)
            {
            case TIMER_FLAG_RISING:
                // 00
                break;
            case TIMER_FLAG_FALLING:
                // 01
                TIMER_REGS[num]->CCER |= (1 << 1) << (channel * 4);
                break;
            default:
                // 11
                TIMER_REGS[num]->CCER |= ((1 << 1) | (1 << 3)) << (channel * 4);
            }
            // enable compare
            TIMER_REGS[num]->CCER |= (1 << 0) << (channel * 4);
            //connect TI1/2, select ext. clock
            TIMER_REGS[num]->SMCR = ((5 + channel) << 4) | 7;
            return;
        }
    }
    error(ERROR_INVALID_PARAMS);
}

void stm32_timer_disable_ext_clock(CORE *core, TIMER_NUM num, PIN pin)
{
    int i;
    for (i = 0; i < 6; ++i)
    {
        if (TIMER_EXT_PINS[num][i] == pin)
        {
            //disable timer
            stm32_timer_disable(core, num);
            //disable pin
            stm32_gpio_request_inside(core, STM32_GPIO_DISABLE_PIN, pin, 0, 0);
#if defined (STM32F1)
            //unmap AFIO
            if (i % 3)
            {
                if (num <= TIM_8)
                    AFIO->MAPR &= ~TIMER_REMAP_MASK[num];
                else
                    AFIO->MAPR2 &= ~TIMER_REMAP_MASK[num];
                stm32_gpio_request_inside(core, STM32_GPIO_DISABLE_AFIO, 0, 0, 0);
            }
#endif
            return;
        }
    }
    error(ERROR_INVALID_PARAMS);
}

unsigned int stm32_timer_get_clock(CORE* core, TIMER_NUM num)
{
    if (num >= TIMERS_COUNT)
    {
        error(ERROR_NOT_SUPPORTED);
        return 0;
    }
    unsigned int ahb, apb;
    ahb = stm32_power_get_clock_inside(core, STM32_CLOCK_AHB);
    switch (num)
    {
#if defined(STM32F1) || defined(STM32F2) || defined(STM32F4)
    case TIM_1:
    case TIM_8:
    case TIM_9:
    case TIM_10:
    case TIM_11:
    case TIM_15:
    case TIM_16:
    case TIM_17:
#else
    case TIM_21:
    case TIM_22:
#endif
        apb = stm32_power_get_clock_inside(core, STM32_CLOCK_APB2);
        break;
    default:
        apb = stm32_power_get_clock_inside(core, STM32_CLOCK_APB1);
    }
    if (ahb != apb)
        apb <<= 1;
    return apb;
}

void stm32_timer_setup_hz(CORE* core, TIMER_NUM num, unsigned int hz)
{
    if (num >= TIMERS_COUNT)
    {
        error(ERROR_NOT_SUPPORTED);
        return;
    }
    unsigned int psc, value, clock;
    //setup psc
    clock = stm32_timer_get_clock(core, num);
    //period in clock units, rounded
    value = (clock * 10) / hz;
    if (value % 10 >= 5)
        value += 10;
    value /= 10;
    psc = value / 0xffff;
    if (value % 0xffff)
        ++psc;

    TIMER_REGS[num]->PSC = psc - 1;
    TIMER_REGS[num]->ARR = (value / psc) - 1;
}

void stm32_timer_start(TIMER_NUM num)
{
    if (num >= TIMERS_COUNT)
    {
        error(ERROR_NOT_SUPPORTED);
        return;
    }
    TIMER_REGS[num]->CNT = 0;
    TIMER_REGS[num]->EGR = TIM_EGR_UG;
    TIMER_REGS[num]->CR1 |= TIM_CR1_CEN;
}

void stm32_timer_stop(TIMER_NUM num)
{
    if (num >= TIMERS_COUNT)
    {
        error(ERROR_NOT_SUPPORTED);
        return;
    }
    TIMER_REGS[num]->CR1 &= ~TIM_CR1_CEN;
    TIMER_REGS[num]->SR &= ~TIM_SR_UIF;
}

void hpet_isr(int vector, void* param)
{
    TIMER_REGS[HPET_TIMER]->SR &= ~TIM_SR_UIF;
    timer_hpet_timeout();
}

void hpet_start(unsigned int value, void* param)
{
    CORE* core = (CORE*)param;
    //find near prescaller
    unsigned int mul = value / 0xffff;
    if (value % 0xffff)
        ++mul;

    TIMER_REGS[HPET_TIMER]->PSC = core->timer.hpet_uspsc * mul - 1;
    TIMER_REGS[HPET_TIMER]->ARR = value / mul - 1;
    TIMER_REGS[HPET_TIMER]->CNT = 0;

    TIMER_REGS[HPET_TIMER]->EGR = TIM_EGR_UG;
    TIMER_REGS[HPET_TIMER]->CR1 |= TIM_CR1_CEN;
}

void hpet_stop(void* param)
{
    stm32_timer_stop(HPET_TIMER);
}

unsigned int hpet_elapsed(void* param)
{
    CORE* core = (CORE*)param;
    return (((TIMER_REGS[HPET_TIMER]->PSC) + 1)/ core->timer.hpet_uspsc) * ((TIMER_REGS[HPET_TIMER]->CNT) + 1);
}

#if (TIMER_SOFT_RTC)
void second_pulse_isr(int vector, void* param)
{
    TIMER_REGS[SECOND_PULSE_TIMER]->SR &= ~TIM_SR_UIF;
    timer_second_pulse();
}
#endif

#if (SYS_INFO)
void stm32_timer_info()
{
    printd("HPET timer: TIM_%d\n\r", HPET_TIMER + 1);
#if (TIMER_SOFT_RTC)
    printd("Second pulse timer: TIM_%d\n\r", SECOND_PULSE_TIMER + 1);
#endif
}
#endif

void stm32_timer_init(CORE *core)
{
#if defined(STM32F1) || defined(STM32F2) || defined(STM32F4)
    core->timer.shared1 = core->timer.shared8 = 0;
#endif

    //setup HPET
    irq_register(TIMER_VECTORS[HPET_TIMER], hpet_isr, (void*)core);
    core->timer.hpet_uspsc = stm32_timer_get_clock(core, HPET_TIMER) / 1000000;
    stm32_timer_enable(core, HPET_TIMER, TIMER_FLAG_ONE_PULSE_MODE | TIMER_FLAG_ENABLE_IRQ | (13 << TIMER_FLAG_PRIORITY));
    CB_SVC_TIMER cb_svc_timer;
    cb_svc_timer.start = hpet_start;
    cb_svc_timer.stop = hpet_stop;
    cb_svc_timer.elapsed = hpet_elapsed;
    timer_setup(&cb_svc_timer, core);
#if (TIMER_SOFT_RTC)
    irq_register(TIMER_VECTORS[SECOND_PULSE_TIMER], second_pulse_isr, (void*)core);
    stm32_timer_enable(core, SECOND_PULSE_TIMER, TIMER_FLAG_ENABLE_IRQ | (13 << TIMER_FLAG_PRIORITY));
    stm32_timer_setup_hz(core, SECOND_PULSE_TIMER, 1);
    stm32_timer_start(SECOND_PULSE_TIMER);
#endif
}

bool stm32_timer_request(CORE* core, IPC* ipc)
{
    bool need_post = false;
    switch (ipc->cmd)
    {
#if (SYS_INFO)
    case IPC_GET_INFO:
        stm32_timer_info();
        need_post = true;
        break;
#endif
    case STM32_TIMER_ENABLE:
        stm32_timer_enable(core, (TIMER_NUM)ipc->param1, ipc->param2);
        need_post = true;
        break;
    case STM32_TIMER_DISABLE:
        stm32_timer_disable(core, (TIMER_NUM)ipc->param1);
        need_post = true;
        break;
    case STM32_TIMER_ENABLE_EXT_CLOCK:
        stm32_timer_enable_ext_clock(core, (TIMER_NUM)ipc->param1, (PIN)ipc->param2, ipc->param3);
        need_post = true;
        break;
    case STM32_TIMER_DISABLE_EXT_CLOCK:
        stm32_timer_disable_ext_clock(core, (TIMER_NUM)ipc->param1, (PIN)ipc->param2);
        need_post = true;
        break;
    case STM32_TIMER_SETUP_HZ:
        stm32_timer_setup_hz(core, (TIMER_NUM)ipc->param1, ipc->param2);
        need_post = true;
        break;
    case STM32_TIMER_START:
        stm32_timer_start((TIMER_NUM)ipc->param1);
        need_post = true;
        break;
    case STM32_TIMER_STOP:
        stm32_timer_stop(ipc->param1);
        need_post = true;
        break;
    case STM32_TIMER_GET_CLOCK:
        ipc->param1 = stm32_timer_get_clock(core, ipc->param1);
        need_post = true;
        break;
    default:
        ipc_set_error(ipc, ERROR_NOT_SUPPORTED);
        need_post = true;
        break;
    }
    return need_post;
}
