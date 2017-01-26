/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef STM32_EEP_H
#define STM32_EEP_H

#include "stm32_core.h"

void stm32_eep_init(CORE* core);
bool stm32_eep_request(CORE* core, IPC* ipc);

#endif // STM32_EEP_H
