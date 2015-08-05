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

#define get_core_clock                              lpc_power_get_core_clock_inside

#ifdef LPC18xx

#define EEP_BASE                                    0x20040000
#define EEP_SIZE                                    0x3f80
#define EEP_PAGE_SIZE                               0x80

#define EEP_CLK                                     1500000

#else //LPC11Uxx

#define IAP_LOCATION 0x1FFF1FF1

typedef void (*IAP)(unsigned int command[], unsigned int result[]);

static const IAP iap = (IAP)IAP_LOCATION;

#define IAP_CMD_WRITE_EEPROM                        61
#define IAP_CMD_READ_EEPROM                         62

#endif //LPC18xx

static inline void lpc_eep_read(CORE* core, IPC* ipc)
{
    IO* io = (IO*)ipc->param2;
#ifdef LPC18xx
    if (ipc->param1 + ipc->param3 > EEP_SIZE)
    {
        ipc_post_ex(ipc, ERROR_OUT_OF_RANGE);
        return;
    }
    io_data_write(io, (void*)(ipc->param1 + EEP_BASE), ipc->param3);
#else //LPC11Uxx
    unsigned int req[5];
    unsigned int resp[1];
    req[0] = IAP_CMD_READ_EEPROM;
    req[1] = ipc->param1;
    req[2] = (unsigned int)io_data(io);
    req[3] = ipc->param3;
    req[4] = lpc_power_get_core_clock_inside(core) / 1000;
    iap(req, resp);
    if (resp[0] != 0)
    {
        ipc_post_ex(ipc, ERROR_INVALID_PARAMS);
        return;
    }
    io->data_size = ipc->param3;
#endif //LPC18xx
    ipc_post_ex(ipc, io->data_size);
}

static inline void lpc_eep_write(CORE* core, IPC* ipc)
{
    IO* io = (IO*)ipc->param2;
#ifdef LPC18xx
    unsigned int addr, count, processed, cur, i;
    if (ipc->param1 + io->data_size > EEP_SIZE)
    {
        ipc_post_ex(ipc, ERROR_OUT_OF_RANGE);
        return;
    }
    for(count = (io->data_size + 3) >> 2, processed = 0, addr = (ipc->param1 + EEP_BASE) & ~3; count; count -= cur, processed += (cur << 2), addr += (cur << 2))
    {
        cur = (EEP_PAGE_SIZE - (addr & (EEP_PAGE_SIZE - 1))) >> 2;
        if (cur > count)
            cur = count;
        for (i = 0; i < cur; ++i)
            ((uint32_t*)addr)[i] = ((uint32_t*)(io_data(io) + processed))[i];
        LPC_EEPROM->INTSTATCLR = LPC_EEPROM_INTSTATCLR_PROG_CLR_ST_Msk;
        LPC_EEPROM->CMD = LPC_EEPROM_CMD_ERASE_PROGRAM;
        while ((LPC_EEPROM->INTSTAT & LPC_EEPROM_INTSTAT_END_OF_PROG_Msk) == 0)
        {
            sleep_ms(1);
        }
    }
#else //LPC11Uxx
    unsigned int req[5];
    unsigned int resp[1];
    req[0] = IAP_CMD_WRITE_EEPROM;
    req[1] = ipc->param1;
    req[2] = (unsigned int)io_data(io);
    req[3] = io->data_size;
    req[4] = lpc_power_get_core_clock_inside(core) / 1000;
    iap(req, resp);
    if (resp[0] != 0)
    {
        ipc_post_ex(ipc, ERROR_INVALID_PARAMS);
        return;
    }
#endif //LPC18xx
    ipc_post_ex(ipc, io->data_size);
}

#ifdef LPC18xx
void lpc_eep_init(CORE* core)
{
    LPC_EEPROM->PWRDWN |= LPC_EEPROM_PWRDWN_Msk;
    LPC_EEPROM->CLKDIV = get_core_clock(core) / EEP_CLK - 1;
    sleep_us(100);
    LPC_EEPROM->PWRDWN &= ~LPC_EEPROM_PWRDWN_Msk;
}
#endif //LPC18xx

bool lpc_eep_request(CORE* core, IPC* ipc)
{
    bool need_post = false;
    switch (HAL_ITEM(ipc->cmd))
    {
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
