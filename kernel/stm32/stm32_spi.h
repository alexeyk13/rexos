/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: Alexey E. Kramarenko (alexeyk13@yandex.ru)
*/

#ifndef STM32_SPI_H
#define STM32_SPI_H

#include "stm32_exo.h"
//#include "../../userspace/io.h"
#include "../../userspace/spi.h"
#include "../../userspace/process.h"
#include <stdbool.h>

typedef struct  {
    HANDLE process;
    unsigned int cs_pin;
    bool io_mode;
    IO* io;
    uint32_t cnt;
} SPI;

typedef struct  {
    SPI* spis[SPI_COUNT];
} SPI_DRV;

void stm32_spi_init(EXO* exo);
void stm32_spi_request(EXO* exo, IPC* ipc);

#endif // STM32_SPI_H
