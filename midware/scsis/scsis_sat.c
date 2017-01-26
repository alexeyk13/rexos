/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "scsis_sat.h"
#include "scsis_private.h"
#include "../../userspace/stdio.h"
#include "sys_config.h"

void scsis_sat_ata_pass_through12(SCSIS* scsis, uint8_t* req)
{
#if (SCSI_DEBUG_REQUESTS)
    uint8_t proto = (req[1] >> 1) & 0xf;
    printf("SCSI ata pass through (12) STUB, protocol: %d\n", proto);
#endif //SCSI_DEBUG_REQUESTS
    scsis_fail(scsis, SENSE_KEY_ILLEGAL_REQUEST, ASCQ_INVALID_COMMAND_OPERATION_CODE);
}

void scsis_sat_ata_pass_through16(SCSIS* scsis, uint8_t* req)
{
#if (SCSI_DEBUG_REQUESTS)
    uint8_t proto = (req[1] >> 1) & 0xf;
    printf("SCSI ata pass through (16) STUB, protocol: %d\n", proto);
#endif //SCSI_DEBUG_REQUESTS
    scsis_fail(scsis, SENSE_KEY_ILLEGAL_REQUEST, ASCQ_INVALID_COMMAND_OPERATION_CODE);
}
