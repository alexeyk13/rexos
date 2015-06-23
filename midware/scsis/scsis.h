/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef SCSIS_H
#define SCSIS_H

#include <stdint.h>
#include "../userspace/io.h"

typedef struct _SCSIS SCSIS;

typedef enum {
    SCSIS_RESPONSE_PASS = 0,
    SCSIS_RESPONSE_FAIL,
    SCSIS_RESPONSE_PHASE_ERROR,
    SCSIS_RESPONSE_STORAGE_REQUEST
} SCSIS_RESPONSE;

void scsis_reset(SCSIS* scsis);
SCSIS* scsis_create();
SCSIS_RESPONSE scsis_request(SCSIS* scsis, uint8_t* req, IO* io);
void scsis_destroy(SCSIS* scsis);

#endif // SCSIS_H
