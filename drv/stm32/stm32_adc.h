/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef STM32_ADC_H
#define STM32_ADC_H

#include "stm32_config.h"
#include "stm32_core.h"

#define ADC_SMPR_1_5                                0
#define ADC_SMPR_7_5                                1
#define ADC_SMPR_13_5                               2
#define ADC_SMPR_28_5                               3
#define ADC_SMPR_41_5                               4
#define ADC_SMPR_55_5                               5
#define ADC_SMPR_71_5                               6
#define ADC_SMPR_239_5                              7

#define STM32_TEMP_SENSOR							16
#define STM32_VREF									17

typedef enum {
    STM32_ADC_GET_VALUE = HAL_IPC(HAL_ADC),
    STM32_ADC_IPC_MAX
} STM32_ADC_IPCS;

void stm32_adc_init(CORE* core);
bool stm32_adc_request(CORE* core, IPC* ipc);

#endif // STM32_ADC_H
