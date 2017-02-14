/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "ti_rf.h"
#include "ti_exo_private.h"
#include "../../userspace/process.h"
#include "../../userspace/error.h"

void ti_rf_init(EXO* exo)
{
    exo->rf.active = false;
}

static inline void ti_rf_open(EXO* exo)
{
    //TODO:
}

static inline void ti_rf_close(EXO* exo)
{
    //TODO:
}

void ti_rf_request(EXO* exo, IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_OPEN:
        ti_rf_open(exo);
        break;
    case IPC_CLOSE:
        ti_rf_close(exo);
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
    }
}
