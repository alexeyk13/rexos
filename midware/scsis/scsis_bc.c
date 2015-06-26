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
    else if ((*scsis->media)->num_sectors > lba)
        lba = (*scsis->media)->num_sectors - 1;
    int2be(io_data(io) + 0, lba);
    int2be(io_data(io) + 4, (*scsis->media)->sector_size);
    io->data_size = 8;

    return SCSIS_RESPONSE_PASS;
}

static inline unsigned int scsis_bc_append_block_descriptors(SCSIS* scsis, IO* io)
{
    //append to end
    uint8_t* buf = io_data(io) + io->data_size;
    int2be(buf, (*scsis->media)->num_sectors_hi ?  0xffffffff : (*scsis->media)->num_sectors);
    int2be(buf + 4, (*scsis->media)->sector_size);
    //reserved
    buf[4] = 0;
    io->data_size += 8;
    return 8;
}

void scsis_bc_mode_sense_fill_header(SCSIS* scsis, IO *io, bool dbd)
{
    uint8_t* header = io_data(io);
    //medium type according to SBC
    header[1] = 0;
    //no WP, no caching DPOFUA support
    header[2] = 0;
    if (dbd)
        header[3] = 0;
    else
        header[3] = scsis_bc_append_block_descriptors(scsis, io);
}

static void scsis_bc_mode_sense_add_caching_page(SCSIS* scsis, IO* io)
{
    uint8_t* data = io_data(io) + io->data_size;
    memset(data, 0, 20);
    data[0] = MODE_SENSE_PAGE_CACHING;
    data[1] = 20 - 2;
    //RCD
    data[2] = (1 << 0);
    io->data_size += 20;
}

SCSIS_RESPONSE scsis_bc_mode_sense_add_page(SCSIS* scsis, IO* io, unsigned int page, unsigned int subpage)
{
    SCSIS_RESPONSE res = SCSIS_RESPONSE_FAIL;

    switch (page)
    {
    case MODE_SENSE_PAGE_ALL_PAGES:
        if (subpage == MODE_SENSE_SUBPAGE_ALL || subpage == MODE_SENSE_SUBPAGE_NONE)
        {
            scsis_bc_mode_sense_add_caching_page(scsis, io);
            res = SCSIS_RESPONSE_PASS;
        }
        break;
    case MODE_SENSE_PAGE_CACHING:
        if (subpage == MODE_SENSE_SUBPAGE_NONE)
        {
            scsis_bc_mode_sense_add_caching_page(scsis, io);
            res = SCSIS_RESPONSE_PASS;
        }
        break;
    default:
        break;
    }

    if (res != SCSIS_RESPONSE_PASS)
        scsis_error(scsis, SENSE_KEY_ILLEGAL_REQUEST, ASCQ_INVALID_FIELD_IN_CDB);
    return res;
}
