/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef STM32_ADC_H
#define STM32_ADC_H

#include "stm32_config.h"
#include "stm32_exo.h"
#include "../../userspace/stm32/stm32_driver.h"
#include "../../userspace/adc.h"
#define ADC_CHANNELS_COUNT   STM32_ADC_MAX

typedef struct {
    uint8_t samplerate;
} ADC_CHANNEL_DRV;

typedef struct {
    ADC_CHANNEL_DRV channels[ADC_CHANNELS_COUNT];
    bool active;
} ADC_DRV;

void stm32_adc_init(EXO* exo);
void stm32_adc_request(EXO* exo, IPC* ipc);

#endif // STM32_ADC_H
