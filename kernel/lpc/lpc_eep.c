/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "lpc_eep.h"
#include "lpc_exo_private.h"
#include "lpc_config.h"
#include "../../userspace/io.h"
#include "lpc_power.h"
#include "lpc_iap.h"
#include "../kerror.h"

#ifdef LPC18xx

#define EEP_BASE                                    0x20040000
#define EEP_SIZE                                    0x3f80
#define EEP_PAGE_SIZE                               0x80

#define EEP_CLK                                     1500000

#endif //LPC18xx

static inline void lpc_eep_read(EXO* exo, IPC* ipc)
{
    IO* io = (IO*)ipc->param2;
#ifdef LPC18xx
    if (ipc->param1 + ipc->param3 > EEP_SIZE)
    {
        kerror(ERROR_OUT_OF_RANGE);
        return;
    }
    io_data_write(io, (void*)(ipc->param1 + EEP_BASE), ipc->param3);
#else //LPC11Uxx
    LPC_IAP_TYPE iap;
    req[1] = ipc->param1;
    req[2] = (unsigned int)io_data(io);
    req[3] = ipc->param3;
    //EEPROM operates on M3 clock
    req[4] = lpc_power_get_core_clock_inside() / 1000;
    if (!lpc_iap(&iap, IAP_CMD_READ_EEPROM))
        return;
    io->data_size = ipc->param3;
#endif //LPC18xx
}

static inline void lpc_eep_write(EXO* exo, IPC* ipc)
{
    IO* io = (IO*)ipc->param2;
#ifdef LPC18xx
    unsigned int addr, count, processed, cur, i;
    if (ipc->param1 + io->data_size > EEP_SIZE)
    {
        kerror(ERROR_OUT_OF_RANGE);
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
            __NOP();
        }
    }
#else //LPC11Uxx
    LPC_IAP_TYPE iap;
    req[1] = ipc->param1;
    req[2] = (unsigned int)io_data(io);
    req[3] = io->data_size;
    //EEPROM operates on M3 clock
    req[4] = lpc_power_get_core_clock_inside() / 1000;
    iap(req, resp);
    if (!lpc_iap(&iap, IAP_CMD_WRITE_EEPROM))
        return;
#endif //LPC18xx
}

#ifdef LPC18xx
void lpc_eep_init(EXO* exo)
{
    unsigned int i;
    LPC_EEPROM->PWRDWN |= LPC_EEPROM_PWRDWN_Msk;
    //EEPROM operates on M3 clock
    LPC_EEPROM->CLKDIV = lpc_power_get_core_clock_inside() / EEP_CLK - 1;
    for (i = 0; i < 100; ++i)
        __NOP();
    LPC_EEPROM->PWRDWN &= ~LPC_EEPROM_PWRDWN_Msk;
}
#endif //LPC18xx

void lpc_eep_request(EXO* exo, IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_READ:
        lpc_eep_read(exo, ipc);
        break;
    case IPC_WRITE:
        lpc_eep_write(exo, ipc);
        break;
    default:
        kerror(ERROR_NOT_SUPPORTED);
    }
}
