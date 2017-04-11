/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "scsis_bc.h"
#include "scsis_private.h"
#include "sys_config.h"
#include "../../userspace/stdio.h"
#include "../../userspace/scsi.h"
#include "../../userspace/endian.h"
#include "../../userspace/error.h"
#include <string.h>

void scsis_bc_read_capacity10(SCSIS* scsis, uint8_t* req)
{
    unsigned int lba;
    if (!scsis_get_media(scsis))
        return;
#if (SCSI_DEBUG_REQUESTS)
    printf("SCSI read capacity (10)\n");
#endif //SCSI_DEBUG_REQUESTS
    lba = be2int(req + 2);
    if (lba && ((req[8] & SCSI_READ_CAPACITY_PMI) == 0))
    {
        scsis_fail(scsis, SENSE_KEY_ILLEGAL_REQUEST, ASCQ_INVALID_FIELD_IN_CDB);
        return;
    }
    if ((scsis->media->num_sectors_hi) || (scsis->media->num_sectors < lba))
        lba = 0xffffffff;
    else
        lba = scsis->media->num_sectors - 1;
    int2be(io_data(scsis->io) + 0, lba);
    int2be(io_data(scsis->io) + 4, scsis->media->sector_size);
    scsis->io->data_size = 8;

    scsis->state = SCSIS_STATE_COMPLETE;
    scsis_cb_host(scsis, SCSIS_RESPONSE_WRITE, scsis->io->data_size);
}

#if (SCSI_LONG_LBA)
void scsis_bc_read_capacity16(SCSIS* scsis, uint8_t* req)
{
    unsigned int lba, lba_hi, len;
    if (!scsis_get_media(scsis))
        return;
#if (SCSI_DEBUG_REQUESTS)
    printf("SCSI read capacity (16)\n");
#endif //SCSI_DEBUG_REQUESTS
    lba_hi = be2int(req + 2);
    lba = be2int(req + 6);
    len = be2int(req + 10);
    if ((lba || lba_hi) && ((req[16] & SCSI_READ_CAPACITY_PMI) == 0))
    {
        scsis_fail(scsis, SENSE_KEY_ILLEGAL_REQUEST, ASCQ_INVALID_FIELD_IN_CDB);
        return;
    }
    if ((scsis->media->num_sectors_hi < lba_hi) || ((scsis->media->num_sectors_hi == lba_hi) && (scsis->media->num_sectors < lba)))
        lba_hi = lba = 0xffffffff;
    else
    {
        lba_hi = scsis->media->num_sectors_hi;
        if (scsis->media->num_sectors == 0)
        {
            lba = 0xffffffff;
            --lba_hi;
        }
        else
            lba = scsis->media->num_sectors - 1;
    }
    scsis->io->data_size = 32;
    memset(io_data(scsis->io), 0, scsis->io->data_size);
    int2be(io_data(scsis->io) + 0, lba_hi);
    int2be(io_data(scsis->io) + 4, lba);
    int2be(io_data(scsis->io) + 8, scsis->media->sector_size);
    if (scsis->io->data_size > len)
        scsis->io->data_size = len;

    scsis->state = SCSIS_STATE_COMPLETE;
    scsis_cb_host(scsis, SCSIS_RESPONSE_WRITE, scsis->io->data_size);
}
#endif //SCSI_LONG_LBA

void scsis_bc_start_stop_unit(SCSIS* scsis, uint8_t* req)
{
#if (SCSI_DEBUG_REQUESTS)
    printf("SCSI start/stop unit\n");
#endif //SCSI_DEBUG_REQUESTS
    if (req[4] & (1 << 1))
        storage_request_eject(scsis->storage_descriptor->hal, scsis->storage_descriptor->storage, scsis->storage_descriptor->user);
    scsis_pass(scsis);
}

static void scsis_io(SCSIS* scsis)
{
    scsis->io->data_size = 0;
#if (SCSI_LONG_LBA)
    uint32_t lba_old = scsis->lba;
#endif //SCSI_LONG_LBA
    if (!scsis_get_media(scsis))
        return;
    //request completed
    if (scsis->count == 0)
    {
        scsis_pass(scsis);
        return;
    }
    scsis->lba += scsis->count_cur;
#if (SCSI_LONG_LBA)
    if (scsis->lba < lba_old)
        ++scsis->lba_hi;
#endif //SCSI_LONG_LBA

    scsis->count_cur = (io_get_free(scsis->io) - sizeof(STORAGE_STACK)) / scsis->media->sector_size;
    if (scsis->count < scsis->count_cur)
        scsis->count_cur = scsis->count;

    scsis->count -= scsis->count_cur;

    switch (scsis->state)
    {
    case SCSIS_STATE_READ:
        storage_read(scsis->storage_descriptor->hal, scsis->storage_descriptor->storage, scsis->storage_descriptor->user,
                     scsis->io, scsis->lba, scsis->count_cur * scsis->media->sector_size);
        break;
    case SCSIS_STATE_WRITE:
#if (SCSI_VERIFY_SUPPORTED)
    case SCSIS_STATE_VERIFY:
    case SCSIS_STATE_WRITE_VERIFY:
#endif //SCSI_VERIFY_SUPPORTED
        scsis_cb_host(scsis, SCSIS_RESPONSE_READ, scsis->count_cur * scsis->media->sector_size);
        break;
    default:
        break;
    }
}

static bool scsis_bc_io_response_check(SCSIS* scsis, int size)
{
    if (!scsis_get_media(scsis))
        return false;
    if (size < 0)
    {
        switch (size)
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
    else if (size != scsis->count_cur * scsis->media->sector_size)
    {
        scsis_fail(scsis, SENSE_KEY_HARDWARE_ERROR, ASCQ_LOGICAL_UNIT_COMMUNICATION_FAILURE);
        return false;
    }
    return true;
}

void scsis_bc_host_io_complete(SCSIS* scsis, int resp_size)
{
    if (!scsis_bc_io_response_check(scsis, resp_size))
        return;
    switch (scsis->state)
    {
    case SCSIS_STATE_READ:
        scsis_io(scsis);
        break;
    case SCSIS_STATE_WRITE:
#if (SCSI_WRITE_CACHE)
        if (scsis->count == 0)
            scsis_cb_host(scsis, SCSIS_RESPONSE_PASS, 0);
#endif //SCSI_WRITE_CACHE
        storage_write(scsis->storage_descriptor->hal, scsis->storage_descriptor->storage, scsis->storage_descriptor->user,
                      scsis->io, scsis->lba);
        break;
#if (SCSI_VERIFY_SUPPORTED)
    case SCSIS_STATE_VERIFY:
        storage_verify(scsis->storage_descriptor->hal, scsis->storage_descriptor->storage, scsis->storage_descriptor->user,
                      scsis->io, scsis->lba);
        break;
    case SCSIS_STATE_WRITE_VERIFY:
        storage_write_verify(scsis->storage_descriptor->hal, scsis->storage_descriptor->storage, scsis->storage_descriptor->user,
                      scsis->io, scsis->lba);
        break;
#endif //SCSI_VERIFY_SUPPORTED
    default:
        break;
    }
}

void scsis_bc_storage_io_complete(SCSIS* scsis, int resp_size)
{
    if (!scsis_bc_io_response_check(scsis, resp_size))
        return;

    switch (scsis->state)
    {
    case SCSIS_STATE_READ:
        scsis_cb_host(scsis, SCSIS_RESPONSE_WRITE, scsis->io->data_size);
        break;
    case SCSIS_STATE_WRITE:
#if (SCSI_VERIFY_SUPPORTED)
    case SCSIS_STATE_VERIFY:
    case SCSIS_STATE_WRITE_VERIFY:
#endif //SCSI_VERIFY_SUPPORTED
        scsis_io(scsis);
        break;
    default:
        break;
    }
}
void scsis_bc_read6(SCSIS* scsis, uint8_t* req)
{
    scsis->lba = ((req[1] & 0x1f) << 16) | be2short(req + 2);
#if (SCSI_LONG_LBA)
    scsis->lba_hi = 0;
#endif //SCSI_LONG_LBA
    scsis->count = req[4];
    if (scsis->count == 0)
        scsis->count = 256;
#if (SCSI_DEBUG_REQUESTS)
    printf("SCSI read(6) lba: %#08X, len: %#X\n", scsis->lba, scsis->count);
#endif //SCSI_DEBUG_REQUESTS
    scsis->state = SCSIS_STATE_READ;
    scsis->count_cur = 0;
    scsis_io(scsis);
}

void scsis_bc_read10(SCSIS* scsis, uint8_t* req)
{
    scsis->lba = be2int(req + 2);
#if (SCSI_LONG_LBA)
    scsis->lba_hi = 0;
#endif //SCSI_LONG_LBA
    scsis->count = be2short(req + 7);
#if (SCSI_DEBUG_REQUESTS)
    printf("SCSI read(10) lba: %#08X, len: %#X\n", scsis->lba, scsis->count);
#endif //SCSI_DEBUG_REQUESTS
    scsis->state = SCSIS_STATE_READ;
    scsis->count_cur = 0;
    scsis_io(scsis);
}

void scsis_bc_read12(SCSIS* scsis, uint8_t* req)
{
    scsis->lba = be2int(req + 2);
#if (SCSI_LONG_LBA)
    scsis->lba_hi = 0;
#endif //SCSI_LONG_LBA
    scsis->count = be2short(req + 6);
#if (SCSI_DEBUG_REQUESTS)
    printf("SCSI read(12) lba: %#08X, len: %#X\n", scsis->lba, scsis->count);
#endif //SCSI_DEBUG_REQUESTS
    scsis->state = SCSIS_STATE_READ;
    scsis->count_cur = 0;
    scsis_io(scsis);
}

void scsis_bc_write6(SCSIS* scsis, uint8_t* req)
{
    scsis->lba = ((req[1] & 0x1f) << 16) | be2short(req + 2);
#if (SCSI_LONG_LBA)
    scsis->lba_hi = 0;
#endif //SCSI_LONG_LBA
    scsis->count = req[4];
    if (scsis->count == 0)
        scsis->count = 256;
#if (SCSI_DEBUG_REQUESTS)
    printf("SCSI write(6) lba: %#08X, len: %#X\n", scsis->lba, scsis->count);
#endif //SCSI_DEBUG_REQUESTS
    scsis->state = SCSIS_STATE_WRITE;
    scsis->count_cur = 0;
    scsis_io(scsis);
}

void scsis_bc_write10(SCSIS* scsis, uint8_t* req)
{
    scsis->lba = be2int(req + 2);
#if (SCSI_LONG_LBA)
    scsis->lba_hi = 0;
#endif //SCSI_LONG_LBA
    scsis->count = be2short(req + 7);
#if (SCSI_DEBUG_REQUESTS)
    printf("SCSI write(10) lba: %#08X, len: %#X\n", scsis->lba, scsis->count);
#endif //SCSI_DEBUG_REQUESTS
    scsis->state = SCSIS_STATE_WRITE;
    scsis->count_cur = 0;
    scsis_io(scsis);
}

void scsis_bc_write12(SCSIS* scsis, uint8_t* req)
{
    scsis->lba = be2int(req + 2);
#if (SCSI_LONG_LBA)
    scsis->lba_hi = 0;
#endif //SCSI_LONG_LBA
    scsis->count = be2short(req + 6);
#if (SCSI_DEBUG_REQUESTS)
    printf("SCSI write(12) lba: %#08X, len: %#X\n", scsis->lba, scsis->count);
#endif //SCSI_DEBUG_REQUESTS
    scsis->state = SCSIS_STATE_WRITE;
    scsis->count_cur = 0;
    scsis_io(scsis);
}

#if (SCSI_VERIFY_SUPPORTED)
void scsis_bc_verify10(SCSIS* scsis, uint8_t* req)
{
    scsis->lba = be2int(req + 2);
#if (SCSI_LONG_LBA)
    scsis->lba_hi = 0;
#endif //SCSI_LONG_LBA
    scsis->count = be2short(req + 7);
#if (SCSI_DEBUG_REQUESTS)
    printf("SCSI verify(10) lba: %#08X, len: %#X\n", scsis->lba, scsis->count);
#endif //SCSI_DEBUG_REQUESTS
    scsis->state = SCSIS_STATE_VERIFY;
    scsis->count_cur = 0;
    scsis_io(scsis);
}

void scsis_bc_verify12(SCSIS* scsis, uint8_t* req)
{
    scsis->lba = be2int(req + 2);
#if (SCSI_LONG_LBA)
    scsis->lba_hi = 0;
#endif //SCSI_LONG_LBA
    scsis->count = be2short(req + 6);
#if (SCSI_DEBUG_REQUESTS)
    printf("SCSI verify(12) lba: %#08X, len: %#X\n", scsis->lba, scsis->count);
#endif //SCSI_DEBUG_REQUESTS
    scsis->state = SCSIS_STATE_VERIFY;
    scsis->count_cur = 0;
    scsis_io(scsis);
}

void scsis_bc_write_verify10(SCSIS* scsis, uint8_t* req)
{
    scsis->lba = be2int(req + 2);
#if (SCSI_LONG_LBA)
    scsis->lba_hi = 0;
#endif //SCSI_LONG_LBA
    scsis->count = be2short(req + 7);
#if (SCSI_DEBUG_REQUESTS)
    printf("SCSI write and verify(10) lba: %#08X, len: %#X\n", scsis->lba, scsis->count);
#endif //SCSI_DEBUG_REQUESTS
    scsis->state = SCSIS_STATE_WRITE_VERIFY;
    scsis->count_cur = 0;
    scsis_io(scsis);
}

void scsis_bc_write_verify12(SCSIS* scsis, uint8_t* req)
{
    scsis->lba = be2int(req + 2);
#if (SCSI_LONG_LBA)
    scsis->lba_hi = 0;
#endif //SCSI_LONG_LBA
    scsis->count = be2short(req + 6);
#if (SCSI_DEBUG_REQUESTS)
    printf("SCSI write and verify(12) lba: %#08X, len: %#X\n", scsis->lba, scsis->count);
#endif //SCSI_DEBUG_REQUESTS
    scsis->state = SCSIS_STATE_WRITE_VERIFY;
    scsis->count_cur = 0;
    scsis_io(scsis);
}
#endif //SCSI_VERIFY_SUPPORTED

#if (SCSI_LONG_LBA)
void scsis_bc_read16(SCSIS* scsis, uint8_t* req)
{
    scsis->lba = be2int(req + 2);
    scsis->lba_hi = be2int(req + 6);
    scsis->count = be2short(req + 10);
#if (SCSI_DEBUG_REQUESTS)
    printf("SCSI read(16) lba: %#08X%08X, len: %#X\n", scsis->lba_hi, scsis->lba, scsis->count);
#endif //SCSI_DEBUG_REQUESTS
    scsis->state = SCSIS_STATE_READ;
    scsis->count_cur = 0;
    scsis_io(scsis);
}

void scsis_bc_read32(SCSIS* scsis, uint8_t* req)
{
    scsis->lba = be2int(req + 12);
    scsis->lba_hi = be2int(req + 16);
    scsis->count = be2short(req + 28);
#if (SCSI_DEBUG_REQUESTS)
    printf("SCSI read(32) lba: %#08X%08X, len: %#X\n", scsis->lba_hi, scsis->lba, scsis->count);
#endif //SCSI_DEBUG_REQUESTS
    scsis->state = SCSIS_STATE_READ;
    scsis->count_cur = 0;
    scsis_io(scsis);
}

void scsis_bc_write16(SCSIS* scsis, uint8_t* req)
{
    scsis->lba = be2int(req + 2);
    scsis->lba_hi = be2int(req + 6);
    scsis->count = be2short(req + 10);
#if (SCSI_DEBUG_REQUESTS)
    printf("SCSI write(16) lba: %#08X%08X, len: %#X\n", scsis->lba_hi, scsis->lba, scsis->count);
#endif //SCSI_DEBUG_REQUESTS
    scsis->state = SCSIS_STATE_WRITE;
    scsis->count_cur = 0;
    scsis_io(scsis);
}

void scsis_bc_write32(SCSIS* scsis, uint8_t* req)
{
    scsis->lba = be2int(req + 12);
    scsis->lba_hi = be2int(req + 16);
    scsis->count = be2short(req + 28);
#if (SCSI_DEBUG_REQUESTS)
    printf("SCSI write(32) lba: %#08X%08X, len: %#X\n", scsis->lba_hi, scsis->lba, scsis->count);
#endif //SCSI_DEBUG_REQUESTS
    scsis->state = SCSIS_STATE_WRITE;
    scsis->count_cur = 0;
    scsis_io(scsis);
}

#if (SCSI_VERIFY_SUPPORTED)
void scsis_bc_verify16(SCSIS* scsis, uint8_t* req)
{
    scsis->lba = be2int(req + 2);
    scsis->lba_hi = be2int(req + 6);
    scsis->count = be2short(req + 10);
#if (SCSI_DEBUG_REQUESTS)
    printf("SCSI verify(16) lba: %#08X%08X, len: %#X\n", scsis->lba_hi, scsis->lba, scsis->count);
#endif //SCSI_DEBUG_REQUESTS
    scsis->state = SCSIS_STATE_VERIFY;
    scsis->count_cur = 0;
    scsis_io(scsis);
}

void scsis_bc_verify32(SCSIS* scsis, uint8_t* req)
{
    scsis->lba = be2int(req + 12);
    scsis->lba_hi = be2int(req + 16);
    scsis->count = be2short(req + 28);
#if (SCSI_DEBUG_REQUESTS)
    printf("SCSI verify(32) lba: %#08X%08X, len: %#X\n", scsis->lba_hi, scsis->lba, scsis->count);
#endif //SCSI_DEBUG_REQUESTS
    scsis->state = SCSIS_STATE_VERIFY;
    scsis->count_cur = 0;
    scsis_io(scsis);
}

void scsis_bc_write_verify16(SCSIS* scsis, uint8_t* req)
{
    scsis->lba = be2int(req + 2);
    scsis->lba_hi = be2int(req + 6);
    scsis->count = be2short(req + 10);
#if (SCSI_DEBUG_REQUESTS)
    printf("SCSI write and verify(16) lba: %#08X%08X, len: %#X\n", scsis->lba_hi, scsis->lba, scsis->count);
#endif //SCSI_DEBUG_REQUESTS
    scsis->state = SCSIS_STATE_WRITE_VERIFY;
    scsis->count_cur = 0;
    scsis_io(scsis);
}

void scsis_bc_write_verify32(SCSIS* scsis, uint8_t* req)
{
    scsis->lba = be2int(req + 12);
    scsis->lba_hi = be2int(req + 16);
    scsis->count = be2short(req + 28);
#if (SCSI_DEBUG_REQUESTS)
    printf("SCSI write and verify(32) lba: %#08X%08X, len: %#X\n", scsis->lba_hi, scsis->lba, scsis->count);
#endif //SCSI_DEBUG_REQUESTS
    scsis->state = SCSIS_STATE_WRITE_VERIFY;
    scsis->count_cur = 0;
    scsis_io(scsis);
}
#endif //SCSI_VERIFY_SUPPORTED
#endif //SCSI_LONG_LBA

static unsigned short scsis_bc_append_block_descriptors(SCSIS* scsis)
{
    //append to end
    uint8_t* buf = io_data(scsis->io) + scsis->io->data_size;
    if (scsis->media)
    {
        int2be(buf, scsis->media->num_sectors_hi ?  0xffffffff : scsis->media->num_sectors);
        int2be(buf + 4, scsis->media->sector_size);
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
        int2be(buf, scsis->media->num_sectors_hi);
        int2be(buf + 4, scsis->media->num_sectors);
        int2be(buf + 12, scsis->media->sector_size);
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
        if (long_lba && scsis->media->num_sectors_hi)
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

#if (SCSI_WRITE_CACHE)
    data[2] |= 1 << 2;
#endif //SCSI_WRITE_CACHE
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
