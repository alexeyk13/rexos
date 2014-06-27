/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef HW_CONFIG_H
#define HW_CONFIG_H

/*
        hardware-specific config
 */

#include "arch.h"

#ifdef _STM

#if defined(STM32F1)
#include "hw_config_stm32f1.h"
#elif defined(STM32F2)
#include "hw_config_stm32f2.h"
#elif defined(STM32F4)
#include "hw_config_stm32f4.h"
#endif

#elif defined(CUSTOM_ARCH)
#include "hw_config_custom.h"
#endif

#endif // HW_CONFIG_H
