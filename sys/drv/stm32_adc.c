/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "stm32_adc.h"
#include "stm32_power.h"
#include "../sys.h"

#define ADC1_CLOCK_MAX              14000000
#define ADC1_TSTAB                  11
#define ADC_SQR_LEN(len)            (((len - 1ul) & 0xful) << 20ul)
#define ADC2uV(raw)					((raw * ADC_VREF * 100 / 0xfff) * 10ul)

void stm32_adc();

const REX __STM32_ADC = {
    //name
    "STM32 ADC",
    //size
    512,
    //priority - driver priority
    92,
    //flags
    PROCESS_FLAGS_ACTIVE,
    //ipc size
    10,
    //function
    stm32_adc
};

static inline void stm32_adc_init()
{
    HANDLE core = sys_get_object(SYS_OBJECT_CORE);
    if (core == INVALID_HANDLE)
    {
        error(ERROR_NOT_FOUND);
        return;
    }
    ack(core, STM32_POWER_ADC_ON, 0, 0, 0);

    //turn ADC1 on
    ADC1->CR2 |= ADC_CR2_ADON;
    ADC1->CR2 |= ADC_CR2_TSVREFE;
    sleep_us(ADC1_TSTAB);

    //start self-calibration
    ADC1->CR2 |= ADC_CR2_CAL;
    while(ADC1->CR2 & ADC_CR2_CAL) {}
}

void stm32_adc_set_sequence_channel(int sq, int chan)
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

void stm32_adc_set_sample_rate(int chan, int sample_rate)
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

int stm32_adc_get_single_sample(int chan, int sample_rate)
{
    stm32_adc_set_sample_rate(chan, sample_rate);

    //sequence len = 1
    ADC1->SQR1 = ADC_SQR_LEN(1);
    stm32_adc_set_sequence_channel(1, chan);

    //start conversion cycle
    ADC1->SR = 0;
    ADC1->CR2 |= ADC_CR2_ADON;
    while ((ADC1->SR & ADC_SR_EOC) == 0) {}

    return ADC1->DR;
}

void stm32_adc()
{
    IPC ipc;
    open_stdout();
    stm32_adc_init();

    for (;;)
    {
        ipc_read_ms(&ipc, 0, 0);
        switch (ipc.cmd)
        {
        case IPC_PING:
            ipc_post(&ipc);
            break;
        case IPC_CALL_ERROR:
            break;
#if (SYS_INFO)
        case SYS_GET_INFO:
//            stm32_adc_info(uarts);
            ipc_post(&ipc);
            break;
#endif
        case STM32_ADC_SINGLE_CHANNEL:
            ipc.param1 = stm32_adc_get_single_sample(ipc.param1, ipc.param2);
            ipc_post(&ipc);
            break;
        case STM32_ADC_TEMP:
            ipc.param1 = (V25_MV * 1000 - ADC2uV(stm32_adc_get_single_sample(STM32_TEMP_SENSOR, ADC_SMPR_239_5))) * 10 / AVG_SLOPE + 25l * 10l;
            ipc_post(&ipc);
            break;
        default:
            ipc_post_error(ipc.process, ERROR_NOT_SUPPORTED);
            break;
        }
    }
}
