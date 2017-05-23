/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef LPC_FLASH_H
#define LPC_FLASH_H

#include "lpc_exo.h"
#include <stdbool.h>
#include "../../userspace/types.h"

typedef struct {
    bool active;
    HANDLE user, activity;
} FLASH_DRV;

void lpc_flash_init(EXO* exo);
void lpc_flash_request(EXO* exo, IPC* ipc);

#endif // LPC_FLASH_H
