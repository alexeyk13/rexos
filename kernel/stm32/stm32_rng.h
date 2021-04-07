/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: Alexey E. Kramarenko (alexeyk13@yandex.ru)
*/

#ifndef STM32_RNG_H
#define STM32_RNG_H

#include "stm32_exo.h"
#include "io.h"

typedef struct  {
    IO* io;
    uint32_t readed;
    HANDLE process;
} RNG_DRV;

void stm32_rng_init(EXO* exo);
void stm32_rng_request(EXO* exo, IPC* ipc);

#endif // STM32_RNG_H
