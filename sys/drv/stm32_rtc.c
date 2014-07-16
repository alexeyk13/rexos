/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/
#include "stm32_rtc.h"
#include "stm32_config.h"
#include "sys_config.h"
#include "../sys_call.h"
#include "stm32_power.h"
#include "../../userspace/lib/stdio.h"
#include "../../userspace/irq.h"
#include "../../userspace/timer.h"

void stm32_rtc();

const REX __STM32_RTC = {
    //name
    "STM32 RTC",
    //size
    512,
    //priority - driver priority
    91,
    //flags
    PROCESS_FLAGS_ACTIVE,
    //ipc size
    10,
    //function
    stm32_rtc
};

void stm32_rtc_isr(int vector, void* param)
{
    while ((RTC->CRL & RTC_CRL_RTOFF) == 0) {}
    RTC->CRL = 0;
    timer_second_pulse();
}

static inline void stm32_rtc_enable()
{
    HANDLE power = sys_get_object(SYS_OBJECT_POWER);
    if (power == INVALID_HANDLE)
        return;
    ack(power, IPC_POWER_BACKUP_ON, 0, 0, 0);
    ack(power, IPC_POWER_BACKUP_WRITE_ENABLE, 0, 0, 0);

    //backup domain reset?
    if (RCC->BDCR == 0)
    {
        //turn on 32khz oscillator
        RCC->BDCR |= RCC_BDCR_LSEON;
        while ((RCC->BDCR & RCC_BDCR_LSERDY) == 0) {}
        //select LSE as clock source
        RCC->BDCR |= (01ul << 8ul);
        //turn on RTC
        RCC->BDCR |= RCC_BDCR_RTCEN;

        //enter configuration mode
        while ((RTC->CRL & RTC_CRL_RTOFF) == 0) {}
        RTC->CRL |= RTC_CRL_CNF;

        //prescaller to 1 sec
        RTC->PRLH = (LSE_VALUE >> 16) & 0xf;
        RTC->PRLL = (LSE_VALUE & 0xffff) - 1;

        //reset counter & alarm
        RTC->CNTH = 0;
        RTC->CNTL = 0;
        RTC->ALRH = 0;
        RTC->ALRL = 0;

        //leave configuration mode
        RTC->CRL &= ~RTC_CRL_CNF;
        while ((RTC->CRL & RTC_CRL_RTOFF) == 0) {}
    }
    ack(power, IPC_POWER_BACKUP_WRITE_PROTECT, 0, 0, 0);

    //wait for APB1<->RTC_CORE sync
    RTC->CRL &= RTC_CRL_RSF;
    while ((RTC->CRL & RTC_CRL_RSF) == 0) {}

    //enable second pulse
    irq_register(RTC_IRQn, stm32_rtc_isr, (void*)process_get_current());
    NVIC_EnableIRQ(RTC_IRQn);
    NVIC_SetPriority(RTC_IRQn, 15);
    while ((RTC->CRL & RTC_CRL_RTOFF) == 0) {}
    RTC->CRH |= RTC_CRH_SECIE;
}

static inline time_t stm32_rtc_get()
{
    return (time_t)(((RTC->CNTH) << 16ul) | (RTC->CNTL));
}

static inline void stm32_rtc_set(time_t time)
{
    HANDLE power = sys_get_object(SYS_OBJECT_POWER);
    if (power == INVALID_HANDLE)
        return;
    ack(power, IPC_POWER_BACKUP_WRITE_ENABLE, 0, 0, 0);

    //enter configuration mode
    while ((RTC->CRL & RTC_CRL_RTOFF) == 0) {}
    RTC->CRL |= RTC_CRL_CNF;

    //reset counter & alarm
    RTC->CNTH = (uint16_t)(time >> 16ul);
    RTC->CNTL = (uint16_t)(time & 0xffff);

    //leave configuration mode
    RTC->CRL &= ~RTC_CRL_CNF;
    while ((RTC->CRL & RTC_CRL_RTOFF) == 0) {}

    ack(power, IPC_POWER_BACKUP_WRITE_PROTECT, 0, 0, 0);
}

#if (SYS_INFO)
void stm32_rtc_info()
{
    printf("STM32 rtc driver info\n\r\n\r");
    printf("LSE clock: %d\n\r", LSE_VALUE);
    struct tm tm;
    gmtime(stm32_rtc_get(), &tm);
    printf("Now: %d.%d.%d %d:%d:%d\n\r", tm.tm_mday, tm.tm_mon + 1, tm.tm_year, tm.tm_hour, tm.tm_min, tm.tm_sec);
    printf("\n\r\n\r");
}
#endif

void stm32_rtc()
{
    IPC ipc;
    sys_ack(SYS_SET_OBJECT, SYS_OBJECT_RTC, 0, 0);
    open_stdout();
    stm32_rtc_enable();
    for (;;)
    {
        ipc_read_ms(&ipc, 0, 0);
        switch (ipc.cmd)
        {
        case IPC_PING:
            ipc_post(&ipc);
            break;
        case IPC_RTC_GET:
            ipc.param1 = (unsigned int)stm32_rtc_get();
            ipc_post(&ipc);
            break;
        case IPC_RTC_SET:
            stm32_rtc_set(ipc.param1);
            ipc_post(&ipc);
            break;
#if (SYS_INFO)
        case SYS_GET_INFO:
            stm32_rtc_info();
            ipc_post(&ipc);
            break;
#endif
        default:
            ipc_post_error(ipc.process, ERROR_NOT_SUPPORTED);
            break;
        }
    }
}
