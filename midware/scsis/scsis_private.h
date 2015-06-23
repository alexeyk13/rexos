/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef SCSIS_PRIVATE_H
#define SCSIS_PRIVATE_H

#include <stdint.h>
#include "sys_config.h"
#include "../userspace/rb.h"
#include "../userspace/io.h"
#include "../userspace/scsi.h"
#include "scsis.h"

typedef struct {
    uint8_t key_sense;
    uint8_t align;
    uint16_t ascq;
} SCSIS_ERROR;

typedef struct _SCSIS {
    SCSI_STORAGE_DESCRIPTOR** storage;
    void* media;
    bool storage_request;
    SCSIS_ERROR errors[SCSI_SENSE_DEPTH];
    RB rb_error;
} SCSIS;

void scsis_error_init(SCSIS* scsis);
void scsis_error(SCSIS* scsis, uint8_t key_sense, uint16_t ascq);
void scsis_error_get(SCSIS* scsis, SCSIS_ERROR error);

SCSIS_RESPONSE scsis_request_storage(SCSIS* scsis, IO* io);


#endif // SCSIS_PRIVATE_H
