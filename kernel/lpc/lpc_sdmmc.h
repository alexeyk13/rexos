/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef LPC_SDMMC_H
#define LPC_SDMMC_H

#include "lpc_exo.h"
#include "../drv/sdmmcs.h"
#include "../../userspace/io.h"
#include "../../midware/crypto/sha1.h"

typedef enum {
    SDMMC_STATE_IDLE,
    SDMMC_STATE_READ,
    SDMMC_STATE_WRITE,
    SDMMC_STATE_VERIFY,
    SDMMC_STATE_WRITE_VERIFY
} SDMMC_STATE;

typedef struct _LPC_SDMMC_DESCR LPC_SDMMC_DESCR;

typedef struct  {
    SDMMCS sdmmcs;
    SDMMC_STATE state;
    struct _LPC_SDMMC_DESCR* descr;
    IO* io;
    HANDLE process, user, activity;
    unsigned int sector, total;
    uint8_t hash[SHA1_BLOCK_SIZE];
    bool active;
} SDMMC_DRV;

void lpc_sdmmc_init(EXO* exo);
void lpc_sdmmc_request(EXO* exo, IPC* ipc);

#endif // LPC_SDMMC_H
