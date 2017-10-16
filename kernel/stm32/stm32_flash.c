/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "stm32_flash.h"
#include "stm32_exo_private.h"
#include "../../userspace/storage.h"
#include "../../userspace/stdio.h"
#include "../kipc.h"
#include "../kstdlib.h"
#include "../kerror.h"
#include <string.h>

#if defined(STM32F1)
#define FLASH_SIZE_BASE                       0x1FFFF7E0
#define FLASH_UUID_BASE                       0x1FFFF7E8
#elif defined(STM32F0)
#define FLASH_KEY1                            0x45670123
#define FLASH_KEY2                            0xCDEF89AB
#define FLASH_SIZE_BASE                       0x1FFFF7CC
#define FLASH_UUID_BASE                       0x1FFFF7AC
#endif

#define FLASH_SECTOR_SIZE                     0x200
#define FLASH_PAGE_SIZE                      (1024)
#define FLASH_PAGE_MASK                      (~(FLASH_PAGE_SIZE-1))
//#define FLASH_SECTORS_IN_PAGE                ( FLASH_PAGE_SIZE/FLASH_SECTOR_SIZE)

#define FLASH_SECTOR_COUNT                   (*((uint16_t*)(FLASH_SIZE_BASE))*1024ul/FLASH_SECTOR_SIZE)

typedef struct {
    unsigned int first_page, last_page;
    unsigned int sector;
    unsigned int bank;
    unsigned int size;
} FLASH_ADDR_BLOCK_TYPE;

void stm32_flash_init(EXO* exo)
{
    exo->flash.active = false;
    exo->flash.user = INVALID_HANDLE;
    exo->flash.activity = INVALID_HANDLE;
}

static bool check_range(uint32_t sector, uint32_t size)
{
    if((size == 0) || (size & (FLASH_SECTOR_SIZE - 1)))
        return false;
    if((sector + size/FLASH_SECTOR_SIZE) > FLASH_SECTOR_COUNT)
        return false;
    return true;
}

static inline void stm32_flash_flush(EXO* exo)
{
    if (!exo->flash.active)
    {
        kerror(ERROR_NOT_CONFIGURED);
        return;
    }
}

static inline void stm32_flash_open(EXO* exo, HANDLE user)
{
    if (exo->flash.active)
    {
        kerror(ERROR_ALREADY_CONFIGURED);
        return;
    }
    if (FLASH->CR & FLASH_CR_LOCK)
    {
        disable_interrupts();
        FLASH->KEYR = FLASH_KEY1;
        FLASH->KEYR = FLASH_KEY2;
        enable_interrupts();
    }
    kerror(ERROR_OK);

    exo->flash.user = user;
    exo->flash.active = true;
}

static inline void stm32_flash_close(EXO* exo)
{
    if (!exo->flash.active)
    {
        kerror(ERROR_NOT_CONFIGURED);
        return;
    }
    FLASH->CR |= FLASH_CR_LOCK;
    exo->flash.active = false;
}

static inline void stm32_flash_read(EXO* exo, HANDLE process, HANDLE user, IO* io, unsigned int size)
{
    STORAGE_STACK* stack = io_stack(io);
    unsigned int base_addr;
    io_pop(io, sizeof(STORAGE_STACK));
    if (!exo->flash.active)
    {
        kerror(ERROR_NOT_CONFIGURED);
        return;
    }
    if ((user != exo->flash.user) || !check_range(stack->sector, size))
    {
        kerror(ERROR_INVALID_PARAMS);
        return;
    }
    if ((exo->flash.activity != INVALID_HANDLE) && !(stack->flags & STORAGE_FLAG_IGNORE_ACTIVITY_ON_REQUEST))
    {
        kipc_post_exo(exo->flash.activity, HAL_CMD(HAL_FLASH, STORAGE_NOTIFY_ACTIVITY), exo->flash.user, 0, 0);
        exo->flash.activity = INVALID_HANDLE;
    }

    base_addr = stack->sector * FLASH_SECTOR_SIZE + FLASH_BASE + STM32_FLASH_USER_CODE_SIZE;

    io->data_size = 0;
    io_data_append(io, (void*)base_addr, size);

    kipc_post_exo(process, HAL_IO_CMD(HAL_FLASH, IPC_READ), user, (unsigned int)io, io->data_size);
    kerror(ERROR_SYNC);
}

static inline bool is_block_empty(uint8_t* addr, unsigned int words)
{
    unsigned int i;
    for (i = 0; i < words; ++i)
        if (addr[i] != 0xffffffff)
            return false;
    return true;
}

static inline void erase_page(uint32_t page_addr)
{
    if(is_block_empty((uint8_t*)page_addr, FLASH_PAGE_SIZE / sizeof(uint32_t)))
        return;
    FLASH->CR |= FLASH_CR_PER;
    FLASH->AR = page_addr;
    FLASH->CR |= FLASH_CR_STRT;
    while(FLASH->SR & FLASH_SR_BSY);
    FLASH->CR &= ~FLASH_CR_PER;

}

static void flash_block_write(uint32_t dst, uint8_t* src, uint32_t size, bool* verify)
{
    __IO uint16_t* flash = (__IO uint16_t*)dst;
    uint16_t* ram = (uint16_t*)src;
    size /= 2;
    do
    {
        __disable_irq();
        FLASH->CR |= FLASH_CR_PG;
        *flash = *ram;
        while(FLASH->SR & FLASH_SR_BSY);
        FLASH->CR &= ~FLASH_CR_PG;
        __enable_irq();
        if(*flash++ != *ram++)
            *verify = false;
    }while(--size);
}

static inline void stm32_flash_write(EXO* exo, HANDLE process, HANDLE user, IO* io, unsigned int size)
{
    unsigned int write_size, addr, page_addr;
    void* head_ptr;
    void *tail_ptr;
    int head_size, tail_size;
    bool verify = true;
    STORAGE_STACK* stack = io_stack(io);
    io_pop(io, sizeof(STORAGE_STACK));
    if ((stack->flags & STORAGE_MASK_MODE) == STORAGE_FLAG_ERASE_ONLY)
    {
        io->data_size = size;
        size *= FLASH_SECTOR_SIZE;
    }

    if (!exo->flash.active)
    {
        kerror(ERROR_NOT_CONFIGURED);
        return;
    }
    if ((user != exo->flash.user) || !check_range(stack->sector, size))
    {
        kerror(ERROR_INVALID_PARAMS);
        return;
    }
    if ((exo->flash.activity != INVALID_HANDLE) && !(stack->flags & STORAGE_FLAG_IGNORE_ACTIVITY_ON_REQUEST))
    {
        kipc_post_exo(exo->flash.activity, HAL_CMD(HAL_FLASH, STORAGE_NOTIFY_ACTIVITY), exo->flash.user, STORAGE_FLAG_WRITE, 0);
        exo->flash.activity = INVALID_HANDLE;
    }

    addr = stack->sector * FLASH_SECTOR_SIZE + FLASH_BASE + STM32_FLASH_USER_CODE_SIZE;
    //just to ignore warnings
    tail_ptr = NULL;
    while (size)
    {
        page_addr = addr & FLASH_PAGE_MASK;
        head_size = addr - page_addr;
        if(head_size > 0)
        {
            head_ptr = kmalloc(head_size);
            memcpy(head_ptr, (uint8_t*)page_addr, head_size);
        }
        else
            head_size = 0;
        tail_size = (page_addr + FLASH_PAGE_SIZE) - (addr + size);
        if (tail_size > 0)
        {
            tail_ptr = kmalloc(tail_size);
            memcpy(tail_ptr, (uint8_t*)(addr + size), tail_size);
        }
        else
            tail_size = 0;

        erase_page(page_addr);
        if(head_size > 0)
        {
            flash_block_write(page_addr, head_ptr, head_size, &verify);
            kfree(head_ptr);
        }
        write_size = FLASH_PAGE_SIZE - tail_size - head_size;
        if((stack->flags & STORAGE_MASK_MODE) != 0)
        {
            flash_block_write(addr, io_data(io), write_size, &verify);
            io_hide(io, write_size);
        }
        if (tail_size > 0)
        {
            flash_block_write(addr + size, tail_ptr, tail_size, &verify);
            kfree(tail_ptr);
        }
        size -= write_size;
        addr = page_addr + FLASH_PAGE_SIZE;
    }
//check for verify
    if ((stack->flags & STORAGE_FLAG_VERIFY) && !verify)
    {
        kerror(ERROR_CRC);
        return;
    }
    io_show(io);
    kipc_post_exo(process, HAL_IO_CMD(HAL_FLASH, IPC_WRITE), user, (unsigned int)io, io->data_size);
    kerror(ERROR_SYNC);
}

static inline void stm32_flash_get_media_descriptor(EXO* exo, HANDLE process, HANDLE user, IO* io)
{
    uint32_t* uuid = (uint32_t*)FLASH_UUID_BASE;
    STORAGE_MEDIA_DESCRIPTOR* media;
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
    media->num_sectors = FLASH_SECTOR_COUNT;
    media->num_sectors_hi = 0;
    media->sector_size = FLASH_SECTOR_SIZE;

    sprintf(STORAGE_MEDIA_SERIAL(media), "%08X%08X%08X", uuid[0], uuid[1], uuid[2]);

    io->data_size = sizeof(STORAGE_MEDIA_DESCRIPTOR) + 6 * 4 + 1;
    kipc_post_exo(process, HAL_IO_CMD(HAL_FLASH, STORAGE_GET_MEDIA_DESCRIPTOR), exo->flash.user, (unsigned int)io, io->data_size);
    kerror(ERROR_SYNC);
}

static inline void stm32_flash_request_notify_activity(EXO* exo, HANDLE process)
{
    if (exo->flash.activity != INVALID_HANDLE)
    {
        kerror(ERROR_ALREADY_CONFIGURED);
        return;
    }
    exo->flash.activity = process;
    kerror(ERROR_SYNC);
}

void stm32_flash_request(EXO* exo, IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_OPEN:
        stm32_flash_open(exo, (HANDLE)ipc->param1);
        break;
    case IPC_CLOSE:
        stm32_flash_close(exo);
        break;
    case IPC_READ:
        stm32_flash_read(exo, ipc->process, (HANDLE)ipc->param1, (IO*)ipc->param2, ipc->param3);
        break;
    case IPC_WRITE:
        stm32_flash_write(exo, ipc->process, (HANDLE)ipc->param1, (IO*)ipc->param2, ipc->param3);
        break;
    case IPC_FLUSH:
        stm32_flash_flush(exo);
        break;
    case STORAGE_GET_MEDIA_DESCRIPTOR:
        stm32_flash_get_media_descriptor(exo, ipc->process, (HANDLE)ipc->param1, (IO*)ipc->param2);
        break;
    case STORAGE_NOTIFY_ACTIVITY:
        stm32_flash_request_notify_activity(exo, ipc->process);
        break;
    default:
        kerror(ERROR_NOT_SUPPORTED);
    }
}

