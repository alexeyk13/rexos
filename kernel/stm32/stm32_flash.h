/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: Alexey E. Kramarenko (alexeyk13@yandex.ru)
*/

#ifndef STM32_FLASH_H
#define STM32_FLASH_H

#include "stm32_exo.h"
#include <stdbool.h>
#include "../../userspace/types.h"

typedef struct {
    bool active;
    HANDLE user, activity;
} FLASH_DRV;

void stm32_flash_init(EXO* exo);
void stm32_flash_request(EXO* exo, IPC* ipc);

#endif // STM32_FLASH_H
