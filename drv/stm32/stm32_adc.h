/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2016, Alexey Kramarenko
    All rights reserved.
*/

#ifndef STM32_ADC_H
#define STM32_ADC_H

#include "stm32_config.h"
#include "stm32_core.h"
#include "../../userspace/stm32/stm32_driver.h"
#include "../../userspace/adc.h"

typedef struct {
    bool active;
} ADC_DRV;

void stm32_adc_init(CORE* core);
void stm32_adc_request(CORE* core, IPC* ipc);

#endif // STM32_ADC_H
