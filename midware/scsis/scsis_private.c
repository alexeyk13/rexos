/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "scsis_private.h"
#include "../../userspace/stdio.h"
#include "../../userspace/stdlib.h"
#include "sys_config.h"
#include <string.h>

void scsis_error_init(SCSIS* scsis)
{
    rb_init(&scsis->rb_error, SCSI_SENSE_DEPTH);
}

void scsis_error_put(SCSIS* scsis, uint8_t key_sense, uint16_t ascq)
{
    unsigned int idx;
    if (rb_is_full(&scsis->rb_error))
        rb_get(&scsis->rb_error);
    idx = rb_put(&scsis->rb_error);
    scsis->errors[idx].key_sense = key_sense;
    scsis->errors[idx].ascq = ascq;
#if (SCSI_DEBUG_ERRORS)
    printf("SCSI error: sense key: %02xh, ASC: %02xh, ASQ: %02xh\n", key_sense, ascq >> 8, ascq & 0xff);
#endif //SCSI_DEBUG_ERRORS
}

void scsis_error_get(SCSIS* scsis, SCSIS_ERROR *err)
{
    unsigned int idx;
    if (rb_is_empty(&scsis->rb_error))
    {
        err->key_sense = SENSE_KEY_NO_SENSE;
        err->ascq = ASCQ_NO_ADDITIONAL_SENSE_INFORMATION;
    }
    else
    {
        idx = rb_get(&scsis->rb_error);
        err->key_sense = scsis->errors[idx].key_sense;
        err->ascq = scsis->errors[idx].ascq;
    }
}

void scsis_cb_host(SCSIS* scsis, SCSIS_RESPONSE response, unsigned int size)
{
    scsis->cb_host(scsis->param, scsis->id, response, size);
}

static void scsis_done(SCSIS* scsis, SCSIS_RESPONSE resp)
{
    bool was_ready = false;
#if (SCSI_WRITE_CACHE)
    if (scsis->state == SCSIS_STATE_WRITE && (scsis->count == 0))
        was_ready = true;
#endif //SCSI_WRITE_CACHE
    scsis->state = SCSIS_STATE_IDLE;
    if (scsis->need_media)
        storage_get_media_descriptor(scsis->storage_descriptor->hal, scsis->storage_descriptor->storage, scsis->storage_descriptor->user, scsis->io);
    else
    {
        scsis->io = NULL;
        scsis_cb_host(scsis, SCSIS_RESPONSE_RELEASE_IO, 0);
    }
    //pass already sent on write
    if (!was_ready)
        scsis_cb_host(scsis, resp, 0);
}

void scsis_fail(SCSIS* scsis, uint8_t key_sense, uint16_t ascq)
{
    scsis_error_put(scsis, key_sense, ascq);
    scsis_done(scsis, SCSIS_RESPONSE_FAIL);
}

void scsis_pass(SCSIS* scsis)
{
    scsis_done(scsis, SCSIS_RESPONSE_PASS);
}

bool scsis_get_media(SCSIS* scsis)
{
    if (scsis->media == NULL)
    {
        scsis_fail(scsis, SENSE_KEY_NOT_READY, ASCQ_MEDIUM_NOT_PRESENT);
        return false;
    }
    return true;
}
