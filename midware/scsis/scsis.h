/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2016, Alexey Kramarenko
    All rights reserved.
*/

#ifndef SCSIS_H
#define SCSIS_H

#include <stdint.h>
#include "../../userspace/io.h"
#include "../../userspace/scsi.h"

typedef struct _SCSIS SCSIS;

typedef enum {
    //request to read or write more data
    SCSIS_RESPONSE_READ = 0,
    SCSIS_RESPONSE_WRITE,
    //operation completed: pass or fail
    SCSIS_RESPONSE_PASS,
    SCSIS_RESPONSE_FAIL,
    //ready for next after asynchronous write complete
    SCSIS_RESPONSE_READY,
    //TODO: this will be removed
    SCSIS_RESPONSE_VERIFY,
    SCSIS_RESPONSE_WRITE_VERIFY,
} SCSIS_RESPONSE;

typedef void (*SCSIS_CB)(void*, IO*, SCSIS_RESPONSE, unsigned int);

SCSIS* scsis_create(SCSIS_CB cb_host, void* param, SCSI_STORAGE_DESCRIPTOR* storage_descriptor);
void scsis_destroy(SCSIS* scsis);

//host interface
void scsis_reset(SCSIS* scsis);
bool scsis_is_ready(SCSIS* scsis);
bool scsis_request_cmd(SCSIS* scsis, uint8_t* req);
void scsis_host_io_complete(SCSIS* scsis, int resp_size);
void scsis_request(SCSIS* scsis, IPC* ipc);

//TODO: move to request
//storage interface
void scsis_media_removed(SCSIS* scsis);

#endif // SCSIS_H
