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
    FLASH_GET_MEDIA_DESCRIPTOR = IPC_USER,
    FLASH_NOTIFY_ACTIVITY,
    FLASH_PAGE_READ,
    FLASH_PAGE_WRITE,
    FLASH_PAGE_ERASE,
    FLASH_GET_PAGE_SIZE,
    FLASH_GET_TOTAL_SIZE,
} FLASH_IPC;

unsigned int flash_get_page_size_bytes();
unsigned int flash_get_size_bytes();

int flash_page_write(unsigned int addr, IO* io);
int flash_page_read(unsigned int addr, IO* io, unsigned int size);
int flash_page_erase(unsigned int addr);


#endif /* _FLASH_H_ */
