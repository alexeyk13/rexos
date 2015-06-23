/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "scsis_pc.h"
#include "scsis_private.h"
#include "sys_config.h"
#include "../../userspace/stdio.h"
#include "../../userspace/scsi.h"
#include "../../userspace/endian.h"
#include <string.h>

static inline SCSIS_RESPONSE scsis_pc_standart_inquiry(SCSIS* scsis, IO* io)
{
    int i;
    uint8_t* data = io_data(io);
#if (SCSI_DEBUG_REQUESTS)
    printf("SCSI standart INQUIRY\n\r");
#endif //SCSI_DEBUG_REQUESTS

    io->data_size = 36;
    memset(data, 0, io->data_size);
    //0: peripheral qualifier: device present, peripheral device type
    data[0] = (*scsis->storage)->scsi_device_type & SCSI_PERIPHERAL_DEVICE_TYPE_MASK;
    //1: removable, lu_cong
    data[1] = (((*scsis->storage)->flags & SCSI_STORAGE_DESCRIPTOR_REMOVABLE) ? (1 << 7) : 0);
    //2: version. SPC4
    data[2] = 0x6;
    //3: normaca, hisup, response data formata
    data[3] = 0x2;
    //4: additional len
    data[4] = io->data_size - 5;
    //5: SCCS, ACC, TPGS, 3PC, PROTECT
    //6: ENC_SERV, VS, MULTI_P, ADDR16
    //7: WBUS16, SYNC, CMD_QUE, VS

    strncpy((char*)(data + 8), (*scsis->storage)->vendor, 8);
    strncpy((char*)(data + 16), (*scsis->storage)->product, 16);
    strncpy((char*)(data + 32), (*scsis->storage)->revision, 4);
    for (i = 8; i < 36; ++i)
        if (data[i] < ' ' || data[i] > '~')
            data[i] = ' ';
    return SCSIS_RESPONSE_PASS;
}

SCSIS_RESPONSE scsis_pc_inquiry(SCSIS* scsis, uint8_t* req, IO* io)
{
    unsigned int len;
    SCSIS_RESPONSE res = scsis_get_storage_descriptor(scsis, io);
    if (res != SCSIS_RESPONSE_PASS)
        return res;
    len = be2short(req + 3);
    if (req[1] & SCSI_INQUIRY_EVPD)
    {
        printd("evpd\n\r");
        //TODO: vital page request
    }
    else
        res = scsis_pc_standart_inquiry(scsis, io);
    if (io->data_size > len)
        io->data_size = len;
    return res;
}

SCSIS_RESPONSE scsis_pc_test_unit_ready(SCSIS* scsis, uint8_t* req, IO* io)
{
    SCSIS_RESPONSE res = scsis_get_media_descriptor(scsis, io);
    if (res != SCSIS_RESPONSE_PASS)
        return res;
#if (SCSI_DEBUG_REQUESTS)
    printf("SCSI test unit ready: %d\n\r", scsis->media ? 1 : 0);
#endif //SCSI_DEBUG_REQUESTS
    if (scsis->media == NULL)
    {
        scsis_error(scsis, SENSE_KEY_NOT_READY, ASCQ_MEDIUM_NOT_PRESENT);
        res = SCSIS_RESPONSE_FAIL;
    }
    return res;
}
