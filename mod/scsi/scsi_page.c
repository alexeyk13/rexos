/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "scsi_page.h"
#include "scsi_private.h"
#include <string.h>
#include "storage.h"
#include "kernel_config.h"
#include "queue.h"

void scsi_fill_error_page(SCSI* scsi, uint8_t code, uint16_t asq)
{
	char* buf = storage_io_buf_allocate(scsi->storage);
	int len = 18;
	memset(buf, 0, len);

	buf[0] = 0x70;
	buf[2] = code;
	buf[7] = len - 8;
	buf[12] = (char)(asq >> 8);
	buf[13] = (char)(asq & 0xff);

	storage_io_buf_filled(scsi->storage, buf, len);
}

void scsi_fill_capacity_page(SCSI* scsi)
{
	char* buf = storage_io_buf_allocate(scsi->storage);
	int len = 8;

	uint32_t max_sector = 0;
	uint32_t sector_size = 0;
	if (storage_is_media_present(scsi->storage))
	{
		max_sector = storage_get_media_descriptor(scsi->storage)->num_sectors - 1;
		sector_size = storage_get_device_descriptor(scsi->storage)->sector_size;
	}
	buf[0] = (char)(max_sector >> 24);
	buf[1] = (char)(max_sector >> 16);
	buf[2] = (char)(max_sector >> 8);
	buf[3] = (char)(max_sector);

	buf[4] = (char)(sector_size >> 24);
	buf[5] = (char)(sector_size >> 16);
	buf[6] = (char)(sector_size >> 8);
	buf[7] = (char)(sector_size);

	storage_io_buf_filled(scsi->storage, buf, len);
}

void scsi_fill_format_capacity_page(SCSI* scsi)
{
	char* buf = storage_io_buf_allocate(scsi->storage);
	int len = 12;

	buf[3] = len - 4;

	uint32_t num_sectors = 0;
	uint32_t sector_size = 0;
	if (storage_is_media_present(scsi->storage))
	{
		num_sectors = storage_get_media_descriptor(scsi->storage)->num_sectors;
		sector_size = storage_get_device_descriptor(scsi->storage)->sector_size;

		buf[4] = (char)(num_sectors >> 24);
		buf[5] = (char)(num_sectors >> 16);
		buf[6] = (char)(num_sectors >> 8);
		buf[7] = (char)(num_sectors);

		buf[8] = 2;
		buf[9] = (char)(sector_size >> 16);
		buf[10] = (char)(sector_size >> 8);
		buf[11] = (char)(sector_size);
	}
	else
	{
		buf[8] = 3;
	}

	storage_io_buf_filled(scsi->storage, buf, len);
}

void scsi_fill_standart_inquiry_page(SCSI* scsi)
{
	int i;

	char* buf = storage_io_buf_allocate(scsi->storage);
	int len = 36;
	memset(buf, 0, len);
	buf[0] = (char)scsi->descriptor->scsi_device_type;
	buf[1] = storage_get_device_descriptor(scsi->storage)->flags & STORAGE_DEVICE_FLAG_REMOVABLE ? 0x80 : 0x00;
	buf[2] = 0x04; //spc-2
	buf[3] = 0x02; //flags
	buf[4] = len -5;

	strncpy(buf + 8, scsi->descriptor->vendor, 8);
	strncpy(buf + 16, scsi->descriptor->product, 16);
	strncpy(buf + 32, scsi->descriptor->revision, 4);
	for (i = 8; i < 36; ++i)
		if (buf[i] < ' ' || buf[i] > '~')
			buf[i] = ' ';

	storage_io_buf_filled(scsi->storage, buf, len);
}

void scsi_fill_evpd_page_00(SCSI* scsi)
{
	char* buf = storage_io_buf_allocate(scsi->storage);
	int len = 7;
	memset(buf, 0, len);

	buf[0] = (char)scsi->descriptor->scsi_device_type;
	buf[1] = 0x00; //page address
	buf[3] = len - 4;
	buf[4] = INQUIRY_VITAL_PAGE_SUPPORTED_PAGES;
	buf[5] = INQUIRY_VITAL_PAGE_DEVICE_INFO;
	buf[6] = INQUIRY_VITAL_PAGE_SERIAL_NUM;

	storage_io_buf_filled(scsi->storage, buf, len);
}

void scsi_fill_evpd_page_80(SCSI* scsi)
{
	char* buf = storage_io_buf_allocate(scsi->storage);
	int len = STORAGE_SERIAL_NUMBER_SIZE + 4;
	memset(buf, 0, len);

	buf[0] = (char)scsi->descriptor->scsi_device_type;
	buf[1] = 0x80; //page address
	buf[3] = STORAGE_SERIAL_NUMBER_SIZE;
	if (storage_is_media_present(scsi->storage))
		strncpy(buf + 4, storage_get_media_descriptor(scsi->storage)->serial_number, STORAGE_SERIAL_NUMBER_SIZE);
	else
		memset(buf + 4, ' ', STORAGE_SERIAL_NUMBER_SIZE);

	storage_io_buf_filled(scsi->storage, buf, len);
}

void scsi_fill_evpd_page_83(SCSI* scsi)
{
	char* buf = storage_io_buf_allocate(scsi->storage);
	int len = 4 + 4 + strlen(scsi->descriptor->device_id);
	memset(buf, 0, len);

	buf[0] = (char)scsi->descriptor->scsi_device_type;
	buf[1] = 0x83; //page address

	buf[4] = 0x02;					//ASCII
	buf[5] = 0x01;					//The first eight bytes of the identifier field are Vendor ID
	buf[7] = (char)strlen(scsi->descriptor->device_id);
	strncpy(buf + 8, scsi->descriptor->device_id, buf[7]);
	buf[3] = buf[7] + 4;

	storage_io_buf_filled(scsi->storage, buf, len);
}

void scsi_fill_sense_page_1c(SCSI* scsi)
{
	char* buf = storage_io_buf_allocate(scsi->storage);
	int len = 8;
	memset(buf, 0, len);
	buf[0] = 0x1c;
	buf[1] = len -2;
	storage_io_buf_filled(scsi->storage, buf, len);
}

void scsi_fill_sense_page_3f(SCSI* scsi)
{
	scsi_fill_sense_page_1c(scsi);
}
