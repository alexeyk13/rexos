/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef SCSI_H
#define SCSI_H

#include "scsi_defs.h"
#include "storage.h"
#include "kernel_config.h"

typedef struct {
	uint8_t scsi_device_type;
	const char* const vendor;
	const char* const product;
	const char* const revision;
	const char* const device_id;
}SCSI_DESCRIPTOR, *P_SCSI_DESCRIPTOR;

typedef struct {
	SCSI_CMD cmd;
	SCSI_DESCRIPTOR* descriptor;
	STORAGE* storage;
	SCSI_ERROR error_queue[SCSI_ERROR_BUF_SIZE];
	uint8_t error_head, error_tail;
}SCSI;

SCSI* scsi_create(STORAGE* storage, SCSI_DESCRIPTOR* descriptor);
void scsi_destroy(SCSI* scsi);
void scsi_reset(SCSI* scsi);

bool scsi_cmd(SCSI* scsi, char* cmd_buf, uint8_t cmd_size);

#endif // SCSI_H
