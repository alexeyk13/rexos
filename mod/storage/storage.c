/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "storage.h"
#include <string.h>
#include "mem_private.h"
#include "error.h"
#include "event.h"
#include "dbg.h"
#include "queue.h"
#include "kernel_config.h"

STORAGE* storage_create(const STORAGE_DEVICE_DESCRIPTOR *device_descriptor, STORAGE_DRIVER_CB *driver_cb,
							void *driver_param)
{
	STORAGE* storage = (STORAGE*)sys_alloc(sizeof(STORAGE));
	if (storage)
	{
		storage->rx_event = event_create();
		storage->tx_event = event_create();
		storage->device_descriptor = device_descriptor;

		storage->driver_cb = driver_cb;
		storage->driver_param = driver_param;

		storage->media_descriptor = NULL;

		storage->host_cb = NULL;
		storage->host_io_cb = NULL;
		storage->host_io_param = NULL;
		storage->reading = false;
		storage->writing = false;
		storage->queue = 0;
		storage->block_size = 0;
	}
	else
		error(ERROR_MEM_OUT_OF_SYSTEM_MEMORY, "STORAGE");
	return storage;
}

void storage_destroy(STORAGE* storage)
{
	event_destroy(storage->tx_event);
	event_destroy(storage->rx_event);
	sys_free(storage);
}

void storage_update_state(STORAGE* storage)
{
	STORAGE_STATE state = STORAGE_STATE_IDLE;
	if (storage->reading && storage->writing)
		state = STORAGE_STATE_READ_WRITE;
	else if (storage->writing)
		state = STORAGE_STATE_WRITE;
	else if (storage->reading)
		state = STORAGE_STATE_READ;

	STORAGE_HOST_CB* cur;
	DLIST_ENUM de;
	dlist_enum_start((DLIST**)&storage->host_cb, &de);
	while (dlist_enum(&de, (DLIST**)&cur))
	{
		if (cur->state_changed)
			cur->state_changed(cur->param, state);
	}
}

const STORAGE_DEVICE_DESCRIPTOR* storage_get_device_descriptor(STORAGE* storage)
{
	return storage->device_descriptor;
}

const STORAGE_MEDIA_DESCRIPTOR *storage_get_media_descriptor(STORAGE* storage)
{
	return storage->media_descriptor;
}

void storage_insert_media(STORAGE* storage, STORAGE_MEDIA_DESCRIPTOR* media)
{
	storage->media_descriptor = media;
	STORAGE_HOST_CB* cur;
	DLIST_ENUM de;
	dlist_enum_start((DLIST**)&storage->host_cb, &de);
	while (dlist_enum(&de, (DLIST**)&cur))
	{
		if (cur->media_state_changed)
			cur->media_state_changed(cur->param, STORAGE_MEDIA_INSERTED);
	}
}

void storage_eject_media(STORAGE* storage)
{
	if (storage->media_descriptor)
	{
		bool surprisal = storage->media_descriptor->flags & STORAGE_MEDIA_FLAG_REMOVAL_DISABLED ? true : false;
		storage->reading = false;
		storage->writing = false;
		storage_update_state(storage);
		storage->media_descriptor = NULL;
		STORAGE_HOST_CB* cur;
		DLIST_ENUM de;
		dlist_enum_start((DLIST**)&storage->host_cb, &de);
		while (dlist_enum(&de, (DLIST**)&cur))
		{
			if (cur->media_state_changed)
				cur->media_state_changed(cur->param, surprisal ? STORAGE_MEDIA_SURPRISAL_REMOVED : STORAGE_MEDIA_REMOVED);
		}
	}
}

void storage_blocks_readed(STORAGE* storage, STORAGE_STATUS status)
{
	storage->read_status = status;
	event_set(storage->rx_event);
}

void storage_blocks_writed(STORAGE* storage, STORAGE_STATUS status)
{
	storage->write_status = status;
	event_set(storage->tx_event);
}

void storage_register_handler(STORAGE* storage, STORAGE_HOST_CB* host_cb)
{
	dlist_add_tail((DLIST**)&storage->host_cb, (DLIST*)host_cb);
}

void storage_unregister_handler(STORAGE* storage, STORAGE_HOST_CB* host_cb)
{
	dlist_remove((DLIST**)&storage->host_cb, (DLIST*)host_cb);
}

void storage_allocate_queue(STORAGE* storage, STORAGE_QUEUE_PARAMS* params)
{
	storage->block_size = params->block_size;
	storage->queue = params->queue;
	storage->sectors_in_block = params->block_size / storage->device_descriptor->sector_size;
	ASSERT(storage->sectors_in_block * storage->device_descriptor->sector_size == storage->block_size);
	storage->host_io_cb = params->host_io_cb;
	storage->host_io_param = params->host_io_param;
}

void storage_disable_media_removal(STORAGE* storage)
{
	if (storage->media_descriptor)
		storage->media_descriptor->flags |= STORAGE_MEDIA_FLAG_REMOVAL_DISABLED;
}

void storage_enable_media_removal(STORAGE* storage)
{
	if (storage->media_descriptor)
		storage->media_descriptor->flags &= ~STORAGE_MEDIA_FLAG_REMOVAL_DISABLED;
}

void storage_stop(STORAGE* storage)
{
	if (storage->media_descriptor)
	{
		storage->media_descriptor = NULL;
		if (storage->driver_cb->on_stop)
			storage->driver_cb->on_stop(storage->driver_param);
		STORAGE_HOST_CB* cur;
		DLIST_ENUM de;
		dlist_enum_start((DLIST**)&storage->host_cb, &de);
		while (dlist_enum(&de, (DLIST**)&cur))
		{
			if (cur->media_state_changed)
				cur->media_state_changed(cur->param, STORAGE_MEDIA_REMOVED);
		}
	}
}

bool storage_check_media(STORAGE* storage)
{
	bool res = false;
	if (storage->driver_cb->on_storage_check_media)
		res = storage->driver_cb->on_storage_check_media(storage->driver_param);
	return res;
}

bool storage_is_media_present(STORAGE* storage)
{
	return storage->media_descriptor != NULL;
}

void storage_cancel_io(STORAGE* storage)
{
	if (storage->reading || storage->writing)
	{
		if (storage->driver_cb->on_cancel_io)
			storage->driver_cb->on_cancel_io(storage->driver_param);
		storage->reading = false;
		storage->writing = false;
		storage_update_state(storage);
	}
}

void storage_write_cache(STORAGE* storage)
{
	if (storage->driver_cb->on_write_cache)
		storage->driver_cb->on_write_cache(storage->driver_param);
}

STORAGE_STATUS storage_read(STORAGE* storage, unsigned long addr, unsigned long count)
{
	storage->read_status = STORAGE_STATUS_OK;
	if (!storage->reading)
	{
		if (storage->media_descriptor)
		{
			if ((storage->media_descriptor->flags & STORAGE_MEDIA_FLAG_READ_PROTECTION) == 0)
			{
				if (addr < storage->media_descriptor->num_sectors && addr + count <= storage->media_descriptor->num_sectors)
				{
					if (storage->driver_cb->on_storage_prepare_read)
						storage->read_status = storage->driver_cb->on_storage_prepare_read(storage->driver_param, addr, count);
					if (storage->read_status == STORAGE_STATUS_OK)
					{
						storage->reading = true;
						storage_update_state(storage);
						unsigned long cur_addr = addr;
						unsigned long sectors_left = count;
						unsigned long sectors_cur;
						char* buf;
						int retry = STORAGE_RETRY_COUNT;
						while (storage->read_status == STORAGE_STATUS_OK && sectors_left)
						{
							sectors_cur = storage->sectors_in_block;
							if (sectors_cur > sectors_left)
								sectors_cur = sectors_left;

							buf = queue_allocate_buffer_ms(storage->queue, INFINITE);

							event_clear(storage->rx_event);
							storage->read_status = storage->driver_cb->on_storage_read_blocks(storage->driver_param, cur_addr, buf, sectors_cur);
							if (storage->read_status == STORAGE_STATUS_OK)
								event_wait_ms(storage->rx_event, INFINITE);

							if (storage->read_status == STORAGE_STATUS_OK)
							{
								queue_push(storage->queue, buf);
								storage->host_io_cb->on_storage_buffer_filled(storage->host_io_param, sectors_cur * storage->device_descriptor->sector_size);
								retry = STORAGE_RETRY_COUNT;
								sectors_left -= sectors_cur;
								cur_addr += sectors_cur;
							}
							else
							{
								queue_release_buffer(storage->queue, buf);
								storage_cancel_io(storage);
								while (retry-- && (storage->read_status == STORAGE_STATUS_TIMEOUT || storage->read_status == STORAGE_STATUS_CRC_ERROR))
								{
									storage->read_status = STORAGE_STATUS_OK;
									if (storage->driver_cb->on_storage_prepare_read)
										storage->read_status = storage->driver_cb->on_storage_prepare_read(storage->driver_param, cur_addr, sectors_left);
								}
							}
						}
						storage->reading = false;
						storage_update_state(storage);
						if (storage->driver_cb->on_storage_read_done && sectors_left == 0)
							storage->read_status = storage->driver_cb->on_storage_read_done(storage->driver_param);
					}
				}
				else
					storage->read_status = STORAGE_STATUS_INVALID_ADDRESS;
			}
			else
				storage->read_status = STORAGE_STATUS_DATA_PROTECTED;
		}
		else
			storage->read_status = STORAGE_STATUS_NO_MEDIA;

	}
	else
	{
		storage_cancel_io(storage);
		storage->read_status = STORAGE_STATUS_OPERATION_IN_PROGRESS;
	}
	return storage->read_status;
}

STORAGE_STATUS storage_write(STORAGE* storage, unsigned long addr, unsigned long count)
{
	storage->write_status = STORAGE_STATUS_OK;
	bool write_finished;
	if (!storage->writing)
	{
		if (storage->media_descriptor)
		{
			if ((storage->media_descriptor->flags & STORAGE_MEDIA_FLAG_WRITE_PROTECTION) == 0)
			{
				if (addr < storage->media_descriptor->num_sectors && addr + count <= storage->media_descriptor->num_sectors)
				{
					if (storage->driver_cb->on_storage_prepare_write)
						storage->write_status = storage->driver_cb->on_storage_prepare_write(storage->driver_param, addr, count);
					if (storage->write_status == STORAGE_STATUS_OK)
					{
						storage->writing = true;
						storage_update_state(storage);
						unsigned long cur_addr = addr;
						unsigned long sectors_left = count;
						unsigned long sectors_cur;
						char* buf;
						//double-buffering
						storage->host_io_cb->on_storage_request_buffers(storage->host_io_param, sectors_left > storage->sectors_in_block ? storage->block_size :
							sectors_left * storage->device_descriptor->sector_size);
						int retry = STORAGE_RETRY_COUNT;
						while (storage->write_status == STORAGE_STATUS_OK && sectors_left)
						{
							if (sectors_left > storage->sectors_in_block)
								storage->host_io_cb->on_storage_request_buffers(storage->host_io_param, (sectors_left - storage->sectors_in_block > storage->sectors_in_block) ?
									 storage->block_size : (sectors_left - storage->sectors_in_block) * storage->device_descriptor->sector_size);
							sectors_cur = storage->sectors_in_block;
							if (sectors_cur > sectors_left)
								sectors_cur = sectors_left;
							buf = queue_pull_ms(storage->queue, INFINITE);

							write_finished = false;
							while (retry && !write_finished)
							{
								event_clear(storage->tx_event);
								storage->write_status = storage->driver_cb->on_storage_write_blocks(storage->driver_param, cur_addr, buf, sectors_cur);
								if (storage->write_status == STORAGE_STATUS_OK)
								{
									event_wait_ms(storage->tx_event, INFINITE);
									if (storage->write_status == STORAGE_STATUS_OK)
									{
										queue_release_buffer(storage->queue, buf);
										sectors_left -= sectors_cur;
										cur_addr += sectors_cur;
										write_finished = true;
									}
								}
								if (storage->write_status != STORAGE_STATUS_OK)
								{
									storage_cancel_io(storage);
									while (retry-- && (storage->write_status == STORAGE_STATUS_TIMEOUT || storage->write_status == STORAGE_STATUS_CRC_ERROR))
									{
										storage->write_status = STORAGE_STATUS_OK;
										if (storage->driver_cb->on_storage_prepare_write)
											storage->write_status = storage->driver_cb->on_storage_prepare_write(storage->driver_param, cur_addr, sectors_left);
									}
								}
							}
						}
						storage->writing = false;
						storage_update_state(storage);
						if (storage->driver_cb->on_storage_write_done && sectors_left == 0)
							storage->driver_cb->on_storage_write_done(storage->driver_param);
					}
				}
				else
					storage->write_status = STORAGE_STATUS_INVALID_ADDRESS;
			}
			else
				storage->write_status = STORAGE_STATUS_DATA_PROTECTED;
		}
		else
			storage->write_status = STORAGE_STATUS_NO_MEDIA;

	}
	else
	{
		storage_cancel_io(storage);
		storage->write_status = STORAGE_STATUS_OPERATION_IN_PROGRESS;
	}
	return storage->write_status;
}

STORAGE_STATUS storage_verify(STORAGE* storage, unsigned long addr, unsigned long count, bool byte_compare)
{
	storage->read_status = STORAGE_STATUS_OK;
	int retry = STORAGE_RETRY_COUNT;
	if (!storage->reading)
	{
		if (storage->media_descriptor)
		{
			if ((storage->media_descriptor->flags & STORAGE_MEDIA_FLAG_READ_PROTECTION) == 0)
			{
				if (addr < storage->media_descriptor->num_sectors && addr + count <= storage->media_descriptor->num_sectors)
				{
					if (storage->driver_cb->on_storage_prepare_read)
						storage->read_status = storage->driver_cb->on_storage_prepare_read(storage->driver_param, addr, count);
					if (storage->read_status == STORAGE_STATUS_OK)
					{
						storage->reading = true;
						storage_update_state(storage);
						unsigned long cur_addr = addr;
						unsigned long sectors_left = count;
						unsigned long sectors_cur;
						char* buf;
						char* verify_buf = queue_allocate_buffer_ms(storage->queue, INFINITE);

						while (storage->read_status == STORAGE_STATUS_OK && sectors_left)
						{
							sectors_cur = storage->sectors_in_block;
							if (sectors_cur > sectors_left)
								sectors_cur = sectors_left;
							event_clear(storage->rx_event);
							storage->read_status = storage->driver_cb->on_storage_read_blocks(storage->driver_param, cur_addr, verify_buf, sectors_cur);
							if (storage->read_status == STORAGE_STATUS_OK)
								event_wait_ms(storage->rx_event, INFINITE);

							if (storage->read_status == STORAGE_STATUS_OK)
							{
								if (byte_compare)
								{
									storage->host_io_cb->on_storage_request_buffers(storage->host_io_param, sectors_cur * storage->device_descriptor->sector_size);
									buf = queue_pull_ms(storage->queue, INFINITE);
									if (memcmp(verify_buf, buf, sectors_cur * storage->device_descriptor->sector_size) != 0)
										storage->read_status = STORAGE_STATUS_MISCOMPARE;
									queue_release_buffer(storage->queue, buf);
								}
								sectors_left -= sectors_cur;
								cur_addr += sectors_cur;
							}
							else
							{
								while (retry && (storage->read_status == STORAGE_STATUS_TIMEOUT || storage->read_status == STORAGE_STATUS_CRC_ERROR))
								{
									--retry;
									storage->read_status = STORAGE_STATUS_OK;
									if (storage->driver_cb->on_storage_prepare_read)
										storage->read_status = storage->driver_cb->on_storage_prepare_read(storage->driver_param, cur_addr, sectors_left);
								}
							}
						}
						if (sectors_left)
							storage_cancel_io(storage);
						else
						{
							storage->reading = false;
							storage_update_state(storage);
						}
						if (storage->driver_cb->on_storage_read_done)
							storage->driver_cb->on_storage_read_done(storage->driver_param);

						queue_release_buffer(storage->queue, verify_buf);
					}
				}
				else
					storage->read_status = STORAGE_STATUS_INVALID_ADDRESS;
			}
			else
				storage->read_status = STORAGE_STATUS_DATA_PROTECTED;
		}
		else
			storage->read_status = STORAGE_STATUS_NO_MEDIA;

	}
	else
	{
		storage_cancel_io(storage);
		storage->read_status = STORAGE_STATUS_OPERATION_IN_PROGRESS;
	}
	return storage->read_status;
}

char* storage_io_buf_allocate(STORAGE* storage)
{
	return queue_allocate_buffer_ms(storage->queue, INFINITE);
}

void storage_io_buf_release(STORAGE* storage, char* buf)
{
	queue_release_buffer(storage->queue, buf);
}

void storage_io_buf_filled(STORAGE* storage, char* buf, unsigned long size)
{
	queue_push(storage->queue, buf);
	storage->host_io_cb->on_storage_buffer_filled(storage->host_io_param, size);
}
