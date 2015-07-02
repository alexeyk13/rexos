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

static void scsis_io(SCSIS* scsis)
{
    SCSI_STACK* stack = io_stack(scsis->io);
    if (!scsis_get_media(scsis))
        return;
    //request completed
    if (scsis->count == 0)
    {
        scsis_pass(scsis);
        return;
    }
    scsis->count_cur = SCSI_IO_SIZE / (*scsis->media)->sector_size;
    if (scsis->count_cur == 0)
    {
#if (SCSI_DEBUG_ERRORS)
        printf("SCSI: failure due to inproper configuration\n\r");
#endif //SCSI_DEBUG_ERRORS
        scsis_fatal(scsis);
    }
    if (scsis->count < scsis->count_cur)
        scsis->count_cur = scsis->count;
    stack->size = scsis->count_cur * (*scsis->media)->sector_size;
    stack->lba = scsis->lba;
#if (SCSI_LONG_LBA)
    stack->lba_hi = 0;
#endif //SCSI_LONG_LBA

    scsis->count -= scsis->count_cur;
    scsis->lba += scsis->count_cur;
#if (SCSI_LONG_LBA)
    if (scsis->lba < stack->lba)
        ++stack->lba_hi;
#endif //SCSI_LONG_LBA

    switch (scsis->state)
    {
    case SCSIS_STATE_READ:
        scsis_storage_request(scsis, SCSIS_REQUEST_READ);
        break;
    case SCSIS_STATE_WRITE:
    case SCSIS_STATE_VERIFY:
        scsis_host_request(scsis, SCSIS_REQUEST_READ);
        break;
    default:
#if (SCSI_DEBUG_ERRORS)
        printf("SCSI: invalid state on io: %d\n\r", scsis->state);
#endif //SCSI_DEBUG_ERRORS
        scsis_fatal(scsis);
    }
}

static bool scsis_bc_io_response_check(SCSIS* scsis)
{
    SCSI_STACK* stack = io_stack(scsis->io);
    if (!scsis_get_media(scsis))
        return false;
    if (stack->size < 0)
    {
        switch (stack->size)
        {
        case ERROR_CRC:
            scsis_fail(scsis, SENSE_KEY_MEDIUM_ERROR, ASCQ_LOGICAL_UNIT_COMMUNICATION_CRC_ERROR);
            break;
        case ERROR_IN_PROGRESS:
            scsis_fail(scsis, SENSE_KEY_MEDIUM_ERROR, ASCQ_LOGICAL_UNIT_NOT_READY_OPERATION_IN_PROGRESS);
            break;
        case ERROR_ACCESS_DENIED:
            scsis_fail(scsis, SENSE_KEY_MEDIUM_ERROR, ASCQ_WRITE_PROTECTED);
            break;
        case ERROR_INVALID_PARAMS:
            scsis_fail(scsis, SENSE_KEY_MEDIUM_ERROR, ASCQ_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE);
            break;
        default:
            scsis_fail(scsis, SENSE_KEY_HARDWARE_ERROR, ASCQ_LOGICAL_UNIT_COMMUNICATION_FAILURE);
            break;
        }
        return false;
    }
    if (stack->size != scsis->count_cur * (*scsis->media)->sector_size)
    {
        scsis_fatal(scsis);
        return false;
    }
    return true;
}

void scsis_bc_host_io_complete(SCSIS* scsis)
{
    if (!scsis_bc_io_response_check(scsis))
        return;
    switch (scsis->state)
    {
    case SCSIS_STATE_READ:
        scsis_io(scsis);
        break;
    case SCSIS_STATE_WRITE:
        //TODO: the best place to put async io is here
        scsis_storage_request(scsis, SCSIS_REQUEST_WRITE);
        break;
    case SCSIS_STATE_VERIFY:
        scsis_storage_request(scsis, SCSIS_REQUEST_VERIFY);
        break;
    default:
        break;
    }
}

void scsis_bc_storage_io_complete(SCSIS* scsis)
{
    if (!scsis_bc_io_response_check(scsis))
        return;
    switch (scsis->state)
    {
    case SCSIS_STATE_READ:
        scsis_host_request(scsis, SCSIS_REQUEST_WRITE);
        break;
    case SCSIS_STATE_WRITE:
    case SCSIS_STATE_VERIFY:
        scsis_io(scsis);
        break;
    default:
        break;
    }
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
    scsis->state = SCSIS_STATE_READ;
    scsis_io(scsis);
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
    scsis->state = SCSIS_STATE_READ;
    scsis_io(scsis);
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
