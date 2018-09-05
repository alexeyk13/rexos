/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2018, Alexey Kramarenko
    All rights reserved.
*/

#ifndef STM32_EEP_H
#define STM32_EEP_H

#include "stm32_exo.h"

void stm32_eep_init(EXO* exo);
bool stm32_eep_request(EXO* exo, IPC* ipc);

#endif // STM32_EEP_H
