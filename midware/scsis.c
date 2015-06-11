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
    //TODO:
}

SCSIS* scsis_create()
{
    SCSIS* scsis = malloc(sizeof(SCSIS));
    if (scsis == NULL)
        return NULL;
    scsis->storage = NULL;
    scsis->media = NULL;
    scsis_reset(scsis);
    return scsis;
}

SCSIS_RESPONSE scsis_request(SCSIS* scsis, uint8_t* req, unsigned int size, IO* io)
{
    SCSIS_RESPONSE resp = SCSIS_RESPONSE_FAIL;
    //TODO: if less 6 fail error
    switch (req[0])
    {
    case SCSI_CMD_INQUIRY:
        //TODO: if request less 6 return
        resp = scsis_pc_inquiry(scsis, req, io);
        break;
    default:
        //TODO: request dump here, sense error
        break;
    }

    //TODO: remove me
    printf("scsis request\n\r");
    int i;
    for (i = 0; i < size; ++i)
        printf(" %02x", req[i]);
    printf(" (%d)\n\r", size);
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
