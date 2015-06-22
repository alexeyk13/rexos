/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "scsis.h"
#include "scsis_private.h"
#include "scsis_pc.h"
#include "../userspace/stdlib.h"
#include "../userspace/stdio.h"
#include "../userspace/scsi.h"

void scsis_reset(SCSIS* scsis)
{
    scsis_error_reset(scsis);
    //TODO:
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
    SCSIS_RESPONSE resp = SCSIS_RESPONSE_FAIL;
    switch (req[0])
    {
    case SCSI_CMD_INQUIRY:
        resp = scsis_pc_inquiry(scsis, req, io);
        break;
    default:
        scsis_error(scsis, SENSE_KEY_ILLEGAL_REQUEST, ASCQ_INVALID_COMMAND_OPERATION_CODE);
#if (SCSI_DEBUG_ERRORS)
        printf("SCSI: unknown cmd opcode: %02xh\n\r", req[0]);
#endif //SCSI_DEBUG_ERRORS
        break;
    }
    return resp;
}

SCSIS_RESPONSE scsis_storage_response(SCSIS* scsis, IO* io)
{
    printd("scsis storage response\n\r");
    return SCSIS_RESPONSE_FAIL;
}


void scsis_destroy(SCSIS* scsis)
{
    free(scsis);
}
