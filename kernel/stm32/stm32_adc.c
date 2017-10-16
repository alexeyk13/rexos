/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "stm32_adc.h"
#include "stm32_pin.h"
#include "stm32_exo_private.h"
#include "sys_config.h"
#include "../kernel.h"
#include "../kerror.h"

#if defined (STM32F1)
#define ADC_TSTAB                                                   11
#define ADC_SQR_LEN(len)                                            (((len - 1ul) & 0xful) << 20ul)
#elif defined(STM32L0)
#define ADC_CLOCK_MAX                                               16000000
#define ADC_TSTAB                                                   10
#endif //STM32F1

#define STM32_ADC_REGULAR_CHANNELS_COUNT                            16


static inline void stm32_adc_open_device(EXO* exo)
{
    if (exo->adc.active)
    {
        kerror(ERROR_ALREADY_CONFIGURED);
        return;
    }
    //enable clock
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
    //turn ADC on
#ifdef STM32F1
    ADC1->CR2 |= ADC_CR2_ADON;
    exodriver_delay_us(ADC_TSTAB);
    //start self-calibration
    ADC1->CR2 |= ADC_CR2_CAL;
    while(ADC1->CR2 & ADC_CR2_CAL) {}
#elif defined STM32L0
#if (STM32_ADC_ASYNCRONOUS_CLOCK)
    RCC->CR |= RCC_CR_HSION;
    while ((RCC->CR & RCC_CR_HSIRDY) == 0) {}
#else
    //PCLK/2
    ADC1->CFGR2 = 1 << 30;
#endif //STM32_ADC_ASYNCRONOUS_CLOCK
    //first calibrate, than start
    ADC1->CR = ADC_CR_ADCAL;
    while (ADC1->CR & ADC_CR_ADCAL) {}

    //start
    ADC1->CR = ADC_CR_ADEN;
    while ((ADC1->ISR & ADC_ISR_ADRDY) == 0) {}

    ADC1->CFGR1 = ADC_CFGR1_AUTOFF | ADC_CFGR1_WAIT;
#endif
    exo->adc.active = true;
}

static inline void stm32_adc_open_channel(EXO* exo, STM32_ADC_CHANNEL channel)
{
    //enable pin
#ifdef STM32F1
    if (channel >= STM32_ADC_REGULAR_CHANNELS_COUNT)
        ADC1->CR2 |= ADC_CR2_TSVREFE;
#elif defined STM32L0
    switch (channel)
    {
    case STM32_ADC_TEMP:
        ADC->CCR |= ADC_CCR_VLCDEN;
        ADC->CCR |= ADC_CCR_TSEN;
        break;
    case STM32_ADC_VREF:
        ADC->CCR |= ADC_CCR_VREFEN;
        break;
    case STM32_ADC_VLCD:
        ADC->CCR |= ADC_CCR_VLCDEN;
        break;
    default:
        break;
    }
#endif
}

static inline void stm32_adc_close_channel(EXO* exo, STM32_ADC_CHANNEL channel)
{
#ifdef STM32F1
    if (channel >= STM32_ADC_REGULAR_CHANNELS_COUNT)
        ADC1->CR2 &= ~ADC_CR2_TSVREFE;
#elif defined STM32L0
    switch (channel)
    {
    case STM32_ADC_TEMP:
        ADC->CCR &= ~ADC_CCR_TSEN;
        break;
    case STM32_ADC_VREF:
        ADC->CCR &= ~ADC_CCR_VREFEN;
        break;
    case STM32_ADC_VLCD:
        ADC->CCR &= ~ADC_CCR_VLCDEN;
        break;
    default:
        break;
    }
#endif
}

static inline void stm32_adc_close_device(EXO* exo)
{
    if (!exo->adc.active)
    {
        kerror(ERROR_NOT_CONFIGURED);
        return;
    }
    //turn ADC off
#ifdef STM32F1
    ADC1->CR2 &= ~ADC_CR2_ADON;
#elif defined STM32L0
#if (STM32_ADC_ASYNCRONOUS_CLOCK)
    RCC->CR &= ~RCC_CR_HSION;
#endif //STM32_ADC_ASYNCRONOUS_CLOCK
    //stop
    ADC1->CR = ADC_CR_ADDIS;
    while (ADC1->ISR & ADC_CR_ADDIS) {}
#endif
    //disable clock
    RCC->APB2ENR &= ~RCC_APB2ENR_ADC1EN;
    exo->adc.active = false;
}

#ifdef STM32F1
static inline void stm32_adc_set_sequence_channel(int sq, int chan)
{
    if (sq >= 13)
    {
        ADC1->SQR1 &= ~(0x1ful << ((sq - 13ul) * 5ul));
        ADC1->SQR1 |= chan << ((sq - 13ul) * 5ul);
    }
    else if (sq >= 7)
    {
        ADC1->SQR2 &= ~(0x1ful << ((sq - 7ul) * 5ul));
        ADC1->SQR2 |= chan << ((sq - 7ul) * 5ul);
    }
    else //1..6
    {
        ADC1->SQR3 &= ~(0x1ful << ((sq - 1ul) * 5ul));
        ADC1->SQR3 |= chan << ((sq - 1ul) * 5ul);
    }
}

static inline void stm32_adc_set_sample_rate(int chan, int sample_rate)
{
    if (chan >= 10)
    {
        ADC1->SMPR1 &= ~(0x7ul << ((chan - 10ul) * 3ul));
        ADC1->SMPR1 |= sample_rate << ((chan - 10ul) * 3ul);
    }
    else
    {
        ADC1->SMPR2 &= ~(0x7ul << (chan * 3ul));
        ADC1->SMPR2 |= sample_rate << (chan * 3ul);
    }
}
#endif //STM32F1

static int stm32_adc_get(EXO* exo, STM32_ADC_CHANNEL channel, unsigned int samplerate)
{
#ifdef STM32F1
    stm32_adc_set_sample_rate(channel, samplerate);

    //sequence len = 1
    ADC1->SQR1 = ADC_SQR_LEN(1);
    stm32_adc_set_sequence_channel(1, channel);

    //start conversion cycle
    ADC1->SR = 0;
    ADC1->CR2 |= ADC_CR2_ADON;
    while ((ADC1->SR & ADC_SR_EOC) == 0) {}

    return ADC1->DR;
#elif defined STM32L0
    ADC1->SMPR = samplerate;
    ADC1->CHSELR = 1 << channel;
    ADC1->CR |= ADC_CR_ADSTART;
    while ((ADC1->ISR & ADC_ISR_EOC) == 0) {}
    ADC1->ISR |= ADC_ISR_EOC;
    return ADC1->DR;
#endif
}

void stm32_adc_request(EXO* exo, IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
    case ADC_GET:
        ipc->param2 = stm32_adc_get(exo, ipc->param1, ipc->param2);
        break;
    case IPC_OPEN:
        if (ipc->param1 == ADC_HANDLE_DEVICE)
            stm32_adc_open_device(exo);
        else
            stm32_adc_open_channel(exo, ipc->param1);
        break;
    case IPC_CLOSE:
        if (ipc->param1 == ADC_HANDLE_DEVICE)
            stm32_adc_close_device(exo);
        else
            stm32_adc_close_channel(exo, ipc->param1);
        break;
    default:
        kerror(ERROR_NOT_SUPPORTED);
        break;
    }
}

void stm32_adc_init(EXO* exo)
{
    exo->adc.active = false;
}
