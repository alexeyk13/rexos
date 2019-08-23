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

//                                                    16 Bit       8 Bit       24 Bit      32 Bits
const unsigned int max_timer_value[4]           =  {0x0000FFFF, 0x000000FF, 0x10000000, 0xFFFFFFFF};

#endif // NRF51

#if (NRF_TIMER_DRIVER)

void nrf_timer_isr(int vector, void* param)
{
    if((TIMER_REGS[HPET_TIMER]->EVENTS_COMPARE[HPET_CHANNEL] != 0) &&
         ((TIMER_REGS[HPET_TIMER]->INTENSET & (TIMER_INTENSET_COMPARE0_Msk << HPET_CHANNEL)) != 0))
    {
        // clear compare register
        TIMER_REGS[HPET_TIMER]->EVENTS_COMPARE[HPET_CHANNEL] = 0;
        ksystime_hpet_timeout();
    }

#if !(KERNEL_SECOND_RTC)
    if((TIMER_REGS[HPET_TIMER]->EVENTS_COMPARE[SECOND_CHANNEL] != 0) &&
         ((TIMER_REGS[HPET_TIMER]->INTENSET & (TIMER_INTENSET_COMPARE0_Msk << SECOND_CHANNEL)) != 0))
    {
        // clear compare register
        TIMER_REGS[HPET_TIMER]->EVENTS_COMPARE[SECOND_CHANNEL] = 0;
        // set nex shot
        TIMER_REGS[HPET_TIMER]->CC[SECOND_CHANNEL] += S1_US;
        // system second pulse

        TIMER_REGS[HPET_TIMER]->TASKS_CAPTURE[HPET_CHANNEL] = 1;
        ksystime_second_pulse();
    }
#endif // KERNEL_SECOND_RTC
}

void nrf_timer_open(EXO* exo, TIMER_NUM num, unsigned int flags)
{
    // keep flags for channel enable/disable
    exo->timer.flags[num] = flags;
    // power up
    TIMER_REGS[num]->POWER = 1;
    TIMER_REGS[num]->TASKS_STOP = 1;
    // clear timer counter
    TIMER_REGS[num]->TASKS_CLEAR = 1;
    // timer mode
    TIMER_REGS[num]->MODE = TIMER_MODE_MODE_Timer;
    // set bitmode
    if(TIMER_0 == num)
        TIMER_REGS[num]->BITMODE = TIMER_BITMODE_BITMODE_32Bit;
    else
        TIMER_REGS[num]->BITMODE = TIMER_BITMODE_BITMODE_16Bit;
    // set prescaler
    TIMER_REGS[num]->PRESCALER = exo->timer.psc;

//    if (flags & TIMER_ONE_PULSE)
//    if (flags & TIMER_EXT_CLOCK)
    // enable IRQ
    if (flags & TIMER_IRQ_ENABLE)
    {
        NVIC_EnableIRQ(TIMER_VECTORS[num]);
        NVIC_SetPriority(TIMER_VECTORS[num], TIMER_IRQ_PRIORITY_VALUE(flags));
    }
}

void nrf_timer_close(EXO* exo, TIMER_NUM num)
{
    // disable timer
    TIMER_REGS[num]->TASKS_STOP = 1;
    // disable IRQ
    NVIC_DisableIRQ(TIMER_VECTORS[num]);
    // power down
    TIMER_REGS[num]->TASKS_SHUTDOWN = 1;
}

static void nrf_timer_setup_cc(EXO* exo, TIMER_NUM num, uint8_t cc_num, unsigned int cc_value)
{
    // setup cc
    TIMER_REGS[num]->CC[cc_num] = cc_value;
}

void nrf_timer_disable_channel(EXO* exo, TIMER_NUM num, uint8_t cc_num)
{
    // disable channel
    TIMER_REGS[num]->CC[cc_num] = 0;
    // disable irq
    if(exo->timer.flags[num] & TIMER_IRQ_ENABLE)
        TIMER_REGS[num]->INTENSET &= ~((TIMER_INTENSET_COMPARE0_Set << cc_num) << TIMER_INTENSET_COMPARE0_Pos);
}

void nrf_timer_enable_channel(EXO* exo, TIMER_NUM num, uint8_t cc_num, TIMER_VALUE_TYPE value_type, unsigned int value)
{
    switch (value_type)
    {
    case TIMER_VALUE_HZ:
        nrf_timer_setup_cc(exo, num, cc_num, 1000000 / value);
        break;
    case TIMER_VALUE_DISABLE:
        nrf_timer_disable_channel(exo, num, cc_num);
        break;
    default:
        nrf_timer_setup_cc(exo, num, cc_num, value);
        break;
    }
    // enable irq
    if(exo->timer.flags[num] & TIMER_IRQ_ENABLE)
        TIMER_REGS[num]->INTENSET |= ((TIMER_INTENSET_COMPARE0_Set << cc_num) << TIMER_INTENSET_COMPARE0_Pos);
}

void nrf_timer_start(EXO* exo, TIMER_NUM num)
{
    // start timer
    TIMER_REGS[num]->TASKS_START = 1;
}

void nrf_timer_stop(TIMER_NUM num)
{
    // stop timer
    TIMER_REGS[num]->TASKS_STOP = 1;
}

void hpet_start(unsigned int value, void* param)
{
    EXO* exo = (EXO*)param;
#if !(KERNEL_SECOND_RTC)
    // cache value of cc
    unsigned int val =  TIMER_REGS[HPET_TIMER]->CC[SECOND_CHANNEL];
    // get actual counter
    TIMER_REGS[HPET_TIMER]->TASKS_CAPTURE[SECOND_CHANNEL] = 1;
    TIMER_REGS[HPET_TIMER]->TASKS_CAPTURE[HPET_CHANNEL] = 1;
    // cache actual counter
    exo->timer.hpet_start = TIMER_REGS[HPET_TIMER]->CC[SECOND_CHANNEL];
    // set cached value back
    TIMER_REGS[HPET_TIMER]->CC[SECOND_CHANNEL] = val;
#else
    TIMER_REGS[HPET_TIMER]->TASKS_CAPTURE[HPET_CHANNEL] = 1;
    exo->timer.hpet_start = TIMER_REGS[HPET_TIMER]->CC[HPET_CHANNEL];
#endif // KERNEL_SECOND_RTC
    //don't need to start in free-run mode, second pulse will go faster anyway
    if(value < S1_US)
        nrf_timer_enable_channel(exo, HPET_TIMER, HPET_CHANNEL, TIMER_VALUE_CLK, exo->timer.hpet_start + value);
}

void hpet_stop(void* param)
{
    nrf_timer_disable_channel((EXO*)param, HPET_TIMER, HPET_CHANNEL);
}

unsigned int hpet_elapsed(void* param)
{
    EXO* exo = (EXO*)param;
    /* cache timer value */
    unsigned int cache_val = TIMER_REGS[HPET_TIMER]->CC[HPET_CHANNEL];
    unsigned int tc = 0;
    /* extract actual value */
    TIMER_REGS[HPET_TIMER]->TASKS_CAPTURE[HPET_CHANNEL] = 1;
    tc = TIMER_REGS[HPET_TIMER]->CC[HPET_CHANNEL];
    // set cached value back
    TIMER_REGS[HPET_TIMER]->CC[HPET_CHANNEL] = cache_val;
    return (exo->timer.hpet_start < tc ? tc - exo->timer.hpet_start : max_timer_value[TIMER_REGS[HPET_TIMER]->BITMODE] + 1 - exo->timer.hpet_start + tc);
}

void nrf_timer_init(EXO* exo)
{
    CB_SVC_TIMER cb_svc_timer;
    exo->timer.core_clock = nrf_power_get_clock_inside(exo);
    exo->timer.hpet_uspsc = exo->timer.core_clock / S1_US;

    // count prescaler
    exo->timer.psc = 0;
    while(exo->timer.hpet_uspsc >>= 1)
        ++exo->timer.psc;

    kirq_register(KERNEL_HANDLE, TIMER_VECTORS[HPET_TIMER], nrf_timer_isr, (void*)exo);
    nrf_timer_open(exo, HPET_TIMER, TIMER_IRQ_ENABLE | (13 << TIMER_IRQ_PRIORITY_POS));
    // start timer
    nrf_timer_start(exo, HPET_TIMER);

    cb_svc_timer.start = hpet_start;
    cb_svc_timer.stop = hpet_stop;
    cb_svc_timer.elapsed = hpet_elapsed;
    ksystime_hpet_setup(&cb_svc_timer, exo);

#if !(KERNEL_SECOND_RTC)
    // setup second counter
    nrf_timer_enable_channel(exo, HPET_TIMER, SECOND_CHANNEL, TIMER_VALUE_US, S1_US);
#endif // KERNEL_SECOND_RTC
}

void nrf_timer_request(EXO* exo, IPC* ipc)
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
        nrf_timer_open(exo, num, ipc->param2);
        break;
    case IPC_CLOSE:
        nrf_timer_close(exo, num);
        break;
    case TIMER_START:
        nrf_timer_start(exo, num);
        break;
    case TIMER_STOP:
        nrf_timer_stop(num);
        break;
    case TIMER_SETUP_CHANNEL:
        nrf_timer_enable_channel(exo, num, TIMER_CHANNEL_VALUE(ipc->param2), TIMER_CHANNEL_TYPE_VALUE(ipc->param2), ipc->param3);
        break;
    default:
        kerror(ERROR_NOT_SUPPORTED);
    }
}

#endif // NRF_TIMER_DRIVER
