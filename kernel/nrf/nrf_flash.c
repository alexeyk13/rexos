/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: RL (jam_roma@yahoo.com)
*/

#include <string.h>
#include "nrf_flash.h"
#include "nrf_exo_private.h"
#include "nrf_config.h"
#include "../../userspace/storage.h"
#include "../../userspace/stdlib.h"
#include "../../userspace/stdio.h"
#include "../kerror.h"
#include "../kstdlib.h"
#include "../kipc.h"
#include "../dbg.h"

#define LOGICAL_SECTOR_SIZE             512
#define FLASH_PAGE_SIZE_WORDS         ((flash_page_size()) / sizeof(uint32_t))

void nrf_flash_init(EXO* exo)
{
    // init
    exo->flash.active = false;
    exo->flash.user = INVALID_HANDLE;
    exo->flash.activity = INVALID_HANDLE;
    exo->flash.page = NULL;
}

unsigned int flash_page_size()
{
    return (NRF_FICR->CODEPAGESIZE);
}

/* return total size in bytes */
unsigned int flash_total_size()
{
    return ((NRF_FICR->CODEPAGESIZE) * (NRF_FICR->CODESIZE));
}

static inline bool nrf_flash_is_sector_empty(unsigned int* addr)
{
    unsigned int i;
    for (i = 0; i < FLASH_PAGE_SIZE_WORDS; ++i)
        if (addr[i] != 0xffffffff)
            return false;
    return true;
}

static void nrf_flash_activity(EXO* exo)
{
    if ((exo->flash.activity != INVALID_HANDLE))
    {
        ipc_post_inline(exo->flash.activity, HAL_CMD(HAL_FLASH, STORAGE_NOTIFY_ACTIVITY), exo->flash.user, 0, 0);
        exo->flash.activity = INVALID_HANDLE;
    }
}

static inline void flash_erase_page(uint32_t addr)
{
    /* Turn on flash erase enable and wait until the NVMC is ready */
    NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Een << NVMC_CONFIG_WEN_Pos);

    while (NRF_NVMC->READY == NVMC_READY_READY_Busy)
    {
        /* wait */
    }

    /* erase page */
    NRF_NVMC->ERASEPAGE = addr;

    while (NRF_NVMC->READY == NVMC_READY_READY_Busy)
    {
        /* wait */
    }

    /* turn off flash erase enable and wait until the NVMC is ready */
    NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos);

    while (NRF_NVMC->READY == NVMC_READY_READY_Busy)
    {
        /* do nothing */
    }
}

static inline void flash_write_word(uint32_t addr, uint32_t val)
{
    /* turn on flash write enable and wait until the NVMC is ready */
    NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos);

    while (NRF_NVMC->READY == NVMC_READY_READY_Busy)
    {
        /* wait */
    }

    *((uint32_t*)addr) = val;
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy)
    {
        /* wait */
    }

    /* turn off flash write enable and wait until the NVMC is ready */
    NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos);
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy)
    {
        /* wait */
    }
}

static inline void nrf_flash_page_read(EXO* exo, unsigned int offs, IO* io, unsigned int size)
{
    /* if storage not active */
    if (!exo->flash.active)
    {
        kerror(ERROR_NOT_CONFIGURED);
        return;
    }

    if ((offs + size) > flash_total_size())
    {
        kerror(ERROR_OUT_OF_RANGE);
        return;
    }

    io_data_append(io, (void*)offs, size);
}

static inline bool nrf_flash_page_erase(EXO* exo, unsigned int offs)
{
    /* if storage not active */
    if (!exo->flash.active)
    {
        kerror(ERROR_NOT_CONFIGURED);
        return false;
    }

    /* verify page aligment */
    if((offs % flash_page_size()) != 0)
    {
        kerror(ERROR_INVALID_ALIGN);
        return false;
    }

    /* erase page */
    flash_erase_page(offs);
    return true;
}

static inline void nrf_flash_page_write(EXO* exo, unsigned int offs, uint8_t* buf, unsigned int size)
{
    unsigned int i = 0;
    unsigned int page_size = flash_page_size();
    /* if storage not active */
    if (!exo->flash.active)
    {
        kerror(ERROR_NOT_CONFIGURED);
        return;
    }

    if(size != page_size)
    {
        kerror(ERROR_INVALID_PARAMS);
        return;
    }

    if(offs > flash_total_size())
    {
        kerror(ERROR_OUT_OF_RANGE);
        return;
    }

    /* dump page from flash in system buffer */
    memcpy(exo->flash.page, buf, size);

    /* erase page */
    if(!nrf_flash_page_erase(exo, offs))
        return;

    /* write new data by words */
    for(i = 0; i < page_size / sizeof(uint32_t); i++, offs += sizeof(uint32_t))
        flash_write_word(offs, *((uint32_t*)(exo->flash.page + (i * sizeof(uint32_t)))));
}

static inline void nrf_flash_read(EXO* exo,  HANDLE user, IO* io, unsigned int size)
{
    unsigned int base_addr;
    STORAGE_STACK* stack = io_stack(io);
    io_pop(io, sizeof(STORAGE_STACK));

    if (!exo->flash.active)
    {
        kerror(ERROR_NOT_CONFIGURED);
        return;
    }

    if ((user != exo->flash.user) || (size == 0))
    {
        kerror(ERROR_INVALID_PARAMS);
        return;
    }

    if((stack->flags & STORAGE_OFFSET_INSTED_SECTOR) == 0)
        stack->sector *= LOGICAL_SECTOR_SIZE;

    base_addr = stack->sector + exo->flash.offset;
    if (base_addr + size > FLASH_SIZE)
    {
        kerror(ERROR_INVALID_PARAMS);
        return;
    }
    if (!(stack->flags & STORAGE_FLAG_IGNORE_ACTIVITY_ON_REQUEST))
        nrf_flash_activity(exo);
    memcpy(io_data(io), (void*)base_addr, size);
    io->data_size = size;
}

static inline void nrf_flash_write(EXO* exo, HANDLE user, IO* io, unsigned int size)
{
    unsigned int page_size = flash_page_size();
    unsigned int base_addr, i, start_addr, sector_start_addr, data_size_cur;
    void* buf;
    STORAGE_STACK* stack = io_stack(io);
    io_pop(io, sizeof(STORAGE_STACK));

    /* if storage not active */
    if (!exo->flash.active)
    {
        kerror(ERROR_NOT_CONFIGURED);
        return;
    }
    if ((stack->flags & STORAGE_MASK_MODE) == STORAGE_FLAG_ERASE_ONLY)
        size *= LOGICAL_SECTOR_SIZE;
    if ((user != exo->flash.user) || (size == 0))
    {
        kerror(ERROR_INVALID_PARAMS);
        return;
    }
    if((stack->flags & STORAGE_OFFSET_INSTED_SECTOR) == 0)
        stack->sector *= LOGICAL_SECTOR_SIZE;
    base_addr = stack->sector + exo->flash.offset;
    if (base_addr + size > FLASH_SIZE)
    {
        kerror(ERROR_INVALID_PARAMS);
        return;
    }

    if (!(stack->flags & STORAGE_FLAG_IGNORE_ACTIVITY_ON_REQUEST))
        nrf_flash_activity(exo);

    if (stack->flags & (STORAGE_FLAG_ERASE_ONLY | STORAGE_OFFSET_INSTED_SECTOR))
    {
        unsigned int flg = stack->flags;

        stack->flags &= STORAGE_FLAG_ERASE_ONLY | STORAGE_OFFSET_INSTED_SECTOR;
        size /= sizeof(uint32_t);

        nrf_flash_page_write(exo, base_addr, io_data(io), size);

        if(flg & STORAGE_FLAG_VERIFY)
        {
            if (memcmp(io_data(io), (void*)base_addr, size) != 0)
                kerror(ERROR_CRC);
        }
        return;
    }

    for (i = 0; i < size; i += data_size_cur)
    {
        start_addr = base_addr + i;
        sector_start_addr = start_addr & ~(page_size - 1);
        data_size_cur = page_size;
        if (data_size_cur > size - i)
            data_size_cur = size - i;

        /* unaligned from start of flash page */
        if (start_addr != sector_start_addr)
        {
            memcpy(exo->flash.page, (void*)sector_start_addr, start_addr - sector_start_addr);
            if (data_size_cur + (start_addr - sector_start_addr) > page_size)
                data_size_cur = page_size - (start_addr - sector_start_addr);
        }

        /* unaligned tail at sector end */
        if (start_addr - sector_start_addr + data_size_cur < page_size)
        {
            memcpy(exo->flash.page + (start_addr - sector_start_addr) + data_size_cur,
                   (void*)(start_addr + data_size_cur),
                   page_size - (start_addr - sector_start_addr) - data_size_cur);
        }

        if (data_size_cur == page_size)
        {
            if ((stack->flags & STORAGE_MASK_MODE) == STORAGE_FLAG_ERASE_ONLY)
            {
                /* just erase page, that's all */
                flash_erase_page(sector_start_addr);
                /* go back */
                return;
            }
            else
                buf = (uint8_t*)io_data(io) + i;
        }
        else
        {
            /* copy data if data is not aligned in flash sector */
            memcpy(exo->flash.page + (start_addr - sector_start_addr), (uint8_t*)io_data(io) + i, data_size_cur);
            buf = exo->flash.page;
        }

        nrf_flash_page_write(exo, sector_start_addr, buf, page_size);

        /* verify data in flash */
        if (memcmp(buf, (void*)sector_start_addr, page_size) != 0)
        {
            kerror(ERROR_CRC);
            return;
        }
    }
}

static inline void nrf_flash_open(EXO* exo, HANDLE user, unsigned int offs)
{
    if (exo->flash.active)
    {
        kerror(ERROR_ALREADY_CONFIGURED);
        return;
    }

    /* user process for send ipc reply */
    exo->flash.user = user;
    /* offset for flash storage */
    exo->flash.offset = NRF_FLASH_USER_CODE_SIZE;
    /* allocate memory for system data buffer for flash not aligned operations */
    exo->flash.page = kmalloc(flash_page_size());

    if(NULL == exo->flash.page)
    {
        kerror(ERROR_OUT_OF_MEMORY);
        return;
    }
    /* change flash active status */
    exo->flash.active = true;
}

static inline void nrf_flash_close(EXO* exo)
{
    /* not open yet */
    if(!exo->flash.active)
        return;

    exo->flash.user = INVALID_HANDLE;
    /* flash size offset set to end of flash not to zero for next safety */
    exo->flash.offset = flash_total_size();
    /* free memory */
    kfree(exo->flash.page);
    /* change flash status */
    exo->flash.active = false;
}

static inline void nrf_flash_get_media_descriptor(EXO* exo, HANDLE process, HANDLE user, IO* io)
{
    IPC ipc;
    STORAGE_MEDIA_DESCRIPTOR* media;
    if (!exo->flash.active)
    {
        kerror(ERROR_NOT_CONFIGURED);
        return;
    }
    if (user != exo->flash.user)
    {
        error(ERROR_ACCESS_DENIED);
        return;
    }
    media = io_data(io);
    media->num_sectors = (flash_total_size() - exo->flash.offset) / flash_get_page_size_bytes();
    media->num_sectors_hi = 0;
    media->sector_size = flash_get_page_size_bytes();

    /* serial number */
    sprintf(STORAGE_MEDIA_SERIAL(media), "%08X", NRF_FICR->DEVICEID[0]);

    io->data_size = sizeof(STORAGE_MEDIA_DESCRIPTOR) + 8 + 1;
    ipc.cmd = HAL_IO_CMD(HAL_FLASH, STORAGE_GET_MEDIA_DESCRIPTOR);
    ipc.process = process;
    ipc.param1 = user;
    ipc.param2 = (unsigned int)io;
    ipc.param3 = io->data_size;
    kipc_post(KERNEL_HANDLE, &ipc);
    error(ERROR_SYNC);
}

static inline void nrf_flash_request_notify_activity(EXO* exo, HANDLE process)
{
    if (exo->flash.activity != INVALID_HANDLE)
    {
        kerror(ERROR_ALREADY_CONFIGURED);
        return;
    }
    exo->flash.activity = process;
    kerror(ERROR_SYNC);
}

void nrf_flash_request(EXO* exo, IPC* ipc)
{
    // Flash request
    switch (HAL_ITEM(ipc->cmd))
    {
        case IPC_OPEN:
            nrf_flash_open(exo, (HANDLE)ipc->param1, ipc->param2);
            break;
        case IPC_CLOSE:
            nrf_flash_close(exo);
            break;
        case IPC_READ:
            nrf_flash_read(exo, (HANDLE)ipc->param1, (IO*)ipc->param2, ipc->param3);
            break;
        case IPC_WRITE:
            nrf_flash_write(exo, (HANDLE)ipc->param1, (IO*)ipc->param2, ipc->param3);
            break;
        case FLASH_GET_MEDIA_DESCRIPTOR:
            nrf_flash_get_media_descriptor(exo, ipc->process, (HANDLE)ipc->param1, (IO*)ipc->param2);
            break;
        case FLASH_NOTIFY_ACTIVITY:
            nrf_flash_request_notify_activity(exo, ipc->process);
            break;
        case FLASH_PAGE_READ:
            nrf_flash_page_read(exo, ipc->param1, (IO*)ipc->param2, ipc->param3);
            break;
        case FLASH_PAGE_WRITE:
            nrf_flash_page_write(exo, ipc->param1, io_data((IO*)ipc->param2), ipc->param3);
            break;
        case FLASH_PAGE_ERASE:
            nrf_flash_page_erase(exo, ipc->param1);
            break;
        case FLASH_GET_PAGE_SIZE:
            ipc->param3 = flash_page_size();
            break;
        case FLASH_GET_TOTAL_SIZE:
            ipc->param3 = flash_total_size();
            break;
        default:
            kerror(ERROR_NOT_SUPPORTED);
    }
}
