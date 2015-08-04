/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "stm32_eep.h"
#include "stm32_core_private.h"
#include "stm32_config.h"
#include "../../userspace/io.h"
#include "../../userspace/stm32_driver.h"
#include "string.h"

#define PEKEY1                          0x89ABCDEF
#define PEKEY2                          0x02030405

#define EEP_BASE                        0x08080000
#define EEP_SIZE                        0x800

static inline void stm32_eep_seek(CORE* core, unsigned int addr)
{
    addr += EEP_BASE;
    if (addr < EEP_BASE || addr >= EEP_BASE + EEP_SIZE)
    {
        error(ERROR_OUT_OF_RANGE);
        return;
    }
    core->eep.addr = addr;
}

static inline void stm32_eep_read(CORE* core, IPC* ipc)
{
    if (core->eep.addr + ipc->param3 > EEP_BASE + EEP_SIZE)
    {
        ipc_post_ex(ipc, ERROR_OUT_OF_RANGE);
        return;
    }
    io_data_write((IO*)ipc->param2, (void*)core->eep.addr, ipc->param3);
    ipc_post(ipc);
}

static inline void stm32_eep_write(CORE* core, IPC* ipc)
{
    unsigned int i;
    IO* io = (IO*)ipc->param2;
    if (core->eep.addr + io->data_size > EEP_BASE + EEP_SIZE)
    {
        ipc_post_ex(ipc, ERROR_OUT_OF_RANGE);
        return;
    }
    FLASH->PECR &= ~(FLASH_PECR_FPRG | FLASH_PECR_ERASE | FLASH_PECR_FTDW | FLASH_PECR_PROG);
    FLASH->PECR |= FLASH_PECR_DATA;
    for (i = 0; i < io->data_size; i += sizeof(uint32_t))
    {
        while (FLASH->SR & FLASH_SR_BSY) {}

        *(uint32_t*)(core->eep.addr + i) = *((uint32_t*)(io_data(io) + i));

        if (FLASH->SR & (FLASH_SR_FWWERR | FLASH_SR_NOTZEROERR | FLASH_SR_SIZERR | FLASH_SR_PGAERR))
        {
            FLASH->SR |= FLASH_SR_FWWERR | FLASH_SR_NOTZEROERR | FLASH_SR_SIZERR | FLASH_SR_PGAERR;
            ipc_post_ex(ipc, ERROR_HARDWARE);
            return;
        }
    }
    ipc->param3 = io->data_size;
    ipc_post(ipc);
}

bool stm32_eep_request(CORE* core, IPC* ipc)
{
    bool need_post = false;
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_SEEK:
        stm32_eep_seek(core, ipc->param2);
        need_post = true;
        break;
    case IPC_READ:
        stm32_eep_read(core, ipc);
        break;
    case IPC_WRITE:
        stm32_eep_write(core, ipc);
        break;
    }
    return need_post;
}

void stm32_eep_init(CORE* core)
{
    //unlock EEP memore for write access
    if (FLASH->PECR & FLASH_PECR_PELOCK)
    {
        __disable_irq();
        FLASH->PEKEYR = PEKEY1;
        FLASH->PEKEYR = PEKEY2;
        __enable_irq();
    }
    core->eep.addr = EEP_BASE;
}
