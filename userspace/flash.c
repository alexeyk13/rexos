/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: RL (jam_roma@yahoo.com)
*/


#include "flash.h"

unsigned int flash_get_page_size_bytes()
{
    return get_size_exo(HAL_REQ(HAL_FLASH, FLASH_GET_PAGE_SIZE), 0, 0, 0);
}
unsigned int flash_get_size_bytes()
{
    return get_size_exo(HAL_REQ(HAL_FLASH, FLASH_GET_TOTAL_SIZE), 0, 0, 0);
}

int flash_page_write(unsigned int addr, IO* io)
{
    return io_write_sync_exo(HAL_IO_REQ(HAL_FLASH, FLASH_PAGE_WRITE), addr, io);
}

int flash_page_read(unsigned int addr, IO* io, unsigned int size)
{
    return io_read_sync_exo(HAL_IO_REQ(HAL_FLASH, FLASH_PAGE_READ), addr, io, size);
}

int flash_page_erase(unsigned int addr)
{
    return get_size_exo(HAL_REQ(HAL_FLASH, FLASH_PAGE_ERASE), addr, 0, 0);
}
