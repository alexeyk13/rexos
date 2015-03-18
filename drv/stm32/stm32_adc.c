/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "stm32_adc.h"
#include "stm32_core_private.h"
#include "sys_config.h"

#if defined (STM32F1)
#define ADC_TSTAB                                       11
#elif defined(STM32L0)
#define ADC_CLOCK_MAX                                   16000000
#define ADC_TSTAB                                       10
#endif //STM32F1
#define ADC_SQR_LEN(len)                                (((len - 1ul) & 0xful) << 20ul)
#define ADC2uV(raw)                                     ((raw * ADC_VREF * 100 / 0xfff) * 10ul)


static inline void stm32_adc_open_device()
{
    //enable clock
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
    //turn ADC on
#ifdef STM32F1
    ADC1->CR2 |= ADC_CR2_ADON;
#if (STM32_ADC_VREF_CHANNEL) || (STM32_ADC_TEMP_CHANNEL)
    ADC1->CR2 |= ADC_CR2_TSVREFE;
#endif //(STM32_ADC_VREF_CHANNEL) || (STM32_ADC_TEMP_CHANNEL)
    sleep_us(ADC_TSTAB);
    //start self-calibration
    ADC1->CR2 |= ADC_CR2_CAL;
    while(ADC1->CR2 & ADC_CR2_CAL) {}
#elif defined STM32L0
    //first calibrate, than start
    ADC1->CR = ADC_CR_ADCAL;
    while (ADC1->CR & ADC_CR_ADCAL) {}

    //start
    ADC1->CR = ADC_CR_ADEN;
    while ((ADC1->ISR & ADC_ISR_ADRDY) == 0) {}
#endif
}

//TODO: close

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

static int stm32_adc_get_single_sample(int chan, int sample_rate)
{
#ifdef STM32F1
    stm32_adc_set_sample_rate(chan, sample_rate);

    //sequence len = 1
    ADC1->SQR1 = ADC_SQR_LEN(1);
    stm32_adc_set_sequence_channel(1, chan);

    //start conversion cycle
    ADC1->SR = 0;
    ADC1->CR2 |= ADC_CR2_ADON;
    while ((ADC1->SR & ADC_SR_EOC) == 0) {}

    return ADC1->DR;
#elif defined STM32L0
    //TODO:
    return 999;
#endif
}

static inline int stm32_adc_get_temp()
{
    return (V25_MV * 1000 - ADC2uV(stm32_adc_get_single_sample(STM32_TEMP_SENSOR, ADC_SMPR_239_5))) * 10 / AVG_SLOPE + 25l * 10l;
}

#if (SYS_INFO)
static inline void stm32_adc_info(CORE* core)
{
    //TODO:
}
#endif //SYS_INFO

bool stm32_adc_request(CORE* core, IPC* ipc)
{
    bool need_post = false;
    switch (ipc->cmd)
    {
#if (SYS_INFO)
    case IPC_GET_INFO:
        stm32_adc_info(drv);
        need_post = true;
        break;
#endif
    case STM32_ADC_GET_VALUE:
        ipc->param2 = stm32_adc_get_single_sample(ipc->param1, ipc->param2);
        need_post = true;
        break;
//    case STM32_ADC_TEMP:
//        ipc->param2 = stm32_adc_get_temp();
//        need_post = true;
//        break;
    case IPC_OPEN:
        stm32_adc_open_device();
        need_post = true;
        break;
    case IPC_CLOSE:
        //TODO:
        error(ERROR_INVALID_PARAMS);
        need_post = true;
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
        need_post = true;
        break;
    }
    return need_post;
}

void stm32_adc_init(CORE* core)
{
    //TODO:
}
