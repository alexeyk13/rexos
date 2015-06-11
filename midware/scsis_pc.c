/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "scsis_pc.h"
#include "scsis_private.h"
#include "sys_config.h"
#include "../userspace/stdio.h"

SCSIS_RESPONSE scsis_pc_inquiry(SCSIS* scsis, uint8_t* req, IO* io)
{
#if (SCSI_DEBUG_REQUESTS)
    printf("SCSI INQUIRY\n\r");
#endif //SCSI_DEBUG_REQUESTS
    //TODO:
    return SCSIS_RESPONSE_FAIL;
}
