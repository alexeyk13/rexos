/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef STM32_POWER_H
#define STM32_POWER_H

#include "stm32_core.h"

#if defined(STM32F10X_CL)
#define PLL_MUL_6_5                             15
#define PLL2_MUL_20                             15
#endif

typedef enum {
    STM32_CLOCK_CORE,
    STM32_CLOCK_AHB,
    STM32_CLOCK_APB1,
    STM32_CLOCK_APB2,
    STM32_CLOCK_ADC
} STM32_POWER_CLOCKS;

typedef enum {
    DMA_1 = 0,
    DMA_2
} STM32_DMA;

unsigned int get_clock(STM32_POWER_CLOCKS type);
//params are product line specific
void update_clock(int param1, int param2, int param3);
#if defined(STM32F1)
void dma_on(CORE* core, unsigned int index);
void dma_off(CORE* core, unsigned int index);
#endif
RESET_REASON get_reset_reason(CORE* core);
#if defined(STM32F1)
void stm32_usb_power_on();
void stm32_usb_power_off();
#endif

#if (SYS_INFO)
void stm32_power_info(CORE* core);
#endif

void stm32_power_init(CORE* core);

#endif // STM32_POWER_H
