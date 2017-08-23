/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "stm32_eep.h"
#include "stm32_exo_private.h"
#include "stm32_config.h"
#include "../../userspace/io.h"
#include "../../userspace/stm32/stm32_driver.h"
#include "string.h"

#define PEKEY1                          0x89ABCDEF
#define PEKEY2                          0x02030405

#define EEP_BASE                        0x08080000
#define EEP_SIZE                        0x800

static inline void stm32_eep_read(EXO* exo, IPC* ipc)
{
    if (ipc->param1 + ipc->param3 > EEP_SIZE)
    {
        ipc_post_ex(ipc, ERROR_OUT_OF_RANGE);
        return;
    }
    io_data_write((IO*)ipc->param2, (void*)(ipc->param1 + EEP_BASE), ipc->param3);
    ipc_post_ex(ipc, io->data_size);
}

static inline void stm32_eep_write(EXO* exo, IPC* ipc)
{
    unsigned int i;
    IO* io = (IO*)ipc->param2;
    if (ipc->param1 + io->data_size > EEP_BASE + EEP_SIZE)
    {
        ipc_post_ex(ipc, ERROR_OUT_OF_RANGE);
        return;
    }
    FLASH->PECR &= ~(FLASH_PECR_FPRG | FLASH_PECR_ERASE | FLASH_PECR_FTDW | FLASH_PECR_PROG);
    FLASH->PECR |= FLASH_PECR_DATA;
    for (i = 0; i < io->data_size; i += sizeof(uint32_t))
    {
        while (FLASH->SR & FLASH_SR_BSY) {}

        *(uint32_t*)(EEP_BASE + ipc->param1 + i) = *((uint32_t*)(io_data(io) + i));

        if (FLASH->SR & (FLASH_SR_FWWERR | FLASH_SR_NOTZEROERR | FLASH_SR_SIZERR | FLASH_SR_PGAERR))
        {
            FLASH->SR |= FLASH_SR_FWWERR | FLASH_SR_NOTZEROERR | FLASH_SR_SIZERR | FLASH_SR_PGAERR;
            ipc_post_ex(ipc, ERROR_HARDWARE);
            return;
        }
    }
    ipc_post_ex(ipc, io->data_size);
}

bool stm32_eep_request(EXO* exo, IPC* ipc)
{
    bool need_post = false;
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_READ:
        stm32_eep_read(exo, ipc);
        break;
    case IPC_WRITE:
        stm32_eep_write(exo, ipc);
        break;
    }
    return need_post;
}

void stm32_eep_init(EXO* exo)
{
    //unlock EEP memore for write access
    if (FLASH->PECR & FLASH_PECR_PELOCK)
    {
        __disable_irq();
        FLASH->PEKEYR = PEKEY1;
        FLASH->PEKEYR = PEKEY2;
        __enable_irq();
    }
}
