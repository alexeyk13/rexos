/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: RJ (jam_roma@yahoo.com)
*/

#include "nrf_rtc.h"
#include "sys_config.h"
#include "nrf_power.h"
#include "nrf_exo_private.h"
#include "../kerror.h"
#include "../ksystime.h"
#include "../kirq.h"
#include "../../userspace/rtc.h"
#include "../../userspace/time.h"
#include "../../userspace/systime.h"
#include <string.h>

typedef NRF_RTC_Type*                            NRF_RTC_Type_P;

#if defined(NRF51)
const int RTC_VECTORS[RTC_COUNT]          =  {RTC0_IRQn,   RTC1_IRQn};
const NRF_RTC_Type_P RTC_REGS[RTC_COUNT]    =  {NRF_RTC0, NRF_RTC1};
#endif // NRF51

#define RTC_PRESCALER               ((LFCLK/32768) - 1)

#if (NRF_RTC_DRIVER)

#if (KERNEL_SECOND_RTC)
void rtc_isr(int vector, void* param)
{
    EXO* exo = (EXO*)param;
    if(RTC_REGS[SECOND_RTC]->EVENTS_COMPARE[SECOND_RTC_CHANNEL] != 0)
    {
        // clear event flag
        RTC_REGS[SECOND_RTC]->EVENTS_COMPARE[0] = 0;
        // set nex shot
        RTC_REGS[SECOND_RTC]->CC[SECOND_RTC_CHANNEL] += exo->rtc.clock_freq;
        // system second pulse
        ksystime_second_pulse();
    }
}
#endif // KERNEL_SECOND_RTC

static void nrf_rtc_open(EXO* exo, RTC_NUM num)
{
    // stop all tasks
    RTC_REGS[num]->POWER = 1;
    RTC_REGS[num]->TASKS_STOP = 1;

    // set prescaler to a TICK of RTC_FREQUENCY
    // 12 bit prescaler for COUNTER frequency (32768/(PRESCALER+1)).Must be written when RTC is stopped
    RTC_REGS[num]->PRESCALER = RTC_PRESCALER;

    // set priority
    NVIC_SetPriority(RTC_VECTORS[num], (14 << RTC_IRQ_PRIORITY_POS));
    // enable Interrupt for RTC in the core.
    NVIC_EnableIRQ(RTC_VECTORS[num]);
}

static void nrf_rtc_close(EXO* exo, RTC_NUM num)
{
    // stop all tasks
    RTC_REGS[num]->TASKS_STOP = 1;
    RTC_REGS[num]->PRESCALER = 0;

    NVIC_DisableIRQ(RTC_VECTORS[num]);
}

static void nrf_rtc_setup_clk(EXO* exo, RTC_NUM num, uint8_t cc, unsigned int clk)
{
    RTC_REGS[num]->CC[cc] = clk;
}

static void nrf_rtc_setup_sec(EXO* exo, TIMER_NUM num, uint8_t cc_num, unsigned int sec)
{
    nrf_rtc_setup_clk(exo, num, cc_num, sec * LFCLK);
}

static void nrf_rtc_start_channel(EXO* exo, RTC_NUM num, RTC_CC cc, unsigned int flags, RTC_CC_TYPE type, unsigned int value)
{
    // set freq
    switch(type)
    {
        case RTC_CC_TYPE_SEC:
            nrf_rtc_setup_sec(exo, num, cc, value);
            break;
        case RTC_CC_TYPE_CLK:
            nrf_rtc_setup_clk(exo, num, cc, value);
            break;
        case RTC_CC_DISABLE:
        default:
            break;
    }

    if(flags & RTC_IRQ_ENABLE)
        RTC_REGS[num]->INTENSET |= ((RTC_INTENSET_COMPARE0_Set << cc) << RTC_INTENSET_COMPARE0_Pos);


    RTC_REGS[num]->TASKS_START = 1;
}

static void nrf_rtc_stop_channel(EXO* exo, RTC_NUM num, RTC_CC cc)
{
    // stop channel
    RTC_REGS[num]->TASKS_STOP = 1;
}

void nrf_rtc_init(EXO* exo)
{
    exo->rtc.clock_freq = LFCLK;
    // init second timer
#if (KERNEL_SECOND_RTC)
    kirq_register(KERNEL_HANDLE, RTC_VECTORS[SECOND_RTC], rtc_isr, (void*)exo);
    nrf_rtc_stop_channel(exo, SECOND_RTC, SECOND_RTC_CHANNEL);
    nrf_rtc_open(exo, SECOND_RTC);
    nrf_rtc_start_channel(exo, SECOND_RTC, SECOND_RTC_CHANNEL, RTC_IRQ_ENABLE, RTC_CC_TYPE_SEC, 1);
#endif // KERNEL_SECOND_RTC
}

TIME* nrf_rtc_get(TIME* time)
{
    struct tm ts;
    ts.tm_sec = 0;
    ts.tm_min = 0;
    ts.tm_hour = 0;
    ts.tm_msec = 0;

    ts.tm_mday = 0;
    ts.tm_mon = 0;
    ts.tm_year = 0;
    return mktime(&ts, time);
}

void nrf_rtc_set(TIME* time)
{
    // TODO ...
}

void nrf_rtc_request(EXO* exo, IPC* ipc)
{
    TIME time;
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_OPEN:
        if (ipc->param1 == RTC_HANDLE_DEVICE)
            nrf_rtc_open(exo, (RTC_NUM)ipc->param2);
        else
            nrf_rtc_start_channel(exo, ipc->param2, 0, 0, 0, 0); // TODO: rtc start channel
        break;
    case IPC_CLOSE:
        if (ipc->param1 == RTC_HANDLE_DEVICE)
            nrf_rtc_close(exo, (RTC_NUM)ipc->param2);
        else
            nrf_rtc_stop_channel(exo, ipc->param2, 0); // TODO: rtc stop channel
        break;
        break;
    case RTC_GET:
        nrf_rtc_get(&time);
        ipc->param1 = (unsigned int)time.day;
        ipc->param2 = (unsigned int)time.ms;
        break;
    case RTC_SET:
        time.day = ipc->param1;
        time.ms = ipc->param2;
        nrf_rtc_set(&time);
        break;
    default:
        kerror(ERROR_NOT_SUPPORTED);
        break;
    }
}

#endif // NRF_RTC_DRIVER
