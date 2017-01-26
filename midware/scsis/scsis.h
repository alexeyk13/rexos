/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
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
    SCSIS_RESPONSE_NEED_IO,
    SCSIS_RESPONSE_RELEASE_IO
} SCSIS_RESPONSE;

typedef void (*SCSIS_CB)(void*, unsigned int, SCSIS_RESPONSE, unsigned int);

SCSIS* scsis_create(SCSIS_CB cb_host, void* param, unsigned int id, SCSI_STORAGE_DESCRIPTOR* storage_descriptor);
void scsis_destroy(SCSIS* scsis);

//host interface
void scsis_init(SCSIS* scsis);
void scsis_reset(SCSIS* scsis);
void scsis_request_cmd(SCSIS* scsis, IO* io, uint8_t* req);
void scsis_host_io_complete(SCSIS* scsis, int resp_size);
void scsis_host_give_io(SCSIS* scsis, IO* io);
void scsis_request(SCSIS* scsis, IPC* ipc);

#endif // SCSIS_H
