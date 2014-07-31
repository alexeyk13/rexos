/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/
#ifndef STM32_ADC_H
#define STM32_ADC_H

#include "../../userspace/process.h"
#include "../../userspace/core/stm32.h"
#include "../sys.h"

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
    STM32_ADC_SINGLE_CHANNEL = IPC_USER,
    STM32_ADC_TEMP
} STM32_ADC_IPCS;

extern const REX __STM32_ADC;

#endif // STM32_ADC_H
