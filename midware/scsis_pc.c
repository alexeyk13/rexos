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
/*    int i;

    char* buf = storage_io_buf_allocate(scsi->storage);
    int len = 36;
    memset(buf, 0, len);
    buf[0] = (char)scsi->descriptor->scsi_device_type;
    buf[1] = storage_get_device_descriptor(scsi->storage)->flags & STORAGE_DEVICE_FLAG_REMOVABLE ? 0x80 : 0x00;
    buf[2] = 0x04; //spc-2
    buf[3] = 0x02; //flags
    buf[4] = len -5;

    strncpy(buf + 8, scsi->descriptor->vendor, 8);
    strncpy(buf + 16, scsi->descriptor->product, 16);
    strncpy(buf + 32, scsi->descriptor->revision, 4);
    for (i = 8; i < 36; ++i)
        if (buf[i] < ' ' || buf[i] > '~')
            buf[i] = ' ';

    storage_io_buf_filled(scsi->storage, buf, len);*/
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
        printd("evpd\n\r");
        //TODO: vital page request
    }
    else
    {
        res = scsis_pc_standart_inquiry(scsis, io);
    }
    return res;
}
