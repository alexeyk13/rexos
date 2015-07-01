/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef SCSIS_H
#define SCSIS_H

#include <stdint.h>
#include "../../userspace/io.h"

typedef struct _SCSIS SCSIS;

typedef enum {
    SCSIS_REQUEST_READ = 0,
    SCSIS_REQUEST_WRITE,
    SCSIS_REQUEST_VERIFY,
    //scsis storage requests
    SCSIS_REQUEST_GET_STORAGE_DESCRIPTOR,
    SCSIS_REQUEST_GET_MEDIA_DESCRIPTOR,
    //scsis host requests
    SCSIS_REQUEST_PASS,
    SCSIS_REQUEST_FAIL,
    SCSIS_REQUEST_INTERNAL_ERROR
} SCSIS_REQUEST;

typedef void (*SCSIS_CB)(void*, IO*, SCSIS_REQUEST);

SCSIS* scsis_create(SCSIS_CB cb_host, SCSIS_CB cb_storage, void* param);
void scsis_destroy(SCSIS* scsis);

//host interface
void scsis_reset(SCSIS* scsis);
void scsis_request(SCSIS* scsis, uint8_t* req);
void scsis_host_io_complete(SCSIS* scsis);
bool scsis_ready(SCSIS* scsis);

//storage interface
void scsis_media_removed(SCSIS* scsis);
void scsis_storage_io_complete(SCSIS* scsis);

#endif // SCSIS_H
