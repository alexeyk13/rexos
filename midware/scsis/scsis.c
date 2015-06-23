/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "scsis.h"
#include "scsis_private.h"
#include "scsis_pc.h"
#include "../../userspace/stdlib.h"
#include "../../userspace/stdio.h"
#include "../../userspace/scsi.h"

void scsis_reset(SCSIS* scsis)
{
    scsis->storage_request = false;
    //TODO: ?
}

SCSIS* scsis_create()
{
    SCSIS* scsis = malloc(sizeof(SCSIS));
    if (scsis == NULL)
        return NULL;
    scsis->storage = NULL;
    scsis->media = NULL;
    scsis_error_init(scsis);
    scsis_reset(scsis);
    return scsis;
}

SCSIS_RESPONSE scsis_request(SCSIS* scsis, uint8_t* req, IO* io)
{
    SCSIS_RESPONSE res = SCSIS_RESPONSE_FAIL;
    switch (req[0])
    {
    case SCSI_CMD_TEST_UNIT_READY:
        res = scsis_pc_test_unit_ready(scsis, req, io);
        break;
    case SCSI_CMD_INQUIRY:
        res = scsis_pc_inquiry(scsis, req, io);
        break;
    default:
#if (SCSI_DEBUG_ERRORS)
        printf("SCSI: unknown cmd opcode: %02xh\n\r", req[0]);
#endif //SCSI_DEBUG_ERRORS
        scsis_error(scsis, SENSE_KEY_ILLEGAL_REQUEST, ASCQ_INVALID_COMMAND_OPERATION_CODE);
        break;
    }
    return res;
}

void scsis_media_removed(SCSIS* scsis)
{
    scsis->media = NULL;
}

void scsis_destroy(SCSIS* scsis)
{
    free(scsis);
}
