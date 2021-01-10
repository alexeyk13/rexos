/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2018, Alexey Kramarenko
    All rights reserved.

    author: Alexey E. Kramarenko (alexeyk13@yandex.ru)
*/

#include "sys_config.h"
#include "object.h"
#include "sdmmc.h"
#include "io.h"
#include <string.h>

extern void sdmmcs_main();

HANDLE sdmmc_create(uint32_t process_size, uint32_t priority)
{
    char name[] = "SDMMC Server";
    REX rex;
    rex.name = name;
    rex.size = process_size;
    rex.priority = priority;
    rex.flags = PROCESS_FLAGS_ACTIVE;
    rex.fn = sdmmcs_main;
    return process_create(&rex);
}

bool sdmmc_open(HANDLE sdmmc, uint32_t user)
{
    return get_size(sdmmc, HAL_REQ(HAL_SDMMC, IPC_OPEN), user, 0, 0) == 0;
}

void sdmmc_close(HANDLE sdmmc)
{
    ack(sdmmc, HAL_REQ(HAL_SDMMC, IPC_CLOSE), 0, 0, 0);
}

/*
void canopen_sdo_init_req(HANDLE co, CO_SDO_REQ* req)
{
    IPC ipc;
    ipc.process = co;
    ipc.cmd = HAL_CMD(HAL_CANOPEN, IPC_CO_SDO_REQUEST);
    ipc.param1 = (req->id << 24) | (req->subindex << 16) | req->index;
    ipc.param2 = req->type;
    ipc.param3 = req->data;
    ipc_post(&ipc);
}

*/
