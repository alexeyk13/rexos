/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2018, Alexey Kramarenko
    All rights reserved.
*/

#include "../ksystime.h"
#include "../kirq.h"
#include "../kerror.h"
#include "../../userspace/rtc.h"
#include "../../userspace/sys.h"
#include "../../userspace/time.h"
#include "../../userspace/systime.h"
#include "../../userspace/stdio.h"
#include "stm32_exo_private.h"
#include "stm32_rtc.h"
#include "stm32_config.h"
#include "sys_config.h"

#if defined(STM32L0) || defined(STM32L1)
#define RTC_EXTI_LINE                               20
#endif

#if defined(STM32L1)
#define RTC_IRQ                                     RTC_WKUP_IRQn
#else
#define RTC_IRQ                                     RTC_IRQn
#endif

//bug in F0 CMSIS
#ifndef RTC_CR_WUTIE
#define RTC_CR_WUTIE                                (1 << 14)
#endif

void stm32_rtc_isr(int vector, void* param)
{
#if defined(STM32F1)
    while ((RTC->CRL & RTC_CRL_RTOFF) == 0) {}
    RTC->CRL = 0;
#else
    RTC->ISR &= ~RTC_ISR_WUTF;
#endif

#if defined(STM32L0) || defined(STM32L1)
    EXTI->PR |= (1 << RTC_EXTI_LINE);
#endif
    ksystime_second_pulse();
}

static inline void stm32_backup_on()
{
#if defined(STM32F1) || defined(STM32F2) || defined(STM32F4)
    //enable BACKUP interface
    RCC->APB1ENR |= RCC_APB1ENR_BKPEN;
#endif

    PWR->CR |= PWR_CR_DBP;
}

static inline void stm32_backup_off()
{
    PWR->CR &= ~PWR_CR_DBP;

#if defined(STM32F1) || defined(STM32F2) || defined(STM32F4)
    RCC->APB1ENR &= ~RCC_APB1ENR_BKPEN;
#endif
}

static inline void stm32_disable_write_protection()
{
#if defined(STM32L0) || defined(STM32F0) || defined(STM32L1)
    //HSE as clock source can cause faults on pin reset, so reset backup domain is required
#if !(LSE_VALUE)
#if defined(STM32L0) || defined(STM32L1)
    RCC->CSR |= RCC_CSR_RTCRST;
    __NOP();
    __NOP();
    __NOP();
    RCC->CSR &= ~RCC_CSR_RTCRST;
#else
    RCC->BDCR |= RCC_BDCR_BDRST;
    __NOP();
    __NOP();
    __NOP();
    RCC->BDCR &= ~RCC_BDCR_BDRST;
#endif
#endif

    __disable_irq();
    RTC->WPR = 0xca;
    RTC->WPR = 0x53;
    __enable_irq();
#endif
}

static inline void stm32_enable_write_protection()
{
#if defined(STM32L0) || defined(STM32F0) || defined(STM32L1)
    __disable_irq();
    RTC->WPR = 0x00;
    RTC->WPR = 0xff;
    __enable_irq();
#endif
}

static void enter_configuration()
{
#if defined(STM32F1)
    while ((RTC->CRL & RTC_CRL_RTOFF) == 0) {}
    RTC->CRL |= RTC_CRL_CNF;
#else
    RTC->ISR |= RTC_ISR_INIT;
    while ((RTC->ISR & RTC_ISR_INITF) == 0) {}
#endif
}

static void leave_configuration()
{
#if defined(STM32F1)
    RTC->CRL &= ~RTC_CRL_CNF;
    while ((RTC->CRL & RTC_CRL_RTOFF) == 0) {}
#else
    RTC->ISR &= ~RTC_ISR_INIT;
#endif
}

static inline bool stm32_is_backup_domain_reset()
{
#if defined(STM32F1) || defined(STM32F0)
    return ((RCC->BDCR & RCC_BDCR_RTCEN) == 0);
#else
    return ((RCC->CSR & RCC_CSR_RTCEN) == 0);
#endif
}

#if (LSE_VALUE)
static inline void stm32_rtc_on()
{
#if defined(STM32F1) || defined(STM32F0)
        //turn on 32khz oscillator
        RCC->BDCR |= RCC_BDCR_LSEON;
        while ((RCC->BDCR & RCC_BDCR_LSERDY) == 0) {}
        //select LSE as clock source
        RCC->BDCR |= (01ul << 8ul);
        //turn on RTC
        RCC->BDCR |= RCC_BDCR_RTCEN;
#else
        //turn on 32khz oscillator
        RCC->CSR |= RCC_CSR_LSEON;
        while ((RCC->CSR & RCC_CSR_LSERDY) == 0) {}
        //select LSE as clock source
        RCC->CSR |= RCC_CSR_RTCSEL_LSE;
        //turn on RTC
        RCC->CSR |= RCC_CSR_RTCEN;
#endif
}
#else
static inline void stm32_rtc_on()
{
#if defined(STM32F1) || defined(STM32F0)
        //select HSE as clock source
        RCC->BDCR |= (03ul << 8ul);
        //turn on RTC
        RCC->BDCR |= RCC_BDCR_RTCEN;
#else
        //select HSE as clock source
        RCC->CSR |= RCC_CSR_RTCSEL_HSE;
        //turn on RTC
        RCC->CSR |= RCC_CSR_RTCEN;
#endif
}
#endif //LSE_VALUE

static inline void stm32_rtc_configure()
{
    enter_configuration();

#if defined(STM32F1)
    //prescaller to 1 sec
    RTC->PRLH = (LSE_VALUE >> 16) & 0xf;
    RTC->PRLL = (LSE_VALUE & 0xffff) - 1;

    //reset counter & alarm
    RTC->CNTH = 0;
    RTC->CNTL = 0;
    RTC->ALRH = 0;
    RTC->ALRL = 0;
#else
    //prescaller to 1 sec
#if (LSE_VALUE)
    RTC->PRER = ((128 - 1) << 16) | (LSE_VALUE / 128 - 1);
#elif (HSE_RTC_DIV)
    RTC->PRER = ((128 - 1) << 16) | ((HSE_VALUE / HSE_RTC_DIV) / 128 - 1);
#else
    RTC->PRER = ((64 - 1) << 16) | (1000000 / 64 - 1);
#endif

    //00:00
    RTC->TR = 0;
    //01.01.2015, thursday
    RTC->DR = 0x00158101;
    //setup second tick
    RTC->CR &= ~RTC_CR_WUTE;
    while ((RTC->ISR & RTC_ISR_WUTWF) == 0) {}
    RTC->CR |= 4;
    RTC->WUTR = 0;
#endif

    leave_configuration();
}

static inline void stm32_rtc_enable_second_pulse()
{
#if defined(STM32F1)
    //wait for APB1<->RTC_CORE sync
   RTC->CRL &= RTC_CRL_RSF;
   while ((RTC->CRL & RTC_CRL_RSF) == 0) {}
   //enable second pulse
   while ((RTC->CRL & RTC_CRL_RTOFF) == 0) {}
   RTC->CRH |= RTC_CRH_SECIE;
#else

#if defined(STM32L0) || defined(STM32L1)
   // EXTI line 20 is connected to the RTC Wakeup event
   EXTI->IMR |= (1 << RTC_EXTI_LINE);
   EXTI->RTSR |= (1 << RTC_EXTI_LINE);

   // Configure the RTC WakeUp Clock source: CK_SPRE (1Hz)
   RTC->CR &= (uint32_t)~RTC_CR_WUCKSEL;
   RTC->CR |= RTC_CR_WUCKSEL_2;
   // set wakeup counter
   RTC->WUTR = 0;
#endif

    RTC->CR |= RTC_CR_WUTE | RTC_CR_WUTIE;
    RTC->ISR &= ~RTC_ISR_WUTF;
#endif
}

void stm32_rtc_init()
{
    stm32_backup_on();
    stm32_disable_write_protection();

    //backup domain reset?
    if (stm32_is_backup_domain_reset())
    {
        stm32_rtc_on();
        stm32_rtc_configure();
    }

    stm32_rtc_enable_second_pulse();
    stm32_enable_write_protection();

    kirq_register(KERNEL_HANDLE, RTC_IRQ, stm32_rtc_isr, NULL);
    NVIC_EnableIRQ(RTC_IRQ);
    NVIC_SetPriority(RTC_IRQ, 13);
}

TIME* stm32_rtc_get(TIME* time)
{
#if defined(STM32F1)
    unsigned long value = (unsigned long)(((RTC->CNTH) << 16ul) | (RTC->CNTL));
    time->ms = (value % SEC_IN_DAY) * 1000;
    time->day = value / SEC_IN_DAY + EPOCH_DATE;
    return time;
#else
    struct tm ts;
    ts.tm_sec = ((RTC->TR >> 4) & 7) * 10 + (RTC->TR & 0xf);
    ts.tm_min = ((RTC->TR >> 12) & 7) * 10 + ((RTC->TR >> 8) & 0xf);
    ts.tm_hour = ((RTC->TR >> 20) & 3) * 10 + ((RTC->TR >> 16) & 0xf);
    ts.tm_msec = 0;

    ts.tm_mday = ((RTC->DR >> 4) & 3) * 10 + (RTC->DR & 0xf);
    ts.tm_mon = ((RTC->DR >> 12) & 1) * 10 + ((RTC->DR >> 8) & 0xf);
    ts.tm_year = ((RTC->DR >> 20) & 0xf) * 10 + ((RTC->DR >> 16) & 0xf) + 2000;

    return mktime(&ts, time);
#endif
}

void stm32_rtc_set_alarm_sec(uint32_t delta_sec)
{
    uint32_t value;
    enter_configuration();

    value = RTC->CNTL | (RTC->CNTH << 16);
    value += delta_sec;
    RTC->ALRH = (uint16_t)(value >> 16ul);
    RTC->ALRL = (uint16_t)(value & 0xffff);

    leave_configuration();
}

void stm32_rtc_set(TIME* time)
{
    enter_configuration();
#if defined(STM32F1)
    unsigned long value = 0;
    if (time->day >= EPOCH_DATE)
        value = (time->day - EPOCH_DATE) * SEC_IN_DAY;
    value += time->ms / 1000;
    RTC->CNTH = (uint16_t)(value >> 16ul);
    RTC->CNTL = (uint16_t)(value & 0xffff);

#else
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

void stm32_rtc_request(IPC* ipc)
{
    TIME time;
    switch (HAL_ITEM(ipc->cmd))
    {
    case RTC_GET:
        stm32_rtc_get(&time);
        ipc->param1 = (unsigned int)time.day;
        ipc->param2 = (unsigned int)time.ms;
        break;
    case RTC_SET:
        time.day = ipc->param1;
        time.ms = ipc->param2;
        stm32_rtc_set(&time);
        break;
    case RTC_SET_ALARM_SEC:
        stm32_rtc_set_alarm_sec(ipc->param1);

    default:
        kerror(ERROR_NOT_SUPPORTED);
        break;
    }
}

void stm32_rtc_disable()
{
#if defined(STM32F1)
    while ((RTC->CRL & RTC_CRL_RTOFF) == 0) {}
    RTC->CRH = 0;
#elif defined(STM32L0) || defined(STM32L1)
    RTC->CR &= ~(RTC_CR_WUTE | RTC_CR_WUTIE | RTC_CR_TSE | RTC_CR_TSIE | RTC_CR_ALRAE | RTC_CR_ALRAIE | RTC_CR_ALRBE | RTC_CR_ALRBIE);
#else
    RTC->CR &= ~(RTC_CR_WUTE | RTC_CR_WUTIE | RTC_CR_TSE | RTC_CR_TSIE | RTC_CR_ALRAE | RTC_CR_ALRAIE);
#endif
}
