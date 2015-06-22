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

static inline SCSIS_RESPONSE scsis_pc_standart_inquiry(SCSIS* scsis, IO* io)
{
#if (SCSI_DEBUG_REQUESTS)
    printf("SCSI standart INQUIRY\n\r");
#endif //SCSI_DEBUG_REQUESTS
    //TODO:
    return SCSIS_RESPONSE_PHASE_ERROR;
}

SCSIS_RESPONSE scsis_pc_inquiry(SCSIS* scsis, uint8_t* req, IO* io)
{
    SCSIS_RESPONSE res = scsis_request_storage(scsis, io);
    if (res != SCSIS_RESPONSE_PASS)
        return res;
    if (req[1] & SCSI_INQUIRY_EVPD)
    {
        //TODO: vital page request
    }
    else
    {
        //TODO: data size
        res = scsis_pc_standart_inquiry(scsis, io);
    }
    return res;
}
