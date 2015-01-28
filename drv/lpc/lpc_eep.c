/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "lpc_eep.h"
#include "lpc_core_private.h"
#include "lpc_config.h"
#include "../../userspace/direct.h"
#include "../../userspace/file.h"
#include "lpc_power.h"

#define IAP_LOCATION 0x1FFF1FF1

typedef void (*IAP)(unsigned int command[], unsigned int result[]);

static const IAP iap = (IAP)IAP_LOCATION;

#define IAP_CMD_WRITE_EEPROM                        61
#define IAP_CMD_READ_EEPROM                         62

static inline void lpc_eep_read(CORE* core, unsigned int size, HANDLE process)
{
    unsigned int chunk_size, processed;
    unsigned int req[5];
    unsigned int resp[1];
    uint8_t buf[LPC_EEPROM_BUF_SIZE];
    for (processed = 0; processed < size; processed += chunk_size)
    {
        chunk_size = size - processed;
        if (chunk_size > LPC_EEPROM_BUF_SIZE)
            chunk_size = LPC_EEPROM_BUF_SIZE;
        req[0] = IAP_CMD_READ_EEPROM;
        req[1] = core->eep.addr + processed;
        req[2] = (unsigned int)(&buf);
        req[3] = chunk_size;
        req[4] = lpc_power_get_system_clock_inside(core) / 1000;
        iap(req, resp);
        if (resp[0] != 0)
        {
            fread_complete(process, HAL_HANDLE(HAL_EEPROM, 0), INVALID_HANDLE, ERROR_INVALID_PARAMS);
            return;
        }
        if (!direct_write(process, buf, chunk_size))
        {
            fread_complete(process, HAL_HANDLE(HAL_EEPROM, 0), INVALID_HANDLE, get_last_error());
            return;
        }
    }
    fread_complete(process, HAL_HANDLE(HAL_EEPROM, 0), INVALID_HANDLE, processed);
}

static inline void lpc_eep_write(CORE* core, unsigned int size, HANDLE process)
{
    unsigned int chunk_size, processed;
    unsigned int req[5];
    unsigned int resp[1];
    uint8_t buf[LPC_EEPROM_BUF_SIZE];
    for (processed = 0; processed < size; processed += chunk_size)
    {
        chunk_size = size - processed;
        if (chunk_size > LPC_EEPROM_BUF_SIZE)
            chunk_size = LPC_EEPROM_BUF_SIZE;
        if (!direct_read(process, buf, chunk_size))
        {
            fwrite_complete(process, HAL_HANDLE(HAL_EEPROM, 0), INVALID_HANDLE, get_last_error());
            return;
        }
        req[0] = IAP_CMD_WRITE_EEPROM;
        req[1] = core->eep.addr + processed;
        req[2] = (unsigned int)(&buf);
        req[3] = chunk_size;
        req[4] = lpc_power_get_system_clock_inside(core) / 1000;
        iap(req, resp);
        if (resp[0] != 0)
        {
            fwrite_complete(process, HAL_HANDLE(HAL_EEPROM, 0), INVALID_HANDLE, ERROR_INVALID_PARAMS);
            return;
        }
    }
    fwrite_complete(process, HAL_HANDLE(HAL_EEPROM, 0), INVALID_HANDLE, processed);
}

bool lpc_eep_request(CORE* core, IPC* ipc)
{
    bool need_post = false;
    switch (ipc->cmd)
    {
    case IPC_SEEK:
        core->eep.addr = ipc->param2;
        need_post = true;
        break;
    case IPC_READ:
        lpc_eep_read(core, ipc->param3, ipc->process);
        break;
    case IPC_WRITE:
        lpc_eep_write(core, ipc->param3, ipc->process);
        break;
    }
    return need_post;
}

void lpc_eep_init(CORE* core)
{
    core->eep.addr = 0;
}
