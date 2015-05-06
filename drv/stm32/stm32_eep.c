/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "stm32_eep.h"
#include "stm32_core_private.h"
#include "stm32_config.h"
#include "../../userspace/direct.h"
#include "../../userspace/file.h"
#include "string.h"

#define PEKEY1                          0x89ABCDEF
#define PEKEY2                          0x02030405

#define EEP_BASE                        0x08080000
#define EEP_SIZE                        0x800

static inline void stm32_eep_seek(CORE* core, unsigned int addr)
{
    if (addr < EEP_BASE || addr >= EEP_BASE + EEP_SIZE)
    {
        error(ERROR_OUT_OF_RANGE);
        return;
    }
    core->eep.addr = addr;
}

static inline void stm32_eep_read(CORE* core, unsigned int size, HANDLE process)
{
    unsigned int chunk_size, processed;
    uint32_t buf[STM32_EEPROM_BUF_SIZE / 4];
    if (core->eep.addr + size > EEP_BASE + EEP_SIZE)
    {
        error(ERROR_OUT_OF_RANGE);
        return;
    }
    for (processed = 0; processed < size; processed += chunk_size)
    {
        chunk_size = size - processed;
        if (chunk_size > STM32_EEPROM_BUF_SIZE)
            chunk_size = STM32_EEPROM_BUF_SIZE;
        memcpy(buf, (void*)(core->eep.addr + processed), chunk_size);
        if (!direct_write(process, buf, chunk_size))
        {
            fread_complete(process, HAL_HANDLE(HAL_EEPROM, 0), INVALID_HANDLE, get_last_error());
            return;
        }
    }
    fread_complete(process, HAL_HANDLE(HAL_EEPROM, 0), INVALID_HANDLE, processed);
}

static inline void stm32_eep_write(CORE* core, unsigned int size, HANDLE process)
{
    unsigned int chunk_size, processed, i;
    uint8_t buf[STM32_EEPROM_BUF_SIZE];
    if (core->eep.addr + size > EEP_BASE + EEP_SIZE)
    {
        error(ERROR_OUT_OF_RANGE);
        return;
    }
    FLASH->PECR &= ~(FLASH_PECR_FPRG | FLASH_PECR_ERASE | FLASH_PECR_FTDW | FLASH_PECR_PROG);
    FLASH->PECR |= FLASH_PECR_DATA;
    for (processed = 0; processed < size; processed += chunk_size)
    {
        chunk_size = size - processed;
        if (chunk_size > STM32_EEPROM_BUF_SIZE)
            chunk_size = STM32_EEPROM_BUF_SIZE;
        if (!direct_read(process, buf, chunk_size))
        {
            fwrite_complete(process, HAL_HANDLE(HAL_EEPROM, 0), INVALID_HANDLE, get_last_error());
            return;
        }

        for (i = 0; i < chunk_size; i += sizeof(uint32_t))
        {
            while (FLASH->SR & FLASH_SR_BSY) {}
            *(uint32_t*)(core->eep.addr + processed + i) = *((uint32_t*)(buf + i));
        }
        if (FLASH->SR & (FLASH_SR_FWWERR | FLASH_SR_NOTZEROERR | FLASH_SR_SIZERR | FLASH_SR_PGAERR))
        {
            FLASH->SR |= FLASH_SR_FWWERR | FLASH_SR_NOTZEROERR | FLASH_SR_SIZERR | FLASH_SR_PGAERR;
            fwrite_complete(process, HAL_HANDLE(HAL_EEPROM, 0), INVALID_HANDLE, ERROR_HARDWARE);
            return;
        }
    }
    fwrite_complete(process, HAL_HANDLE(HAL_EEPROM, 0), INVALID_HANDLE, processed);
}

bool stm32_eep_request(CORE* core, IPC* ipc)
{
    bool need_post = false;
    switch (ipc->cmd)
    {
    case IPC_SEEK:
        stm32_eep_seek(core, ipc->param2);
        need_post = true;
        break;
    case IPC_READ:
        stm32_eep_read(core, ipc->param3, ipc->process);
        break;
    case IPC_WRITE:
        stm32_eep_write(core, ipc->param3, ipc->process);
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
