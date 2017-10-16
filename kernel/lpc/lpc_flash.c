/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "lpc_flash.h"
#include "lpc_exo_private.h"
#include "../../userspace/storage.h"
#include "../../userspace/stdio.h"
#include "../kipc.h"
#include "../kerror.h"
#include "lpc_iap.h"
#include <string.h>

#define FLASH_BANK_A_BASE                       0x1a000000
#define FLASH_BANK_B_BASE                       0x1b000000

#define FLASH_PAGE_SIZE                         0x200
#define FLASH_SMALL_SECTOR_SIZE                 (8 * 1024)
#define FLASH_BIG_SECTOR_SIZE                   (64 * 1024)
#define FLASH_SMALL_SECTORS_COUNT               8
#define FLASH_SMALL_SECTORS_SIZE                (FLASH_SMALL_SECTORS_COUNT * FLASH_SMALL_SECTOR_SIZE)

typedef struct {
    unsigned int first_page, last_page;
    unsigned int sector;
    unsigned int bank;
    unsigned int size;
} FLASH_ADDR_BLOCK_TYPE;


void lpc_flash_init(EXO* exo)
{
    exo->flash.active = false;
    exo->flash.user = INVALID_HANDLE;
    exo->flash.activity = INVALID_HANDLE;
}

static bool lpc_flash_decode_addr_block(unsigned int addr, FLASH_ADDR_BLOCK_TYPE* addr_block, unsigned int size)
{
    if ((addr % FLASH_PAGE_SIZE) || (size % FLASH_PAGE_SIZE))
    {
        kerror(ERROR_INVALID_ALIGN);
        return false;
    }
    addr_block->first_page = addr + LPC_FLASH_USER_CODE_SIZE;
    //bank A
    if (addr_block->first_page + FLASH_PAGE_SIZE <= FLASH_SIZE)
        addr_block->bank = 0;
    //bank B
    else if (addr_block->first_page + FLASH_PAGE_SIZE <= FLASH_SIZE + FLASH_SIZE_B)
    {
        addr_block->first_page -= FLASH_SIZE;
        addr_block->bank = 1;
    }
    else
    {
        kerror(ERROR_OUT_OF_RANGE);
        return false;
    }
    //first 8 sectors are 8KB
    if (addr_block->first_page <= FLASH_SMALL_SECTORS_SIZE)
    {
        addr_block->sector = addr_block->first_page / FLASH_SMALL_SECTOR_SIZE;
        addr_block->size = FLASH_SMALL_SECTOR_SIZE - (addr_block->first_page % FLASH_SMALL_SECTOR_SIZE);
    }
    else
    {
        addr_block->sector = FLASH_SMALL_SECTORS_COUNT + (addr_block->first_page - FLASH_SMALL_SECTORS_SIZE) / FLASH_BIG_SECTOR_SIZE;
        addr_block->size = FLASH_BIG_SECTOR_SIZE - ((addr_block->first_page - FLASH_SMALL_SECTORS_SIZE) % FLASH_BIG_SECTOR_SIZE);
    }
    if (addr_block->size > size)
        addr_block->size = size;
    addr_block->first_page += addr_block->bank == 0 ? FLASH_BANK_A_BASE : FLASH_BANK_B_BASE;
    addr_block->last_page = addr_block->first_page + addr_block->size - FLASH_PAGE_SIZE;
    return true;
}

static bool lpc_flash_prepare_sector(FLASH_ADDR_BLOCK_TYPE* addr_block, LPC_IAP_TYPE* iap)
{
    iap->req[1] = iap->req[2] = addr_block->sector;
    iap->req[3] = addr_block->bank;
    if (!lpc_iap(iap, IAP_CMD_PREPARE_SECTORS_FOR_WRITE))
    {
        kerror(ERROR_HARDWARE);
        return false;
    }
    return true;
}

static inline bool lpc_flash_is_block_empty(unsigned int* addr, unsigned int words)
{
    unsigned int i;
    for (i = 0; i < words; ++i)
        if (addr[i] != 0xffffffff)
            return false;
    return true;
}

static bool lpc_flash_erase_block(FLASH_ADDR_BLOCK_TYPE* addr_block, unsigned int clock_khz)
{
    LPC_IAP_TYPE iap;

    if (lpc_flash_is_block_empty((unsigned int*)addr_block->first_page, addr_block->size / sizeof(unsigned int)))
        return true;

    if (!lpc_flash_prepare_sector(addr_block, &iap))
        return false;

    iap.req[1] = addr_block->first_page;
    iap.req[2] = addr_block->last_page;
    iap.req[3] = clock_khz;
    if (!lpc_iap(&iap, IAP_CMD_ERASE_PAGE))
    {
        kerror(ERROR_HARDWARE);
        return false;
    }
    return true;
}

static bool lpc_flash_write_block(FLASH_ADDR_BLOCK_TYPE* addr_block, void* buf, unsigned int clock_khz)
{
    LPC_IAP_TYPE iap;
    unsigned int i, chunk_size;

    for (i = 0; i < addr_block->size; i += chunk_size)
    {
        chunk_size = addr_block->size - i;
        //only 4096, 1024 and 512 are valid IAP values
        if (chunk_size >= 4096)
            chunk_size = 4096;
        else if (chunk_size >= 1024)
            chunk_size = 1024;
        if (!lpc_flash_prepare_sector(addr_block, &iap))
            return false;
        iap.req[1] = addr_block->first_page + i;
        iap.req[2] = (unsigned int)((uint8_t*)buf + i);
        iap.req[3] = chunk_size;
        iap.req[4] = clock_khz;
        if (!lpc_iap(&iap, IAP_CMD_COPY_RAM_TO_FLASH))
        {
            kerror(ERROR_HARDWARE);
            return false;
        }
    }
    return true;
}

static inline void lpc_flash_flush(EXO* exo)
{
    if (!exo->flash.active)
    {
        kerror(ERROR_NOT_CONFIGURED);
        return;
    }
}

static inline void lpc_flash_open(EXO* exo, HANDLE user)
{
    LPC_IAP_TYPE iap;
    if (exo->flash.active)
    {
        kerror(ERROR_ALREADY_CONFIGURED);
        return;
    }

    lpc_iap(&iap, IAP_CMD_INIT);
    kerror(ERROR_OK);

    exo->flash.user = user;
    exo->flash.active = true;
}

static inline void lpc_flash_close(EXO* exo)
{
    if (!exo->flash.active)
    {
        kerror(ERROR_NOT_CONFIGURED);
        return;
    }
    exo->flash.active = false;
}

static inline void lpc_flash_read(EXO* exo, HANDLE process, HANDLE user, IO* io, unsigned int size)
{
    FLASH_ADDR_BLOCK_TYPE addr_block;
    STORAGE_STACK* stack = io_stack(io);
    unsigned int base_addr;
    io_pop(io, sizeof(STORAGE_STACK));
    if (!exo->flash.active)
    {
        kerror(ERROR_NOT_CONFIGURED);
        return;
    }
    if ((user != exo->flash.user) || (size == 0) || (size % FLASH_PAGE_SIZE))
    {
        kerror(ERROR_INVALID_PARAMS);
        return;
    }
    if ((exo->flash.activity != INVALID_HANDLE) && !(stack->flags & STORAGE_FLAG_IGNORE_ACTIVITY_ON_REQUEST))
    {
        kipc_post_exo(exo->flash.activity, HAL_CMD(HAL_FLASH, STORAGE_NOTIFY_ACTIVITY), exo->flash.user, 0, 0);
        exo->flash.activity = INVALID_HANDLE;
    }

    base_addr = stack->sector * FLASH_PAGE_SIZE;
    for (io->data_size = 0; io->data_size < size; )
    {
        if (!lpc_flash_decode_addr_block(base_addr + io->data_size, &addr_block, size - io->data_size))
            return;
        io_data_append(io, (void*)addr_block.first_page, addr_block.size);
    }
    kipc_post_exo(process, HAL_IO_CMD(HAL_FLASH, IPC_READ), user, (unsigned int)io, io->data_size);
    kerror(ERROR_SYNC);
}

static inline void lpc_flash_write(EXO* exo, HANDLE process, HANDLE user, IO* io, unsigned int size)
{
    SHA1_CTX sha1;
    FLASH_ADDR_BLOCK_TYPE addr_block;
    unsigned int i, base_addr, clock_khz;
    uint8_t hash_in[SHA1_BLOCK_SIZE];
    uint8_t hash_out[SHA1_BLOCK_SIZE];
    STORAGE_STACK* stack = io_stack(io);
    io_pop(io, sizeof(STORAGE_STACK));
    if (!exo->flash.active)
    {
        kerror(ERROR_NOT_CONFIGURED);
        return;
    }
    if ((stack->flags & STORAGE_MASK_MODE) == STORAGE_FLAG_ERASE_ONLY)
        size *= FLASH_PAGE_SIZE;
    if ((user != exo->flash.user) || (size == 0) || (size % FLASH_PAGE_SIZE))
    {
        kerror(ERROR_INVALID_PARAMS);
        return;
    }
    if ((exo->flash.activity != INVALID_HANDLE) && !(stack->flags & STORAGE_FLAG_IGNORE_ACTIVITY_ON_REQUEST))
    {
        kipc_post_exo(exo->flash.activity, HAL_CMD(HAL_FLASH, STORAGE_NOTIFY_ACTIVITY), exo->flash.user, STORAGE_FLAG_WRITE, 0);
        exo->flash.activity = INVALID_HANDLE;
    }

    //generate in hash
    if (stack->flags & STORAGE_FLAG_VERIFY)
    {
        sha1_init(&sha1);
        sha1_update(&sha1, io_data(io), io->data_size);
        sha1_final(&sha1, hash_in);
        sha1_init(&sha1);
    }
    base_addr = stack->sector * FLASH_PAGE_SIZE;
    //core clock may change between requests for power saving
    clock_khz = lpc_power_get_core_clock_inside() / 1000;
    for(i = 0; i < size; i += addr_block.size)
    {
        if (!lpc_flash_decode_addr_block(base_addr + i, &addr_block, size - i))
            return;
        //erase
        if (((stack->flags & STORAGE_MASK_MODE) == 0) || (stack->flags & STORAGE_FLAG_WRITE))
        {
            if (!lpc_flash_erase_block(&addr_block, clock_khz))
                return;
        }
        //write page
        if (stack->flags & STORAGE_FLAG_WRITE)
        {
            if (!lpc_flash_write_block(&addr_block, (uint8_t*)io_data(io) + i, clock_khz))
                return;
        }
        //read back for verify
        if (stack->flags & STORAGE_FLAG_VERIFY)
            sha1_update(&sha1, (void*)addr_block.first_page, addr_block.size);
    }

    //check hash for verify
    if (stack->flags & STORAGE_FLAG_VERIFY)
    {
        sha1_final(&sha1, hash_out);
        if (memcmp(hash_in, hash_out, SHA1_BLOCK_SIZE) != 0)
        {
            kerror(ERROR_CRC);
            return;
        }
    }
    kipc_post_exo(process, HAL_IO_CMD(HAL_FLASH, IPC_WRITE), user, (unsigned int)io, io->data_size);
    kerror(ERROR_SYNC);
}

static inline void lpc_flash_get_media_descriptor(EXO* exo, HANDLE process, HANDLE user, IO* io)
{
    STORAGE_MEDIA_DESCRIPTOR* media;
    LPC_IAP_TYPE iap;
    if (!exo->flash.active)
    {
        kerror(ERROR_NOT_CONFIGURED);
        return;
    }
    if (user != exo->flash.user)
    {
        kerror(ERROR_INVALID_PARAMS);
        return;
    }
    media = io_data(io);
    media->num_sectors = (FLASH_SIZE + FLASH_SIZE_B - LPC_FLASH_USER_CODE_SIZE) / FLASH_PAGE_SIZE;
    media->num_sectors_hi = 0;
    media->sector_size = FLASH_PAGE_SIZE;

    //read serial by IAP
    if (!lpc_iap(&iap, IAP_CMD_READ_DEVICE_SERIAL))
        return;
    sprintf(STORAGE_MEDIA_SERIAL(media), "%08X%08X%08X%08X", iap.resp[1], iap.resp[2], iap.resp[3], iap.resp[4]);

    io->data_size = sizeof(STORAGE_MEDIA_DESCRIPTOR) + 32 + 1;
    kipc_post_exo(process, HAL_IO_CMD(HAL_FLASH, STORAGE_GET_MEDIA_DESCRIPTOR), exo->flash.user, (unsigned int)io, io->data_size);
    kerror(ERROR_SYNC);
}

static inline void lpc_flash_request_notify_activity(EXO* exo, HANDLE process)
{
    if (exo->flash.activity != INVALID_HANDLE)
    {
        kerror(ERROR_ALREADY_CONFIGURED);
        return;
    }
    exo->flash.activity = process;
    kerror(ERROR_SYNC);
}

void lpc_flash_request(EXO* exo, IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_OPEN:
        lpc_flash_open(exo, (HANDLE)ipc->param1);
        break;
    case IPC_CLOSE:
        lpc_flash_close(exo);
        break;
    case IPC_READ:
        lpc_flash_read(exo, ipc->process, (HANDLE)ipc->param1, (IO*)ipc->param2, ipc->param3);
        break;
    case IPC_WRITE:
        lpc_flash_write(exo, ipc->process, (HANDLE)ipc->param1, (IO*)ipc->param2, ipc->param3);
        break;
    case IPC_FLUSH:
        lpc_flash_flush(exo);
        break;
    case STORAGE_GET_MEDIA_DESCRIPTOR:
        lpc_flash_get_media_descriptor(exo, ipc->process, (HANDLE)ipc->param1, (IO*)ipc->param2);
        break;
    case STORAGE_NOTIFY_ACTIVITY:
        lpc_flash_request_notify_activity(exo, ipc->process);
        break;
    default:
        kerror(ERROR_NOT_SUPPORTED);
    }
}


