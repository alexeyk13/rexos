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
#include <string.h>

void scsis_bc_read_capacity10(SCSIS* scsis, uint8_t* req)
{
    unsigned int lba;
    if (!scsis_get_media(scsis))
        return;
#if (SCSI_DEBUG_REQUESTS)
    printf("SCSI read capacity (10)\n\r");
#endif //SCSI_DEBUG_REQUESTS
    lba = be2int(req + 2);
    if (lba && ((req[8] & SCSI_READ_CAPACITY_PMI) == 0))
    {
        scsis_fail(scsis, SENSE_KEY_ILLEGAL_REQUEST, ASCQ_INVALID_FIELD_IN_CDB);
        return;
    }
    if ((*scsis->media)->num_sectors_hi)
        lba = 0xffffffff;
    else if ((*scsis->media)->num_sectors > lba)
        lba = (*scsis->media)->num_sectors - 1;
    int2be(io_data(scsis->io) + 0, lba);
    int2be(io_data(scsis->io) + 4, (*scsis->media)->sector_size);
    scsis->io->data_size = 8;

    scsis->state = SCSIS_STATE_COMPLETE;
    scsis_host_request(scsis, SCSIS_REQUEST_WRITE);
}

static void scsis_read(SCSIS* scsis)
{
    //TODO:
/*    if (len > 1)
    {
        scsis_error(scsis, SENSE_KEY_ILLEGAL_REQUEST, ASCQ_INVALID_FIELD_IN_PARAMETER_LIST);
        return SCSIS_RESPONSE_FAIL;
    }
    memset(io_data(io), 0, 512);
    io->data_size = 512;*/
///    return SCSIS_RESPONSE_PASS;
}

void scsis_bc_read6(SCSIS* scsis, uint8_t* req)
{
    if (!scsis_get_media(scsis))
        return;
    scsis->lba = ((req[1] & 0x1f) << 16) | be2short(req + 2);
#if (SCSI_LONG_LBA)
    scsis->lba_hi = 0;
#endif //SCSI_LONG_LBA
    scsis->count = req[4];
    if (scsis->count == 0)
        scsis->count = 256;
#if (SCSI_DEBUG_REQUESTS)
    printf("SCSI read(6) lba: %#08X, len: %#X\n\r", scsis->lba, scsis->count);
#endif //SCSI_DEBUG_REQUESTS
    //?????
    scsis_read(scsis);
}

void scsis_bc_read10(SCSIS* scsis, uint8_t* req)
{
    if (!scsis_get_media(scsis))
        return;
    scsis->lba = be2int(req + 2);
#if (SCSI_LONG_LBA)
    scsis->lba_hi = 0;
#endif //SCSI_LONG_LBA
    scsis->count = be2short(req + 7);
#if (SCSI_DEBUG_REQUESTS)
    printf("SCSI read(10) lba: %#08X, len: %#X\n\r", scsis->lba, scsis->count);
#endif //SCSI_DEBUG_REQUESTS
    //?????
    scsis_read(scsis);
}

static unsigned short scsis_bc_append_block_descriptors(SCSIS* scsis)
{
    //append to end
    uint8_t* buf = io_data(scsis->io) + scsis->io->data_size;
    if (scsis->media)
    {
        int2be(buf, (*scsis->media)->num_sectors_hi ?  0xffffffff : (*scsis->media)->num_sectors);
        int2be(buf + 4, (*scsis->media)->sector_size);
    }
    else
    {
        int2be(buf, 0xffffffff);
        int2be(buf + 4, 0);
    }
    //reserved
    buf[4] = 0;
    scsis->io->data_size += 8;
    return 8;
}

#if (SCSI_LONG_LBA)
static inline unsigned short scsis_bc_append_block_descriptors_long_lba(SCSIS* scsis)
{
    //append to end
    uint8_t* buf = io_data(scsis->io) + scsis->io->data_size;
    if (scsis->media)
    {
        int2be(buf, (*scsis->media)->num_sectors_hi);
        int2be(buf + 4, (*scsis->media)->num_sectors);
        int2be(buf + 12, (*scsis->media)->sector_size);
    }
    else
    {
        int2be(buf, 0xffffffff);
        int2be(buf + 4, 0xffffffff);
        int2be(buf + 12, 0);
    }
    int2be(buf + 8, 0);
    scsis->io->data_size += 16;
    return 16;
}
#endif //SCSI_LONG_LBA

void scsis_bc_mode_sense_fill_header(SCSIS* scsis, bool dbd)
{
    uint8_t* header = io_data(scsis->io);
    //medium type according to SBC
    header[1] = 0;
    //no WP, no caching DPOFUA support
    header[2] = 0;
    if (dbd)
        header[3] = 0;
    else
        header[3] = scsis_bc_append_block_descriptors(scsis);
}

#if (SCSI_LONG_LBA)
void scsis_bc_mode_sense_fill_header_long(SCSIS* scsis, bool dbd, bool long_lba)
{
    unsigned short len = 0;
    uint8_t* header = io_data(scsis->io);
    //medium type according to SBC
    header[2] = 0;
    //no WP, no caching DPOFUA support
    header[3] = 0;

    header[4] = 0;
    header[5] = 0;
    if (!dbd)
    {
        if (long_lba && (*scsis->media)->num_sectors_hi)
        {
            len = scsis_bc_append_block_descriptors_long_lba(scsis);
            //LONGLBA
            header[4] |= (1 << 0);
        }
        else
            len = scsis_bc_append_block_descriptors(scsis);
    }
    short2be(header + 6, len);
}
#endif //SCSI_LONG_LBA

static void scsis_bc_mode_sense_add_caching_page(SCSIS* scsis)
{
    uint8_t* data = io_data(scsis->io) + scsis->io->data_size;
    memset(data, 0, 20);
    data[0] = MODE_SENSE_PSP_CACHING >> 8;
    data[1] = 20 - 2;
    //RCD
    data[2] = (1 << 0);
    scsis->io->data_size += 20;
}

bool scsis_bc_mode_sense_add_page(SCSIS* scsis, unsigned int psp)
{
    bool res = false;
    switch (psp)
    {
    case MODE_SENSE_PSP_ALL_PAGES:
    case MODE_SENSE_PSP_ALL_PAGES_SUBPAGES:
    case MODE_SENSE_PSP_CACHING:
        scsis_bc_mode_sense_add_caching_page(scsis);
        res = true;
        break;
    default:
        scsis_fail(scsis, SENSE_KEY_ILLEGAL_REQUEST, ASCQ_INVALID_FIELD_IN_CDB);
        break;
    }
    return res;
}
