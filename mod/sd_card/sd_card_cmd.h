/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef SD_CARD_CMD_H
#define SD_CARD_CMD_H

#include "sd_card.h"
#include "sdio.h"

//callbacks from abstract storage
bool sd_card_check_media(void* param);
void sd_card_stop(void* param);
STORAGE_STATUS sd_card_read_blocks(void* param, unsigned long addr, char* buf, unsigned long blocks_count);
STORAGE_STATUS sd_card_write_blocks(void* param, unsigned long addr, char* buf, unsigned long blocks_count);

void sd_card_on_read_complete(SDIO_CLASS port, void* param);
void sd_card_on_write_complete(SDIO_CLASS port, void* param);
void sd_card_on_error(SDIO_CLASS port, void* param, SDIO_ERROR error);

#endif // SD_CARD_CMD_H
