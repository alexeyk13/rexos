/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: RJ (jam_roma@yahoo.com)
*/

#include "nrf_timer.h"
#include "sys_config.h"
#include "nrf_power.h"
#include "nrf_exo_private.h"
#include "../kerror.h"
#include "../ksystime.h"
#include "../kirq.h"
#include <string.h>

#define S1_US                                      1000000

typedef NRF_TIMER_Type*                            NRF_TIMER_Type_P;

#if defined(NRF51)
const int TIMER_VECTORS[TIMERS_COUNT]           =  {TIMER0_IRQn,   TIMER1_IRQn,   TIMER2_IRQn};
const NRF_TIMER_Type_P TIMER_REGS[TIMERS_COUNT] =  {NRF_TIMER0, NRF_TIMER1, NRF_TIMER2};
#endif // NRF51

#if (NRF_TIMER_DRIVER)

void nrf_timer_hpet_isr(int vector, void* param)
{
    if(TIMER_REGS[HPET_TIMER]->EVENTS_COMPARE[HPET_CHANNEL] != 0)
    {
        // clear compare register
        TIMER_REGS[HPET_TIMER]->EVENTS_COMPARE[HPET_CHANNEL] = 0;
        ksystime_hpet_timeout();
    }
}

void nrf_timer_open(EXO* exo, TIMER_NUM num, unsigned int flags)
{
    //power up
    //TIMER_REGS[num]->POWER          = 1;
    TIMER_REGS[num]->TASKS_STOP     = 1;

    if(TIMER_0 == num)
        TIMER_REGS[num]->BITMODE = TIMER_BITMODE_BITMODE_32Bit;
    else
        TIMER_REGS[num]->BITMODE = TIMER_BITMODE_BITMODE_16Bit;

    // timer mode
    TIMER_REGS[num]->MODE = TIMER_MODE_MODE_Timer;

//    if (flags & TIMER_ONE_PULSE)
//    if (flags & TIMER_EXT_CLOCK)

    // enable IRQ
    if (flags & TIMER_IRQ_ENABLE)
    {
        NVIC_EnableIRQ(TIMER_VECTORS[num]);
        NVIC_SetPriority(TIMER_VECTORS[num], TIMER_IRQ_PRIORITY_VALUE(flags));
    }
    // kepp flags for next
    exo->timer.flags[num] = flags;
}

void nrf_timer_close(EXO* exo, TIMER_NUM num)
{
    //disable timer
    TIMER_REGS[num]->TASKS_STOP = 1;
    //disable IRQ
    NVIC_DisableIRQ(TIMER_VECTORS[num]);
    //power down
    TIMER_REGS[num]->TASKS_SHUTDOWN = 1;
}

static void nrf_timer_setup_clk(EXO* exo, TIMER_NUM num, uint8_t cc_num, unsigned int psc,  unsigned int clk)
{
    // enable interrupt
    if(exo->timer.flags[num] & TIMER_IRQ_ENABLE)
        TIMER_REGS[num]->INTENSET |= ((TIMER_INTENSET_COMPARE0_Set << cc_num) << TIMER_INTENSET_COMPARE0_Pos);

    // set prescaler
    TIMER_REGS[num]->PRESCALER = psc;
    // set clock
    TIMER_REGS[num]->CC[cc_num] = clk;
}

static void nrf_timer_setup_us(EXO* exo, TIMER_NUM num, uint8_t cc_num, unsigned int us)
{
    uint32_t freq = exo->timer.core_clock_us;
    uint32_t n = 0;
    // count prescaler
    while(freq >>= 1)
        ++n;
    nrf_timer_setup_clk(exo, num, cc_num, n, us);
}

void nrf_timer_disable_channel(EXO* exo, TIMER_NUM num, uint8_t cc_num)
{
    // disable channel
    TIMER_REGS[num]->CC[cc_num] = 0;
    // disable irq
    if(exo->timer.flags[num] & TIMER_IRQ_ENABLE)
        TIMER_REGS[num]->INTENSET &= ~((TIMER_INTENSET_COMPARE0_Set << cc_num) << TIMER_INTENSET_COMPARE0_Pos);
}

void nrf_timer_setup_channel(EXO* exo, TIMER_NUM num, uint8_t cc_num, TIMER_VALUE_TYPE value_type, unsigned int value)
{
    switch (value_type)
    {
    case TIMER_VALUE_HZ:
        nrf_timer_setup_us(exo, num, cc_num, 1000000 / value);
        break;
    case TIMER_VALUE_US:
        nrf_timer_setup_us(exo, num, cc_num, value);
        break;
    case TIMER_VALUE_CLK:
        nrf_timer_setup_clk(exo, num, cc_num, 1, value);
        break;
    default:
        nrf_timer_disable_channel(exo, num, cc_num);
        break;
    }
}

void nrf_timer_start(EXO* exo, TIMER_NUM num, uint8_t cc_num, TIMER_VALUE_TYPE value_type, unsigned int value)
{
    // setup channel
    nrf_timer_setup_channel(exo, num, cc_num, value_type, value);
    // clear timer counter
    TIMER_REGS[num]->TASKS_CLEAR = 1;
    // start timer
    TIMER_REGS[num]->TASKS_START = 1;
}

void nrf_timer_stop(TIMER_NUM num)
{
    // disable timer
    TIMER_REGS[num]->TASKS_STOP = 1;
    // disable IRQ
}

void hpet_start(unsigned int value, void* param)
{
    EXO* exo = (EXO*)param;
    //find near prescaller
    //don't need to start in free-run mode, second pulse will go faster anyway
    if (value < S1_US)
        nrf_timer_start(exo, HPET_TIMER, HPET_CHANNEL, TIMER_VALUE_US, value);
}

void hpet_stop(void* param)
{
    nrf_timer_stop(HPET_TIMER);
    TIMER_REGS[HPET_TIMER]->INTENSET &= ~((TIMER_INTENSET_COMPARE0_Set << HPET_CHANNEL) << TIMER_INTENSET_COMPARE0_Pos);
}

unsigned int hpet_elapsed(void* param)
{
    EXO* exo = (EXO*)param;
    return TIMER_REGS[HPET_TIMER]->CC[HPET_CHANNEL];
}

void nrf_timer_init(EXO* exo)
{
    CB_SVC_TIMER cb_svc_timer;
    exo->timer.core_clock = nrf_power_get_clock_inside(exo);
    exo->timer.core_clock_us = exo->timer.core_clock / S1_US;

    kirq_register(KERNEL_HANDLE, TIMER_VECTORS[HPET_TIMER], nrf_timer_hpet_isr, (void*)exo);
    nrf_timer_open(exo, HPET_TIMER, TIMER_IRQ_ENABLE | (13 << TIMER_IRQ_PRIORITY_POS));
    cb_svc_timer.start = hpet_start;
    cb_svc_timer.stop = hpet_stop;
    cb_svc_timer.elapsed = hpet_elapsed;
    ksystime_hpet_setup(&cb_svc_timer, exo);
}

void nrf_timer_request(EXO* exo, IPC* ipc)
{
    // TODO: get timer

    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_OPEN:
        //nrf_timer_open(exo, timer, ipc->param2);
        break;
    case IPC_CLOSE:
        //nrf_timer_close(exo, timer);
        break;
    case TIMER_START:
        //nrf_timer_start(exo, timer, ipc->param2, ipc->param3);
        break;
    case TIMER_STOP:
//        nrf_timer_stop(timer);
        break;
    default:
        kerror(ERROR_NOT_SUPPORTED);
    }
}

#endif // NRF_TIMER_DRIVER
