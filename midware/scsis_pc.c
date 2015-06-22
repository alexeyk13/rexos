/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "scsis_pc.h"
#include "scsis_private.h"
#include "sys_config.h"
#include "../userspace/stdio.h"
#include "../userspace/scsi.h"

static SCSIS_RESPONSE scsis_pc_send_standart_inquiry(SCSIS* scsis, IO* io)
{
    //TODO:
    return SCSIS_RESPONSE_PHASE_ERROR;
//    return SCSIS_RESPONSE_PASS;
}

static inline SCSIS_RESPONSE scsis_pc_standart_inquiry(SCSIS* scsis, IO* io)
{
#if (SCSI_DEBUG_REQUESTS)
    printf("SCSI standart INQUIRY\n\r");
#endif //SCSI_DEBUG_REQUESTS
    if (scsis->storage == NULL)
    {
        //TODO: user request
        return SCSIS_RESPONSE_STORAGE_REQUEST;
    }
    return scsis_pc_send_standart_inquiry(scsis, io);
}

SCSIS_RESPONSE scsis_pc_inquiry(SCSIS* scsis, uint8_t* req, IO* io)
{
    if (scsis->storage == NULL)
    {

    }
    SCSIS_RESPONSE res = SCSIS_RESPONSE_PHASE_ERROR;
    if (req[1] & SCSI_INQUIRY_EVPD)
    {
        //TODO: vital page request
    }
    else
        res = scsis_pc_standart_inquiry(scsis, io);
    return res;
}
