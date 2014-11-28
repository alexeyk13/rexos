/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/
#ifndef STM32_ANALOG_H
#define STM32_ANALOG_H

#include "../../../userspace/process.h"
#include "../../../userspace/core/stm32.h"
#include "../../sys.h"
#include "stm32_gpio.h"
#include "stm32_timer.h"
#include "stm32_config.h"
#if (MONOLITH_ANALOG)
#include "stm32_core.h"
#endif

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
    STM32_DAC1 = 0,
    STM32_DAC2,
    STM32_DAC_DUAL,
    STM32_DAC_MAX
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

typedef struct {
    uint8_t flags;
    uint8_t pin, timer;
    HANDLE block, process;
    void* ptr;
    STM32_DAC num;

#if (DAC_DMA)
    char fifo[DAC_DMA_FIFO_SIZE * 2];
    uint16_t cnt, half;
#else
    uint16_t processed;
#endif
    uint16_t size;
} DAC_STRUCT;

typedef struct {
    DAC_STRUCT* dac[2];
} ANALOG_DRV;

#if (MONOLITH_ANALOG)
#define SHARED_ANALOG_DRV                    CORE
#else

typedef struct {
    ANALOG_DRV analog;
} SHARED_ANALOG_DRV;
#endif

bool stm32_analog_request(SHARED_ANALOG_DRV* drv, IPC* ipc);
void stm32_analog_init(SHARED_ANALOG_DRV* drv);

#if !(MONOLITH_ANALOG)
extern const REX __STM32_ANALOG;
#endif

#endif // STM32_ANALOG_H
