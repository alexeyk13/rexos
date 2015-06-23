/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "scsis_private.h"
#include "../../userspace/stdio.h"
#include "../../userspace/stdlib.h"
#include <string.h>

void scsis_error_init(SCSIS* scsis)
{
    rb_init(&scsis->rb_error, SCSI_SENSE_DEPTH);
}

void scsis_error(SCSIS* scsis, uint8_t key_sense, uint16_t ascq)
{
    unsigned int idx;
    if (rb_is_full(&scsis->rb_error))
        rb_get(&scsis->rb_error);
    idx = rb_put(&scsis->rb_error);
    scsis->errors[idx].key_sense = key_sense;
    scsis->errors[idx].ascq = ascq;
#if (SCSI_DEBUG_ERRORS)
    printf("SCSI error: sense key: %02xh, ASC: %02xh, ASQ: %02xh\n\r", key_sense, ascq >> 8, ascq & 0xff);
#endif //SCSI_DEBUG_ERRORS
}

void scsis_error_get(SCSIS* scsis, SCSIS_ERROR error)
{
    unsigned int idx;
    if (rb_is_empty(&scsis->rb_error))
    {
        error.key_sense = SENSE_KEY_NO_SENSE;
        error.ascq = ASCQ_NO_ADDITIONAL_SENSE_INFORMATION;
    }
    else
    {
        idx = rb_get(&scsis->rb_error);
        error.key_sense = scsis->errors[idx].key_sense;
        error.ascq = scsis->errors[idx].ascq;
    }
}

SCSIS_RESPONSE scsis_get_storage_descriptor(SCSIS* scsis, IO *io)
{
    SCSI_STACK* stack;
    SCSI_REQUEST req;
    if (scsis->storage)
        return SCSIS_RESPONSE_PASS;
    if (scsis->storage_request)
    {
        scsis->storage_request = false;
        stack = io_stack(io);
        req = stack->request;
        io_pop(io, sizeof(SCSI_STACK));
        if (req != SCSI_REQUEST_STORAGE_DESCRIPTOR || io->data_size < sizeof(void*))
            return SCSIS_RESPONSE_PHASE_ERROR;
        scsis->storage = malloc(io->data_size);

        if (scsis->storage == NULL)
            return SCSIS_RESPONSE_PHASE_ERROR;
        memcpy(scsis->storage, io_data(io), io->data_size);
        return SCSIS_RESPONSE_PASS;
    }

    //make request
    stack = io_push(io, sizeof(SCSI_STACK));
    if (stack == NULL)
        return SCSIS_RESPONSE_PHASE_ERROR;
    stack->request = SCSI_REQUEST_STORAGE_DESCRIPTOR;
    scsis->storage_request = true;
    return SCSIS_RESPONSE_STORAGE_REQUEST;
}

SCSIS_RESPONSE scsis_get_media_descriptor(SCSIS* scsis, IO* io)
{
    SCSI_STACK* stack;
    SCSI_REQUEST req;
    if (scsis->media)
        return SCSIS_RESPONSE_PASS;
    if (scsis->storage_request)
    {
        scsis->storage_request = false;
        stack = io_stack(io);
        req = stack->request;
        io_pop(io, sizeof(SCSI_STACK));
        if (req != SCSI_REQUEST_MEDIA_DESCRIPTOR)
            return SCSIS_RESPONSE_PHASE_ERROR;
        //no media present, not error
        if (stack->size < 0)
            return SCSIS_RESPONSE_PASS;
        if (io->data_size < sizeof(void*))
            return SCSIS_RESPONSE_PHASE_ERROR;

        scsis->media = malloc(io->data_size);
        if (scsis->media == NULL)
            return SCSIS_RESPONSE_PHASE_ERROR;
        memcpy(scsis->media, io_data(io), io->data_size);
        return SCSIS_RESPONSE_PASS;
    }

    //make request
    stack = io_push(io, sizeof(SCSI_STACK));
    if (stack == NULL)
        return SCSIS_RESPONSE_PHASE_ERROR;
    stack->request = SCSI_REQUEST_MEDIA_DESCRIPTOR;
    scsis->storage_request = true;
    return SCSIS_RESPONSE_STORAGE_REQUEST;
}
