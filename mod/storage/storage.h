/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef STORAGE_H
#define STORAGE_H

/*
		storage.h - abstract block device storage, made standart interface between driver and host
  */

#include "types.h"
#include "dlist.h"

#define STORAGE_DEVICE_FLAG_REMOVABLE			(1 << 0)
#define STORAGE_DEVICE_FLAG_READ_ONLY			(1 << 1)

#define STORAGE_MEDIA_FLAG_WRITE_PROTECTION	(1 << 0)
#define STORAGE_MEDIA_FLAG_READ_PROTECTION	(1 << 1)
#define STORAGE_MEDIA_FLAG_REMOVAL_DISABLED	(1 << 2)

#define STORAGE_SERIAL_NUMBER_SIZE		12

typedef enum {
	STORAGE_STATUS_OK = 0,
	STORAGE_STATUS_OPERATION_IN_PROGRESS,
	STORAGE_STATUS_OPERATION_UNEXPECTED,
	STORAGE_STATUS_NO_MEDIA,
	STORAGE_STATUS_INVALID_ADDRESS,
	STORAGE_STATUS_HARDWARE_FAILURE,
	STORAGE_STATUS_CRC_ERROR,
	STORAGE_STATUS_DATA_PROTECTED,
	STORAGE_STATUS_TIMEOUT,
	STORAGE_STATUS_MISCOMPARE
}STORAGE_STATUS;

typedef enum {
	STORAGE_STATE_IDLE = 0,
	STORAGE_STATE_READ,
	STORAGE_STATE_WRITE,
	STORAGE_STATE_READ_WRITE
}STORAGE_STATE;

typedef enum {
	STORAGE_MEDIA_INSERTED,
	STORAGE_MEDIA_REMOVED,
	STORAGE_MEDIA_SURPRISAL_REMOVED
}STORAGE_MEDIA_STATE;

typedef struct {
	uint32_t flags;
	uint16_t	sector_size;
}STORAGE_DEVICE_DESCRIPTOR;

typedef struct {
	uint32_t num_sectors;
	char serial_number[STORAGE_SERIAL_NUMBER_SIZE + 1];
	uint32_t flags;
}STORAGE_MEDIA_DESCRIPTOR;

typedef struct {
	DLIST dlist;
	void (*state_changed)(void*, STORAGE_STATE);
	void (*media_state_changed)(void*, STORAGE_MEDIA_STATE);
	void* param;
}STORAGE_HOST_CB;

typedef struct {
	void (*on_storage_buffer_filled)(void* param, unsigned int size);
	void (*on_storage_request_buffers)(void* param, unsigned int size);
}STORAGE_HOST_IO_CB, *P_STORAGE_HOST_IO_CB;


typedef struct {
	bool (*on_storage_check_media)(void* driver_param);
	void (*on_cancel_io)(void* driver_param);
	STORAGE_STATUS (*on_storage_prepare_read)(void* driver_param, unsigned long addr, unsigned long count);
	STORAGE_STATUS (*on_storage_read_blocks)(void* driver_param, unsigned long addr, char* buf, unsigned long blocks_count);
	STORAGE_STATUS (*on_storage_read_done)(void* driver_param);
	STORAGE_STATUS (*on_storage_prepare_write)(void* driver_param, unsigned long addr, unsigned long count);
	STORAGE_STATUS (*on_storage_write_blocks)(void* driver_param, unsigned long addr, char* buf, unsigned long blocks_count);
	STORAGE_STATUS (*on_storage_write_done)(void* driver_param);
	void (*on_write_cache)(void* driver_param);
	void (*on_stop)(void* driver_param);
}STORAGE_DRIVER_CB, *P_STORAGE_DRIVER_CB;

typedef struct {
	const STORAGE_DEVICE_DESCRIPTOR* device_descriptor;
	STORAGE_MEDIA_DESCRIPTOR* media_descriptor;
	bool reading, writing;
	STORAGE_STATUS read_status, write_status;
	//callback to upper layer
	STORAGE_HOST_CB* host_cb;
	STORAGE_DRIVER_CB const *driver_cb;
	STORAGE_HOST_IO_CB* host_io_cb;
	void* host_io_param;
	void* driver_param;
	HANDLE rx_event, tx_event;
	HANDLE queue;
	unsigned long block_size, sectors_in_block;
}STORAGE;

typedef struct {
	HANDLE queue;
	unsigned long block_size;
	STORAGE_HOST_IO_CB* host_io_cb;
	void* host_io_param;
}STORAGE_QUEUE_PARAMS;

//common api
const STORAGE_DEVICE_DESCRIPTOR* storage_get_device_descriptor(STORAGE* storage);
const STORAGE_MEDIA_DESCRIPTOR* storage_get_media_descriptor(STORAGE* storage);

//driver api
STORAGE *storage_create(const STORAGE_DEVICE_DESCRIPTOR *device_descriptor, STORAGE_DRIVER_CB *driver_cb, void *driver_param);
void storage_destroy(STORAGE* storage);
void storage_insert_media(STORAGE* storage, STORAGE_MEDIA_DESCRIPTOR* media);
void storage_eject_media(STORAGE* storage);
void storage_blocks_readed(STORAGE* storage, STORAGE_STATUS status);
void storage_blocks_writed(STORAGE* storage, STORAGE_STATUS status);

//host api
void storage_register_handler(STORAGE* storage, STORAGE_HOST_CB* host_cb);
void storage_unregister_handler(STORAGE* storage, STORAGE_HOST_CB* host_cb);
void storage_allocate_queue(STORAGE* storage, STORAGE_QUEUE_PARAMS* params);

void storage_disable_media_removal(STORAGE* storage);
void storage_enable_media_removal(STORAGE* storage);
void storage_stop(STORAGE* storage);

bool storage_check_media(STORAGE* storage);
//doesn't call driver to check media, just return last status
bool storage_is_media_present(STORAGE* storage);
void storage_cancel_io(STORAGE* storage);
void storage_write_cache(STORAGE* storage);
STORAGE_STATUS storage_read(STORAGE* storage, unsigned long addr, unsigned long count);
STORAGE_STATUS storage_write(STORAGE* storage, unsigned long addr, unsigned long count);
STORAGE_STATUS storage_verify(STORAGE* storage, unsigned long addr, unsigned long count, bool byte_compare);
//io controlling api
char* storage_io_buf_allocate(STORAGE* storage);
void storage_io_buf_release(STORAGE* storage, char* buf);
void storage_io_buf_filled(STORAGE* storage, char* buf, unsigned long size);

#endif // STORAGE_H
