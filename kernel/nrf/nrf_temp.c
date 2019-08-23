/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: RL (jam_roma@yahoo.com)
*/


#include "nrf_temp.h"
#include "../../userspace/sys.h"
#include "../../userspace/io.h"
#include "../kerror.h"
#include "nrf_exo_private.h"
#include "sys_config.h"

static inline uint32_t nrf_temp_read(EXO* exo)
{
    /* start measurement */
    NRF_TEMP->TASKS_START = 1;
    /* wait measurement */
    while(NRF_TEMP->EVENTS_DATARDY == 0);
    /* get measured value */
    return ((NRF_TEMP->TEMP) >> 2);
}

void nrf_temp_request(EXO* exo, IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
        case IPC_READ:
            ipc->param2 = nrf_temp_read(exo);
            break;
        default:
            kerror(ERROR_NOT_SUPPORTED);
            break;
    }
}
