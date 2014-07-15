/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "stm32_timer.h"
#include "stm32_config.h"
#include "sys_config.h"
#include "stm32_power.h"
#include "../../userspace/error.h"
#include "../../userspace/timer.h"
#include "../../userspace/irq.h"
#include "../sys_call.h"
#include <string.h>
#include "../../userspace/lib/stdio.h"

void stm32_timer();

const REX __STM32_TIMER = {
    //name
    "STM32 timer",
    //size
    512,
    //priority - driver priority.
    92,
    //flags
    PROCESS_FLAGS_ACTIVE,
    //ipc size
    10,
    //function
    stm32_timer
};

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

typedef struct {
    int shared1, shared8;
    int hpet_uspsc;
}TIMERS;

void stm32_timer_enable(TIMERS* timers, TIMER_NUM num, unsigned int flags)
{
    //power up
    *(TIMER_POWER_PORT[num]) |= 1 << TIMER_POWER_BIT[num];
    TIMER_REGS[num]->CR1 |= TIM_CR1_URS;
    //one-pulse mode
    if (flags & TIMER_FLAG_ONE_PULSE_MODE)
        TIMER_REGS[num]->CR1 |= TIM_CR1_OPM;
    TIMER_REGS[num]->DIER |= TIM_DIER_UIE;

    //enable IRQ
    if (num == TIM_1 || num == TIM_10)
    {
        if (timers->shared1++ == 0)
        {
            NVIC_EnableIRQ(TIMER_VECTORS[num]);
            NVIC_SetPriority(TIMER_VECTORS[num], 15);
        }
    }
    else if (num == TIM_8 || num == TIM_13)
    {
        if (timers->shared8++ == 0)
        {
            NVIC_EnableIRQ(TIMER_VECTORS[num]);
            NVIC_SetPriority(TIMER_VECTORS[num], 15);
        }
    }
    else
    {
        NVIC_EnableIRQ(TIMER_VECTORS[num]);
        NVIC_SetPriority(TIMER_VECTORS[num], 15);
    }
}

void stm32_timer_disable(TIMERS* timers, TIMER_NUM num)
{
    //disable timer
    TIMER_REGS[num]->CR1 &= ~TIM_CR1_CEN;

    //disable IRQ
    if (num == TIM_1 || num == TIM_10)
    {
        if (--timers->shared1== 0)
            NVIC_DisableIRQ(TIMER_VECTORS[num]);
    }
    else if (num == TIM_8 || num == TIM_13)
    {
        if (--timers->shared8 == 0)
            NVIC_DisableIRQ(TIMER_VECTORS[num]);
    }
    else
        NVIC_DisableIRQ(TIMER_VECTORS[num]);

    //power down
    *(TIMER_POWER_PORT[num]) &= ~(1 << TIMER_POWER_BIT[num]);
}

void stm32_timer_start(TIMER_NUM num, unsigned int psc, unsigned int count)
{
    TIMER_REGS[num]->PSC = psc - 1;
    TIMER_REGS[num]->ARR = count - 1;
    TIMER_REGS[num]->CNT = 0;

    TIMER_REGS[num]->EGR = TIM_EGR_UG;
    TIMER_REGS[num]->CR1 |= TIM_CR1_CEN;
}

void stm32_timer_stop(TIMER_NUM num)
{
    TIMER_REGS[num]->CR1 &= ~TIM_CR1_CEN;
    TIMER_REGS[num]->SR &= ~TIM_SR_UIF;
}

unsigned int stm32_timer_get_clock(TIMER_NUM num)
{
    unsigned int ahb, apb;
    HANDLE power = sys_get_object(SYS_OBJECT_POWER);
    if (power == INVALID_HANDLE)
    {
        error(ERROR_NOT_FOUND);
        return 0;
    }
    ahb = get(power, IPC_GET_CLOCK, STM32_CLOCK_AHB, 0, 0);
    switch (num)
    {
    case TIM_1:
    case TIM_8:
    case TIM_9:
    case TIM_10:
    case TIM_11:
    case TIM_15:
    case TIM_16:
    case TIM_17:
        apb = get(power, IPC_GET_CLOCK, STM32_CLOCK_APB2, 0, 0);
        break;
    default:
        apb = get(power, IPC_GET_CLOCK, STM32_CLOCK_APB1, 0, 0);
    }
    if (ahb != apb)
        apb <<= 1;
    return apb;
}

void hpet_isr(int vector, void* param)
{
    TIMER_REGS[HPET_TIMER]->SR &= ~TIM_SR_UIF;
    timer_hpet_timeout();
}

void hpet_start(unsigned int value, void* param)
{
    TIMERS* timers = (TIMERS*)param;
    //find near prescaller
    unsigned int mul = value / 0xffff;
    if (value % 0xffff)
        ++mul;
    stm32_timer_start(HPET_TIMER, timers->hpet_uspsc * mul, value / mul);
}

void hpet_stop(void* param)
{
    stm32_timer_stop(HPET_TIMER);
}

unsigned int hpet_elapsed(void* param)
{
    TIMERS* timers = (TIMERS*)param;
    return (((TIMER_REGS[HPET_TIMER]->PSC) + 1)/ timers->hpet_uspsc) * ((TIMER_REGS[HPET_TIMER]->CNT) + 1);
}

#ifdef TIMER_SOFT_RTC
void second_pulse_isr(int vector, void* param)
{
    TIMER_REGS[SECOND_PULSE_TIMER]->SR &= ~TIM_SR_UIF;
    timer_second_pulse();
}
#endif

#if (SYS_INFO)
static inline void stm32_timer_info()
{
    printf("STM32 timer driver info\n\r\n\r");
    printf("HPET timer: TIM_%d\n\r", HPET_TIMER + 1);
#ifdef TIMER_SOFT_RTC
    printf("Second pulse timer: TIM_%d\n\r", SECOND_PULSE_TIMER + 1);
#endif
    printf("\n\r\n\r");
}
#endif

static inline void stm32_timer_loop(TIMERS* timers)
{
    IPC ipc;
    for (;;)
    {
        ipc_read_ms(&ipc, 0, 0);
        switch (ipc.cmd)
        {
        case IPC_PING:
            ipc_post(&ipc);
            break;
        case IPC_CALL_ERROR:
            break;
        case SYS_SET_STDIO:
            open_stdout();
            ipc_post(&ipc);
            break;
#if (SYS_INFO)
        case SYS_GET_INFO:
            stm32_timer_info();
            ipc_post(&ipc);
            break;
#endif
        case IPC_TIMER_ENABLE:
            if (ipc.param1 < TIMERS_COUNT)
            {
                stm32_timer_enable(timers, (TIMER_NUM)ipc.param1, ipc.param2);
                ipc_post(&ipc);
            }
            else
                ipc_post_error(ipc.process, ERROR_NOT_SUPPORTED);
            break;
        case IPC_TIMER_DISABLE:
            if (ipc.param1 < TIMERS_COUNT)
            {
                stm32_timer_disable(timers, (TIMER_NUM)ipc.param1);
                ipc_post(&ipc);
            }
            else
                ipc_post_error(ipc.process, ERROR_NOT_SUPPORTED);
            break;
        case IPC_TIMER_START:
            if (ipc.param1 < TIMERS_COUNT)
            {
                stm32_timer_start(ipc.param1, ipc.param2, ipc.param3);
                ipc_post(&ipc);
            }
            else
                ipc_post_error(ipc.process, ERROR_NOT_SUPPORTED);
            break;
        case IPC_TIMER_STOP:
            if (ipc.param1 < TIMERS_COUNT)
            {
                stm32_timer_stop(ipc.param1);
                ipc_post(&ipc);
            }
            else
                ipc_post_error(ipc.process, ERROR_NOT_SUPPORTED);
            break;
        case IPC_TIMER_GET_CLOCK:
            if (ipc.param1 < TIMERS_COUNT)
            {
                ipc.param1 = stm32_timer_get_clock(ipc.param1);
                ipc_post(&ipc);
            }
            else
                ipc_post_error(ipc.process, ERROR_NOT_SUPPORTED);
            break;
        default:
            ipc_post_error(ipc.process, ERROR_NOT_SUPPORTED);
            break;
        }
    }
}

void stm32_timer()
{
    TIMERS timers;
    timers.shared1 = timers.shared8 = 0;
    sys_ack(SYS_SET_OBJECT, SYS_OBJECT_TIMER, 0, 0);

    //setup HPET
    irq_register(TIMER_VECTORS[HPET_TIMER], hpet_isr, (void*)&timers);
    timers.hpet_uspsc = stm32_timer_get_clock(HPET_TIMER) / 1000000;
    stm32_timer_enable(&timers, HPET_TIMER, TIMER_FLAG_ONE_PULSE_MODE);
    CB_SVC_TIMER cb_svc_timer;
    cb_svc_timer.start = hpet_start;
    cb_svc_timer.stop = hpet_stop;
    cb_svc_timer.elapsed = hpet_elapsed;
    timer_setup(&cb_svc_timer, &timers);
#ifdef TIMER_SOFT_RTC
    irq_register(TIMER_VECTORS[SECOND_PULSE_TIMER], second_pulse_isr, (void*)&timers);
    stm32_timer_enable(&timers, SECOND_PULSE_TIMER, 0);
    //100us prescaller
    stm32_timer_start(SECOND_PULSE_TIMER, stm32_timer_get_clock(SECOND_PULSE_TIMER) / 10000, 10000);
#endif

    stm32_timer_loop(&timers);
}
