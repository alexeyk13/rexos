/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "sd_card.h"
#include "sd_card_defs.h"
#include "sdio.h"
#include "sd_card_cmd.h"
#include <string.h>
#include "dbg.h"
#include "kernel_config.h"
#include "gpio.h"
#include "event.h"
#include "mem_private.h"
#include "error.h"

static const SDIO_CB _sdio_cb = {
	sd_card_on_read_complete,
	sd_card_on_write_complete,
	sd_card_on_error
};

static const  STORAGE_DEVICE_DESCRIPTOR _storage_device_descriptor = {
	STORAGE_DEVICE_FLAG_REMOVABLE,
	SD_CARD_SECTOR_SIZE
};

static const STORAGE_DRIVER_CB _storage_cb = {
	sd_card_check_media,
	//on_cancel_io
	NULL,
	//on_storage_prepare_read
	NULL,
	sd_card_read_blocks,
	//on_storage_read_done
	NULL,
	//on_storage_prepare_read
	NULL,
	sd_card_write_blocks,
	//on_storage_write_done
	NULL,
	//on_write_cache
	NULL,
	sd_card_stop
};

SD_CARD* sd_card_create(SDIO_CLASS port, int priority)
{
	SD_CARD* sd_card = (SD_CARD*)sys_alloc(sizeof(SD_CARD));
	if (sd_card)
	{
		sd_card->storage = storage_create(&_storage_device_descriptor, (P_STORAGE_DRIVER_CB)&_storage_cb, (void*)sd_card);
		sd_card->data_event = event_create();

		sd_card->card_present = false;
		sd_card->port = port;
		sdio_enable(port, (P_SDIO_CB)&_sdio_cb, sd_card, priority);

		gpio_enable_pin(SD_CARD_CD_PIN, PIN_MODE_IN_PULLUP);
	}
	else
		error(ERROR_MEM_OUT_OF_SYSTEM_MEMORY, "SD_CARD");
	return sd_card;
}

void sd_card_destroy(SD_CARD* sd_card)
{
	storage_destroy(sd_card->storage);
	event_destroy(sd_card->data_event);
	sdio_disable(sd_card->port);
	gpio_disable_pin(SD_CARD_CD_PIN);
	sys_free(sd_card);
}

void sd_card_disable(SD_CARD* sd_card)
{
	event_destroy(sd_card->data_event);
	gpio_disable_pin(SD_CARD_CD_PIN);
	sdio_disable(sd_card->port);
}

STORAGE* sd_card_get_storage(SD_CARD *sd_card)
{
	return sd_card->storage;
}
