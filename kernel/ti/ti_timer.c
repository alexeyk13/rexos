/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "ti_timer.h"
#include "ti_exo_private.h"
#include "ti_power.h"
#include "../../userspace/htimer.h"
#include "ti_config.h"
#include "../kirq.h"
#include "../dbg.h"
#include "../ksystime.h"
#include "../kerror.h"

#define S1_US                                       1000000

typedef GPT_Type* GPT_Type_P;
static const GPT_Type_P TIMER_REG[TIMER_MAX] =      {GPT0, GPT1, GPT2, GPT3};

void ti_timer_second_isr(int vector, void* param)
{
    TIMER_REG[SECOND_PULSE_TIMER]->ICLR = GPT_ICLR_TATOCINT;
    ksystime_second_pulse();
}

void ti_timer_hpet_isr(int vector, void* param)
{
    TIMER_REG[HPET_TIMER]->ICLR = GPT_ICLR_TATOCINT;
    ksystime_hpet_timeout();
}

static void ti_timer_open(EXO* exo, TIMER timer, unsigned int flags)
{
    ti_power_domain_on(exo, POWER_DOMAIN_PERIPH);

    //gate clock for GPTx
    PRCM->GPTCLKGR |= (1 << timer);
    PRCM->CLKLOADCTL = PRCM_CLKLOADCTL_LOAD;
    while ((PRCM->CLKLOADCTL & PRCM_CLKLOADCTL_LOAD_DONE) == 0) {}

    //configure as concatenated 32bit timer
    TIMER_REG[timer]->CFG = GPT_CFG_CFG_32BIT_TIMER;

    TIMER_REG[timer]->TAMR = (flags & TIMER_ONE_PULSE) ? GPT_TAMR_TAMR_ONE_SHOT : GPT_TAMR_TAMR_PERIODIC;
    if (flags & TIMER_IRQ_ENABLE)
    {
        //clear pending interrupts
        NVIC_ClearPendingIRQ(Timer0A_IRQn + (timer << 1));
        //enable interrupts
        TIMER_REG[timer]->IMR |= GPT_IMR_TATOIM;
        NVIC_EnableIRQ(Timer0A_IRQn + (timer << 1));
        NVIC_SetPriority(Timer0A_IRQn + (timer << 1), TIMER_IRQ_PRIORITY_VALUE(flags));
    }
}

static inline void ti_timer_close(EXO* exo, unsigned int timer)
{
    NVIC_DisableIRQ(Timer0A_IRQn + (timer << 1));

    //disable clock for GPTx
    PRCM->GPTCLKGR &= ~(1 << timer);
    PRCM->CLKLOADCTL = PRCM_CLKLOADCTL_LOAD;
    while ((PRCM->CLKLOADCTL & PRCM_CLKLOADCTL_LOAD_DONE) == 0) {}

    ti_power_domain_off(exo, POWER_DOMAIN_PERIPH);
}

static void ti_timer_start(EXO* exo, TIMER timer, TIMER_VALUE_TYPE value_type, unsigned int value)
{
    switch (value_type)
    {
    case TIMER_VALUE_HZ:
        if (value == 0 || value > S1_US)
        {
            kerror(ERROR_INVALID_PARAMS);
            return;
        }
        TIMER_REG[timer]->TAILR = TIMER_REG[timer]->TAV = exo->timer.core_clock / value - 1;
        break;
    case TIMER_VALUE_US:
        TIMER_REG[timer]->TAILR = TIMER_REG[timer]->TAV = (value / S1_US) * exo->timer.core_clock - 1;
        value %= S1_US;
        TIMER_REG[timer]->TAILR = TIMER_REG[timer]->TAV += exo->timer.core_clock_us * value - 1;
        break;
    case TIMER_VALUE_CLK:
        TIMER_REG[timer]->TAILR = TIMER_REG[timer]->TAV = value - 1;
        break;
    default:
        kerror(ERROR_NOT_SUPPORTED);
        return;
    }
    TIMER_REG[timer]->CTL |= GPT_CTL_TAEN;
}

static inline void ti_timer_stop(TIMER timer)
{
    TIMER_REG[timer]->CTL &= ~GPT_CTL_TAEN;
}

void ti_timer_request(EXO* exo, IPC* ipc)
{
    TIMER timer = (TIMER)ipc->param1;
    if (timer >= TIMER_MAX)
    {
        kerror(ERROR_NOT_SUPPORTED);
        return;
    }
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_OPEN:
        ti_timer_open(exo, timer, ipc->param2);
        break;
    case IPC_CLOSE:
        ti_timer_close(exo, timer);
        break;
    case TIMER_START:
        ti_timer_start(exo, timer, ipc->param2, ipc->param3);
        break;
    case TIMER_STOP:
        ti_timer_stop(timer);
        break;
    default:
        kerror(ERROR_NOT_SUPPORTED);
    }
}

void hpet_start(unsigned int value, void* param)
{
    EXO* exo = param;
    ti_timer_start(exo, HPET_TIMER, TIMER_VALUE_US, value);
}

void hpet_stop(void* param)
{
    ti_timer_stop(HPET_TIMER);
}

unsigned int hpet_elapsed(void* param)
{
    EXO* exo = param;
    return (TIMER_REG[HPET_TIMER]->TAILR - TIMER_REG[HPET_TIMER]->TAV) / exo->timer.core_clock_us;
}

void ti_timer_init(EXO* exo)
{
    CB_SVC_TIMER cb_svc_timer;
    exo->timer.core_clock = ti_power_get_core_clock();
    exo->timer.core_clock_us = exo->timer.core_clock / S1_US;

    //setup second pulse
    kirq_register(KERNEL_HANDLE, Timer0A_IRQn + ((SECOND_PULSE_TIMER) << 1), ti_timer_second_isr, exo);
    ti_timer_open(exo, SECOND_PULSE_TIMER, TIMER_IRQ_ENABLE | (5 << TIMER_IRQ_PRIORITY_POS));
    ti_timer_start(exo, SECOND_PULSE_TIMER, TIMER_VALUE_HZ, 1);

    //setup HPET
    kirq_register(KERNEL_HANDLE, Timer0A_IRQn + ((HPET_TIMER) << 1), ti_timer_hpet_isr, exo);
    ti_timer_open(exo, HPET_TIMER, TIMER_IRQ_ENABLE | (5 << TIMER_IRQ_PRIORITY_POS) | TIMER_ONE_PULSE);

    cb_svc_timer.start = hpet_start;
    cb_svc_timer.stop = hpet_stop;
    cb_svc_timer.elapsed = hpet_elapsed;
    ksystime_hpet_setup(&cb_svc_timer, exo);
}
