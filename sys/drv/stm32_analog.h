/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/
#ifndef STM32_ANALOG_H
#define STM32_ANALOG_H

#include "../../userspace/process.h"
#include "../../userspace/core/stm32.h"
#include "../sys.h"
#include "stm32_gpio.h"
#include "stm32_timer.h"

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
    STM32_ADC_TEMP,
} STM32_ANALOG_IPCS;

typedef enum {
    STM32_DAC1,
    STM32_DAC2,
    STM32_DAC_DUAL,
    STM32_ADC
} STM32_DAC;

typedef enum {
    DAC_TRIGGER_TIMER,
    DAC_TRIGGER_PIN
} DAC_TRIGGER;

typedef struct {
    STM32_DAC dac;
    DAC_TRIGGER trigger;

    PIN pin;
    TIMER_NUM timer;
    unsigned int frequency;
} STM32_DAC_ENABLE;

extern const REX __STM32_ANALOG;

#endif // STM32_ANALOG_H
