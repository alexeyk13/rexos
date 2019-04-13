/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: RL (jam_roma@yahoo.com)
*/


#include "flash.h"

unsigned int flash_get_page_size()
{
    return get_size_exo(HAL_REQ(HAL_FLASH, FLASH_GET_PAGE_SIZE), 0, 0, 0);
}
unsigned int flash_get_total_size()
{
    return get_size_exo(HAL_REQ(HAL_FLASH, FLASH_GET_TOTAL_SIZE), 0, 0, 0);
}
