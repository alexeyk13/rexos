/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2018, Alexey Kramarenko
    All rights reserved.

    author: Alexey E. Kramarenko (alexeyk13@yandex.ru)
*/

#ifndef STM32_SDMMC_H
#define STM32_SDMMC_H

#include "stm32_exo.h"
#include "../../userspace/io.h"
#include "../../midware/crypto/sha1.h"

typedef enum {
    SDMMC_STATE_IDLE,
    SDMMC_STATE_CMD,
    SDMMC_STATE_CMD_DATA,
    SDMMC_STATE_DATA,
    SDMMC_STATE_DATA_ABORT,
    SDMMC_STATE_VERIFY,
    SDMMC_STATE_WRITE_VERIFY
} SDMMC_STATE;


typedef struct  {
    SDMMC_STATE state;
    IO* io;
    HANDLE process, user, activity;
    unsigned int sector, total;
    uint8_t hash[SHA1_BLOCK_SIZE];
    uint32_t data_ipc;
    bool active;
} SDMMC_DRV;

void stm32_sdmmc_init(EXO* exo);
void stm32_sdmmc_request(EXO* exo, IPC* ipc);

#endif // STM32_SDMMC_H
