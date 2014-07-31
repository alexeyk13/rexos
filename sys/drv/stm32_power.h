/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef STM32_POWER_H
#define STM32_POWER_H

#include "stm32_core.h"
#include "sys_config.h"

//special values for STM32F1_CL
#define PLL_MUL_6_5                             15
#define PLL2_MUL_20                             15

typedef enum {
    STM32_CLOCK_CORE,
    STM32_CLOCK_AHB,
    STM32_CLOCK_APB1,
    STM32_CLOCK_APB2,
    STM32_CLOCK_ADC
} STM32_POWER_CLOCKS;

typedef enum {
    RESET_REASON_UNKNOWN    = 0,
    RESET_REASON_LOW_POWER,
    RESET_REASON_WATCHDOG,
    RESET_REASON_SOFTWARE,
    RESET_REASON_POWERON,
    RESET_REASON_PIN_RST
} RESET_REASON;

unsigned int get_clock(STM32_POWER_CLOCKS type);
//params are product line specific
void update_clock(int param1, int param2, int param3);
void backup_on(CORE* core);
void backup_off(CORE* core);
void backup_write_enable(CORE* core);
void backup_write_protect(CORE *core);
RESET_REASON get_reset_reason();
#if (ADC_DRIVER)
void stm32_adc_on();
void stm32_adc_off();
#endif

#if (SYS_INFO)
void stm32_power_info();
#endif

void stm32_power_init(CORE* core);

#endif // STM32_POWER_H
