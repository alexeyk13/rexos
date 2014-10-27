/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/
#include "stm32_rtc.h"
#include "../rtc.h"
#include "stm32_config.h"
#include "sys_config.h"
#include "../sys.h"
#include "../../userspace/lib/stdio.h"
#include "../../userspace/lib/time.h"
#include "../../userspace/irq.h"
#include "../../userspace/timer.h"

#if defined(STM32L0)
#define RTC_EXTI_LINE                               20
#endif

void stm32_rtc_isr(int vector, void* param)
{
#if defined(STM32F1)
    while ((RTC->CRL & RTC_CRL_RTOFF) == 0) {}
    RTC->CRL = 0;
#else
    EXTI->PR = 1 << RTC_EXTI_LINE;
    RTC->ISR &= ~RTC_ISR_WUTF;
#endif
    timer_second_pulse();
}

static inline void backup_on()
{
#if defined(STM32F1) || defined(STM32F2) || defined(STM32F4)
    //enable POWER and BACKUP interface
    RCC->APB1ENR |= RCC_APB1ENR_PWREN | RCC_APB1ENR_BKPEN;
    PWR->CR |= PWR_CR_DBP;
#elif defined(STM32L0)
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;
    PWR->CR |= PWR_CR_DBP;
    __disable_irq();
    RTC->WPR = 0xca;
    RTC->WPR = 0x53;
    __enable_irq();
#endif
}

static inline void backup_off()
{
#if defined(STM32F1) || defined(STM32F2) || defined(STM32F4)
    //disable POWER and BACKUP interface
    PWR->CR &= ~PWR_CR_DBP;
    RCC->APB1ENR &= ~(RCC_APB1ENR_PWREN | RCC_APB1ENR_BKPEN);
#elif defined(STM32L0)
    __disable_irq();
    RTC->WPR = 0x00;
    RTC->WPR = 0xff;
    __enable_irq();
    PWR->CR &= ~PWR_CR_DBP;
    RCC->APB1ENR &= ~RCC_APB1ENR_PWREN;
#endif
}

static inline void enter_configuration()
{
#if defined(STM32F1)
    while ((RTC->CRL & RTC_CRL_RTOFF) == 0) {}
    RTC->CRL |= RTC_CRL_CNF;
#else
    RTC->ISR |= RTC_ISR_INIT;
    while ((RTC->ISR & RTC_ISR_INITF) == 0) {}
#endif
}

static inline void leave_configuration()
{
#if defined(STM32F1)
    RTC->CRL &= ~RTC_CRL_CNF;
    while ((RTC->CRL & RTC_CRL_RTOFF) == 0) {}
#else
    RTC->ISR &= ~RTC_ISR_INIT;
#endif
}

void stm32_rtc_init()
{
    backup_on();

    //backup domain reset?
#if defined(STM32F1)
    if (RCC->BDCR == 0)
    {
        //turn on 32khz oscillator
        RCC->BDCR |= RCC_BDCR_LSEON;
        while ((RCC->BDCR & RCC_BDCR_LSERDY) == 0) {}
        //select LSE as clock source
        RCC->BDCR |= (01ul << 8ul);
        //turn on RTC
        RCC->BDCR |= RCC_BDCR_RTCEN;

        enter_configuration();

        //prescaller to 1 sec
        RTC->PRLH = (LSE_VALUE >> 16) & 0xf;
        RTC->PRLL = (LSE_VALUE & 0xffff) - 1;

        //reset counter & alarm
        RTC->CNTH = 0;
        RTC->CNTL = 0;
        RTC->ALRH = 0;
        RTC->ALRL = 0;

        leave_configuration();
    }

    //wait for APB1<->RTC_CORE sync
    RTC->CRL &= RTC_CRL_RSF;
    while ((RTC->CRL & RTC_CRL_RSF) == 0) {}

    //enable second pulse
    while ((RTC->CRL & RTC_CRL_RTOFF) == 0) {}
    RTC->CRH |= RTC_CRH_SECIE;
#else
    if ((RCC->CSR & RCC_CSR_RTCEN) == 0)
    {
        RCC->CSR &= ~(3 << 16);

#if (LSE_VALUE)
        //turn on 32khz oscillator
        RCC->CSR |= RCC_CSR_LSEON;
        while ((RCC->CSR & RCC_CSR_LSERDY) == 0) {}
        //select LSE as clock source
        RCC->CSR |= RCC_CSR_RTCSEL_LSE;
#else
        //select HSE as clock source
        RCC->CSR |= RCC_CSR_RTCSEL_HSE;
#endif
        //turn on RTC
        RCC->CSR |= RCC_CSR_RTCEN;
        enter_configuration();

        //prescaller to 1 sec
#if (LSE_VALUE)
        RTC->PRER = ((128 - 1) << 16) | (LSE_VALUE / 128 - 1);
#else
        RTC->PRER = ((64 - 1) << 16) | (1000000 / 64 - 1);
#endif

        //setup second tick
        RTC->CR &= ~RTC_CR_WUTE;
        while ((RTC->ISR & RTC_ISR_WUTWF) == 0) {}
        RTC->CR |= 4 | RTC_CR_WUTIE;
        RTC->WUTR = 0;
        RTC->CR |= RTC_CR_WUTE;
        leave_configuration();

    }

    //setup EXTI for second pulse
    EXTI->IMR |= 1 << RTC_EXTI_LINE;
    EXTI->RTSR |= 1 << RTC_EXTI_LINE;
#endif
    irq_register(RTC_IRQn, stm32_rtc_isr, NULL);
    NVIC_EnableIRQ(RTC_IRQn);
    NVIC_SetPriority(RTC_IRQn, 13);
}

time_t stm32_rtc_get()
{
#if defined(STM32F1)
    return (time_t)(((RTC->CNTH) << 16ul) | (RTC->CNTL));
#else
    //fucking shit
    struct tm ts;
    ts.tm_sec = ((RTC->TR >> 4) & 7) * 10 + (RTC->TR & 0xf);
    ts.tm_min = ((RTC->TR >> 12) & 7) * 10 + ((RTC->TR >> 8) & 0xf);
    ts.tm_hour = ((RTC->TR >> 20) & 3) * 10 + ((RTC->TR >> 16) & 0xf);

    ts.tm_mday = ((RTC->DR >> 4) & 3) * 10 + (RTC->DR & 0xf);
    ts.tm_mon = ((RTC->DR >> 12) & 1) * 10 + ((RTC->DR >> 8) & 0xf);
    ts.tm_year = ((RTC->DR >> 20) & 0xf) * 10 + ((RTC->DR >> 16) & 0xf) + 2000;

    return mktime(&ts);
#endif
}

void stm32_rtc_set(time_t time)
{
    enter_configuration();
#if defined(STM32F1)
    //reset counter & alarm
    RTC->CNTH = (uint16_t)(time >> 16ul);
    RTC->CNTL = (uint16_t)(time & 0xffff);

#else
    //fucking shit
    struct tm ts;
    gmtime(time, &ts);
    ts.tm_year -= 2000;
    RTC->TR = ((ts.tm_hour / 10) << 20) | ((ts.tm_hour % 10) << 16) |
              ((ts.tm_min / 10) << 12)  | ((ts.tm_min % 10) << 8)   |
              ((ts.tm_sec / 10) << 4)   | (ts.tm_sec % 10);
    RTC->DR = ((ts.tm_year / 10) << 20) | ((ts.tm_year % 10) << 16) |
              ((ts.tm_mon / 10) << 12)  | ((ts.tm_mon % 10) << 8)   |
              ((ts.tm_mday / 10) << 4)  | (ts.tm_mday % 10) | (1 << 13);
#endif
    leave_configuration();
}

#if (SYS_INFO)
void stm32_rtc_info()
{
    printf("LSE clock: %d\n\r", LSE_VALUE);
    struct tm tm;
    gmtime(stm32_rtc_get(), &tm);
    printf("Now: %d.%d.%d %d:%d:%d\n\r", tm.tm_mday, tm.tm_mon + 1, tm.tm_year, tm.tm_hour, tm.tm_min, tm.tm_sec);
    printf("\n\r\n\r");
}
#endif


bool stm32_rtc_request(IPC* ipc)
{
    bool need_post = false;
    switch (ipc->cmd)
    {
#if (SYS_INFO)
    case IPC_GET_INFO:
        stm32_rtc_info();
        need_post = true;
        break;
#endif
    case RTC_GET:
        ipc->param1 = (unsigned int)stm32_rtc_get();
        need_post = true;
        break;
    case RTC_SET:
        stm32_rtc_set((time_t)ipc->param1);
        need_post = true;
        break;
    default:
        ipc_set_error(ipc, ERROR_NOT_SUPPORTED);
        need_post = true;
        break;
    }
    return need_post;
}
