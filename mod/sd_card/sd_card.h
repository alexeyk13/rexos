/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef SD_CARD_H
#define SD_CARD_H

#include "dev.h"
#include "storage.h"
#include "sd_card_defs.h"
#include "sdio.h"
#include "scsi.h"

typedef struct {
	//first field will be aligned 4
	uint32_t sdio_cmd_response[4];
	char cmd_buf[64];
	volatile bool futex;
	SDIO_CLASS port;
	STORAGE* storage;
	STORAGE_STATUS status;
	bool writing;
	STORAGE_MEDIA_DESCRIPTOR media;
	bool card_present;
	HANDLE data_event;
	//raw response data
	//card registers
	SDIO_CARD_TYPE card_type;
	uint16_t rca;
}SD_CARD;

SD_CARD* sd_card_create(SDIO_CLASS port, int priority);
void sd_card_destroy(SD_CARD* sd_card);

STORAGE* sd_card_get_storage(SD_CARD* sd_card);

#endif // SD_CARD_H
