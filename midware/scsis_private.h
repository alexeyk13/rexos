/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef SCSIS_PRIVATE_H
#define SCSIS_PRIVATE_H

#include <stdint.h>
#include "../userspace/rb.h"

typedef struct {
    uint8_t key_sense;
    uint8_t align;
    uint16_t ascq;
} SCSIS_ERROR;

typedef struct _SCSIS {
    void* storage;
    void* media;
    //TODO: size
    SCSIS_ERROR errors[1];
    RB rb_error;
} SCSIS;

#endif // SCSIS_PRIVATE_H
