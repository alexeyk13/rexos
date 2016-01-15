/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2016, Alexey Kramarenko
    All rights reserved.
*/

#include "lpc_sd.h"

void lpc_sd_init(CORE* core)
{
    //TODO:
}

void lpc_sd_open(CORE* core)
{
    //TODO:
    //TODO: pin configuration
    //TODO: clock configuration
    //TODO: reset
    //TODO: setup
    //TODO: dma setup
    //TODO: interrupt setup
}

void lpc_sd_request(CORE* core, IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_OPEN:
        lpc_sd_open(core);
        break;
    case IPC_CLOSE:
        //TODO:
    case IPC_READ:
        //TODO:
    case IPC_WRITE:
        //TODO:
    default:
        error(ERROR_NOT_SUPPORTED);
    }
}
