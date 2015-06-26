/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "scsis_pc.h"
#include "scsis_bc.h"
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
    return SCSIS_RESPONSE_PASS;
}

/*
    mode sense header

    0 len - 1
    1 medium type
    2 device specific
    3 device descriptor size
 */

SCSIS_RESPONSE scsis_pc_mode_sense6(SCSIS* scsis, uint8_t* req, IO* io)
{
    unsigned int psp, len;
    bool dbd;
    SCSIS_RESPONSE res = scsis_get_media_descriptor(scsis, io);
    if (res != SCSIS_RESPONSE_PASS)
        return res;
    psp = ((req[2] & 0x3f) << 8) | req[3];
    dbd = (req[1] & SCSI_MODE_SENSE_DBD) ? true : false;
    len = req[4];
#if (SCSI_DEBUG_REQUESTS)
    printf("SCSI mode sense (6) page, subpage: %#04X, dbd: %d\n\r", psp, dbd);
#endif //SCSI_DEBUG_REQUESTS
    io->data_size = 4;
    switch ((*scsis->storage)->scsi_device_type)
    {
    case SCSI_PERIPHERAL_DEVICE_TYPE_DIRECT_ACCESS:
        scsis_bc_mode_sense_fill_header(scsis, io, dbd);
        res = scsis_bc_mode_sense_add_page(scsis, io, psp);
        break;
    default:
        scsis_error(scsis, SENSE_KEY_HARDWARE_ERROR, ASCQ_INTERNAL_TARGET_FAILURE);
        res = SCSIS_RESPONSE_FAIL;
    }

    if (io->data_size > 256)
        io->data_size = 256;
    if (io->data_size > len)
        io->data_size = len;
    *(uint8_t*)io_data(io) = io->data_size - 1;
    return res;
}

#if (SCSI_LONG_LBA)
/*
    mode sense 10 header

    0 hi len - 1
    1 lo len - 1
    2 medium type
    3 device specific
    4 reserved
    5 reserved
    6 device descriptor size hi
    7 device descriptor size lo
 */

SCSIS_RESPONSE scsis_pc_mode_sense10(SCSIS* scsis, uint8_t* req, IO* io)
{
    unsigned int psp, len;
    bool dbd, llbaa;
    SCSIS_RESPONSE res = scsis_get_media_descriptor(scsis, io);
    if (res != SCSIS_RESPONSE_PASS)
        return res;
    psp = ((req[2] & 0x3f) << 8) | req[3];
    dbd = (req[1] & SCSI_MODE_SENSE_DBD) ? true : false;
    llbaa = (req[1] & SCSI_MODE_SENSE_LLBAA) ? true : false;
    len = be2short(req + 7);
#if (SCSI_DEBUG_REQUESTS)
    printf("SCSI mode sense (10) page, subpage: %#04X, dbd: %d, llbaa: %d\n\r", psp, dbd, llbaa);
#endif //SCSI_DEBUG_REQUESTS
    io->data_size = 4;
    switch ((*scsis->storage)->scsi_device_type)
    {
    case SCSI_PERIPHERAL_DEVICE_TYPE_DIRECT_ACCESS:
        scsis_bc_mode_sense_fill_header_long(scsis, io, dbd, llbaa);
        res = scsis_bc_mode_sense_add_page(scsis, io, psp);
        break;
    default:
        scsis_error(scsis, SENSE_KEY_HARDWARE_ERROR, ASCQ_INTERNAL_TARGET_FAILURE);
        res = SCSIS_RESPONSE_FAIL;
    }

    if (io->data_size > len)
        io->data_size = len;
    short2be(io_data(io), io->data_size - 1);
    return res;
}
#endif //SCSI_LONG_LBA
