/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: Alexey E. Kramarenko (alexeyk13@yandex.ru)
*/

#ifndef STM32_EEP_H
#define STM32_EEP_H

#include "stm32_exo.h"

void stm32_eep_init(EXO* exo);
bool stm32_eep_request(EXO* exo, IPC* ipc);

#endif // STM32_EEP_H
