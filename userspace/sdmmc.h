/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2012, RExOS team
    All rights reserved.

    author: zurabob (zurabob@list.ru)
*/

#ifndef SDMMC_H
#define SDMMC_H

#include <stdint.h>
#include "storage.h"

typedef enum {
    SDMMC_CMD = STORAGE_IPC_MAX,
    SDMMC_CMD_READ,
    SDMMC_CMD_WRITE,
    SDMMC_SET_CLOCK,
    SDMMC_SET_DATA_WIDTH,
} SDMMC_IPCS;


//--------------------------------
HANDLE sdmmc_create(uint32_t process_size, uint32_t priority);
bool sdmmc_open(HANDLE sdmmc, uint32_t user);
void sdmmc_close(HANDLE sdmmc);

#endif // SDMMC_H
