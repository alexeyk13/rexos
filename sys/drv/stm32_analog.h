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
    STM32_ADC_SINGLE_CHANNEL = HAL_IPC(HAL_ADC),
    STM32_ADC_TEMP,
    STM32_DAC_SET_LEVEL = HAL_IPC(HAL_DAC),
    STM32_DAC_UNDERFLOW_DEBUG
} STM32_ANALOG_IPCS;

typedef enum {
    STM32_ADC = HAL_HANDLE(HAL_ADC, 0),
    STM32_DAC1 = HAL_HANDLE(HAL_DAC, 0),
    STM32_DAC2,
    STM32_DAC_DUAL,
} STM32_DAC;

#define DAC_FLAGS_RISING                            (1 << 0)
#define DAC_FLAGS_FALLING                           (1 << 1)
#define DAC_FLAGS_PULLUP                            (1 << 2)
#define DAC_FLAGS_PULLDOWN                          (1 << 3)
#define DAC_FLAGS_TIMER                             (1 << 4)
#define DAC_FLAGS_PIN                               (1 << 5)
#define DAC_FLAGS_TRIGGER_MASK                      (3 << 4)

typedef struct {
    PIN pin;
    TIMER_NUM timer;
    unsigned int value;
    unsigned int flags;
} STM32_DAC_ENABLE;

extern const REX __STM32_ANALOG;

#endif // STM32_ANALOG_H
