/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
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
} SPI;

typedef struct  {
    SPI* spis[SPI_COUNT];
} SPI_DRV;

void stm32_spi_init(EXO* exo);
void stm32_spi_request(EXO* exo, IPC* ipc);

#endif // STM32_SPI_H
