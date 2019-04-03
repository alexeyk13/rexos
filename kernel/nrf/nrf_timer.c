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

#define S1_US                                       1000000

typedef NRF_TIMER_Type*                            NRF_TIMER_Type_P;

#if defined(NRF51)
const int TIMER_VECTORS[TIMERS_COUNT]           =  {TIMER0_IRQn,   TIMER1_IRQn,   TIMER2_IRQn};
const NRF_TIMER_Type_P TIMER_REGS[TIMERS_COUNT] =  {NRF_TIMER0, NRF_TIMER1, NRF_TIMER2};
#endif // NRF51

void nrf_timer_open(EXO* exo, TIMER_NUM num, unsigned int flags)
{
    unsigned int channel = (flags & TIMER_MODE_CHANNEL_MASK) >> TIMER_MODE_CHANNEL_POS;
    exo->timer.main_channel[num] = channel;
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
        TIMER_REGS[num]->INTENSET |= TIMER_INTENSET_COMPARE0_Set;

        NVIC_EnableIRQ(TIMER_VECTORS[num]);
        NVIC_SetPriority(TIMER_VECTORS[num], TIMER_IRQ_PRIORITY_VALUE(flags));
    }
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

static unsigned int nrf_timer_get_clock(EXO* exo)
{
    return nrf_power_get_clock_inside(exo);
}

static void nrf_timer_setup_clk(EXO* exo, TIMER_NUM num, uint8_t cc_num, unsigned int psc,  unsigned int clk)
{
    // set prescaler
    TIMER_REGS[num]->PRESCALER = psc;
    // set clock
    TIMER_REGS[num]->CC[cc_num] = clk;
}

static void nrf_timer_setup_us(EXO* exo, TIMER_NUM num, uint8_t cc_num, unsigned int us)
{
    uint32_t freq = nrf_timer_get_clock(exo);
    uint32_t n = 0;
    // Get freq in MHz
    freq /= 1000000;
    // count prescaler
    while(freq >>= 1)
        ++n;
    nrf_timer_setup_clk(exo, num, cc_num, n, us);
}

void nrf_timer_start(EXO* exo, TIMER_NUM num, uint8_t cc_num, TIMER_VALUE_TYPE value_type, unsigned int value)
{
    // clear the task first to be usable for later.
    TIMER_REGS[num]->TASKS_CLEAR = 1;

    switch (value_type)
    {
    case TIMER_VALUE_HZ:
        nrf_timer_setup_us(exo, num, cc_num, 1000000 / value);
        break;
    case TIMER_VALUE_US:
        nrf_timer_setup_us(exo, num, cc_num, value);
        break;
    default:
        nrf_timer_setup_clk(exo, num, cc_num, 1, value);
        break;
    }

    // start timer
    TIMER_REGS[num]->TASKS_START = 1;
}

void nrf_timer_stop(TIMER_NUM num)
{
    // disable timer
    TIMER_REGS[num]->TASKS_STOP = 1;
    // clear interrupt pending register
    TIMER_REGS[num]->TASKS_CLEAR = 1;
}

void nrf_timer_isr(int vector, void* param)
{
    if((TIMER_REGS[SECOND_TIMER]->EVENTS_COMPARE[HPET_CHANNEL] != 0) &&
            ((TIMER_REGS[SECOND_TIMER]->INTENSET & (TIMER_INTENSET_COMPARE0_Msk << HPET_CHANNEL)) != 0))
    {
        // clear compare register
        TIMER_REGS[SECOND_TIMER]->EVENTS_COMPARE[HPET_CHANNEL] = 0;
        // clear interrupt pending register
        TIMER_REGS[SECOND_TIMER]->TASKS_CLEAR = 1;
        ksystime_hpet_timeout();
    }
    if((TIMER_REGS[SECOND_TIMER]->EVENTS_COMPARE[SECOND_CHANNEL] != 0) &&
            ((TIMER_REGS[SECOND_TIMER]->INTENSET & (TIMER_INTENSET_COMPARE0_Msk << SECOND_CHANNEL)) != 0))
    {
        // clear compare register
        TIMER_REGS[SECOND_TIMER]->EVENTS_COMPARE[SECOND_CHANNEL] = 0;
        // clear interrupt pending register
        TIMER_REGS[SECOND_TIMER]->TASKS_CLEAR = 1;
        ksystime_second_pulse();
    }

}

void hpet_start(unsigned int value, void* param)
{
    EXO* exo = (EXO*)param;
    //find near prescaller
    exo->timer.hpet_start = TIMER_REGS[SECOND_TIMER]->CC[HPET_CHANNEL];
    //don't need to start in free-run mode, second pulse will go faster anyway
    if (value < S1_US)
        nrf_timer_start(exo, SECOND_TIMER, HPET_CHANNEL, TIMER_VALUE_US, value);
}

void hpet_stop(void* param)
{
    EXO* exo = (EXO*)param;
    nrf_timer_stop(SECOND_TIMER);
}

unsigned int hpet_elapsed(void* param)
{
    EXO* exo = (EXO*)param;
    unsigned int tc = 0;
    // capture data
    tc = TIMER_REGS[SECOND_TIMER]->CC[HPET_CHANNEL];
    // return elapsed data
    unsigned int value = (exo->timer.hpet_start < tc) ? tc - exo->timer.hpet_start : TIMER_REGS[SECOND_TIMER]->CC[HPET_CHANNEL] + 1 - exo->timer.hpet_start + tc;
    return value;
}

void nrf_timer_init(EXO* exo)
{
    int i = 0;
    for (i = 0; i < TIMER_MAX; ++i)
        exo->timer.main_channel[i] = TIMER_CHANNEL_INVALID;

    //setup second tick timer
    kirq_register(KERNEL_HANDLE, TIMER_VECTORS[SECOND_TIMER], nrf_timer_isr, (void*)exo);
    nrf_timer_open(exo, SECOND_TIMER, TIMER_IRQ_ENABLE | (SECOND_CHANNEL << TIMER_MODE_CHANNEL_POS) | (13 << TIMER_IRQ_PRIORITY_POS));
    nrf_timer_start(exo, SECOND_TIMER, SECOND_CHANNEL, TIMER_VALUE_US, 1);

    CB_SVC_TIMER cb_svc_timer;
    cb_svc_timer.start = hpet_start;
    cb_svc_timer.stop = hpet_stop;
    cb_svc_timer.elapsed = hpet_elapsed;
    ksystime_hpet_setup(&cb_svc_timer, exo);
}

void nrf_timer_request(EXO* exo, IPC* ipc)
{

}
