/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "lpc_eep.h"
#include "lpc_core_private.h"
#include "lpc_config.h"
#include "../../userspace/io.h"
#include "lpc_power.h"

#define IAP_LOCATION 0x1FFF1FF1

typedef void (*IAP)(unsigned int command[], unsigned int result[]);

static const IAP iap = (IAP)IAP_LOCATION;

#define IAP_CMD_WRITE_EEPROM                        61
#define IAP_CMD_READ_EEPROM                         62

static inline void lpc_eep_read(CORE* core, IPC* ipc)
{
    unsigned int req[5];
    unsigned int resp[1];
    IO* io = (IO*)ipc->param2;
    req[0] = IAP_CMD_READ_EEPROM;
    req[1] = core->eep.addr;
    req[2] = (unsigned int)io_data(io);
    req[3] = ipc->param3;
    req[4] = lpc_power_get_system_clock_inside(core) / 1000;
    iap(req, resp);
    if (resp[0] != 0)
    {
        io_send_error(ipc, ERROR_INVALID_PARAMS);
        return;
    }
    io->data_size = ipc->param1;
    io_send(ipc);
}

static inline void lpc_eep_write(CORE* core, IPC* ipc)
{
    unsigned int req[5];
    unsigned int resp[1];
    IO* io = (IO*)ipc->param2;
    req[0] = IAP_CMD_WRITE_EEPROM;
    req[1] = core->eep.addr;
    req[2] = (unsigned int)io_data(io);
    req[3] = io->data_size;
    req[4] = lpc_power_get_system_clock_inside(core) / 1000;
    iap(req, resp);
    if (resp[0] != 0)
    {
        io_send_error(ipc, ERROR_INVALID_PARAMS);
        return;
    }
    io_send(ipc);
}

bool lpc_eep_request(CORE* core, IPC* ipc)
{
    bool need_post = false;
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_SEEK:
        core->eep.addr = ipc->param2;
        need_post = true;
        break;
    case IPC_READ:
        lpc_eep_read(core, ipc);
        break;
    case IPC_WRITE:
        lpc_eep_write(core, ipc);
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
        need_post = true;
    }
    return need_post;
}

void lpc_eep_init(CORE* core)
{
    core->eep.addr = 0;
}
