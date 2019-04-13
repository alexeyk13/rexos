/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: RL (jam_roma@yahoo.com)
*/

#ifndef _FLASH_H_
#define _FLASH_H_

#include "ipc.h"
#include "io.h"

typedef enum {
    FLASH_GET_PAGE_SIZE = IPC_USER,
    FLASH_GET_TOTAL_SIZE,
} FLASH_IPC;

unsigned int flash_get_page_size();
unsigned int flash_get_total_size();

void flash_page_write(unsigned int addr, IO* io);
void flash_page_read(unsigned int addr, IO* io);


#endif /* _FLASH_H_ */
