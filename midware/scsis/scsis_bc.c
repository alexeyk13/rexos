/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "scsis_bc.h"
#include "scsis_private.h"
#include "sys_config.h"
#include "../../userspace/stdio.h"
#include "../../userspace/scsi.h"
#include "../../userspace/endian.h"

SCSIS_RESPONSE scsis_bc_read_capacity10(SCSIS* scsis, uint8_t* req, IO* io)
{
    SCSIS_RESPONSE res = scsis_get_media(scsis, io);
    unsigned int lba;
    if (res != SCSIS_RESPONSE_PASS)
        return res;
#if (SCSI_DEBUG_REQUESTS)
    printf("SCSI read capacity (10)\n\r");
#endif //SCSI_DEBUG_REQUESTS
    lba = be2int(req + 2);
    if (lba && ((req[8] & SCSI_READ_CAPACITY_PMI) == 0))
    {
        scsis_error(scsis, SENSE_KEY_ILLEGAL_REQUEST, ASCQ_INVALID_FIELD_IN_CDB);
        return SCSIS_RESPONSE_FAIL;
    }
    if ((*scsis->media)->num_sectors_hi)
        lba = 0xffffffff;
    else if ((*scsis->media)->num_sectors_hi > lba)
        lba = (*scsis->media)->num_sectors_hi - 1;
    int2be(io_data(io) + 0, lba);
    int2be(io_data(io) + 4, (*scsis->media)->sector_size);
    io->data_size = 8;

    return SCSIS_RESPONSE_PASS;
}
