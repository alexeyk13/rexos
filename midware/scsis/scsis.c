/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "scsis.h"
#include "scsis_private.h"
#include "scsis_pc.h"
#include "scsis_bc.h"
#include "scsis_sat.h"
#include "sys_config.h"
#include "../../userspace/stdlib.h"
#include "../../userspace/stdio.h"
#include "../../userspace/scsi.h"
#include "../../userspace/endian.h"

SCSIS* scsis_create(SCSIS_CB cb_host, SCSIS_CB cb_storage, void* param)
{
    SCSIS* scsis = malloc(sizeof(SCSIS));
    if (scsis == NULL)
        return NULL;
    scsis->io = io_create(SCSI_IO_SIZE + sizeof(SCSI_STACK));
    if (scsis->io == NULL)
    {
        free(scsis);
        return NULL;
    }
    io_push(scsis->io, sizeof(SCSI_STACK));
    scsis->cb_host = cb_host;
    scsis->cb_storage = cb_storage;
    scsis->param = param;
    scsis_error_init(scsis);
    scsis_reset(scsis);
    return scsis;
}

void scsis_destroy(SCSIS* scsis)
{
    io_destroy(scsis->io);
    free(scsis->storage);
    free(scsis->media);
    free(scsis);
}

void scsis_reset(SCSIS* scsis)
{
    scsis->state = SCSIS_STATE_IDLE;
    scsis->storage = NULL;
    scsis->media = NULL;
}

static inline void scsis_request_internal(SCSIS* scsis, uint8_t* req)
{
    switch (req[0])
    {
    case SCSI_SPC_CMD_MODE_SENSE6:
        scsis_pc_mode_sense6(scsis, req);
        break;
#if (SCSI_LONG_LBA)
    case SCSI_SPC_CMD_MODE_SENSE10:
        scsis_pc_mode_sense10(scsis, req);
        break;
#endif //SCSI_LONG_LBA
    case SCSI_SPC_CMD_MODE_SELECT6:
        scsis_pc_mode_select6(scsis, req);
        break;
#if (SCSI_LONG_LBA)
    case SCSI_SPC_CMD_MODE_SELECT10:
        scsis_pc_mode_select10(scsis, req);
        break;
#endif //SCSI_LONG_LBA
    case SCSI_SPC_CMD_TEST_UNIT_READY:
        scsis_pc_test_unit_ready(scsis, req);
        break;
    case SCSI_SPC_CMD_INQUIRY:
        scsis_pc_inquiry(scsis, req);
        break;
    case SCSI_SPC_CMD_REQUEST_SENSE:
        scsis_pc_request_sense(scsis, req);
        break;
    case SCSI_SBC_CMD_READ_CAPACITY10:
        scsis_bc_read_capacity10(scsis, req);
        break;
    case SCSI_SBC_CMD_READ6:
        scsis_bc_read6(scsis, req);
        break;
    case SCSI_SBC_CMD_READ10:
        scsis_bc_read10(scsis, req);
        break;
    case SCSI_SBC_CMD_READ12:
        scsis_bc_read12(scsis, req);
        break;
#if (SCSI_LONG_LBA)
    case SCSI_SBC_CMD_READ16:
        scsis_bc_read16(scsis, req);
        break;
    case SCSI_SBC_CMD_EXT_7F:
        switch (be2short(req + 2))
        {
        case SCSI_SBC_CMD_EXT_7F_READ32:
            scsis_bc_read32(scsis, req);
            break;
        default:
#if (SCSI_DEBUG_ERRORS)
            printf("SCSI: unknown cmd 0x7f action: %02xh\n\r", be2short(req + 2));
#endif //SCSI_DEBUG_ERRORS
            scsis_fail(scsis, SENSE_KEY_ILLEGAL_REQUEST, ASCQ_INVALID_COMMAND_OPERATION_CODE);
        }
        break;
#endif //SCSI_LONG_LBA
#if (SCSI_SAT)
    //SAT
    case SCSI_SAT_CMD_ATA_PASS_THROUGH12:
        scsis_sat_ata_pass_through12(scsis, req);
        break;
    case SCSI_SAT_CMD_ATA_PASS_THROUGH16:
        scsis_sat_ata_pass_through16(scsis, req);
        break;
#endif //SCSI_SAT
    default:
#if (SCSI_DEBUG_ERRORS)
        printf("SCSI: unknown cmd opcode: %02xh\n\r", req[0]);
#endif //SCSI_DEBUG_ERRORS
        scsis_fail(scsis, SENSE_KEY_ILLEGAL_REQUEST, ASCQ_INVALID_COMMAND_OPERATION_CODE);
        break;
    }
}

void scsis_request(SCSIS* scsis, uint8_t* req)
{
    switch (scsis->state)
    {
    case SCSIS_STATE_IDLE:
    case SCSIS_STATE_STORAGE_DESCRIPTOR_REQUEST:
    case SCSIS_STATE_MEDIA_DESCRIPTOR_REQUEST:
        scsis_request_internal(scsis, req);
        break;
    default:
        //io in progress
#if (SCSI_DEBUG_ERRORS)
        printf("SCSI invalid state on request: %d\n\r", scsis->state);
#endif //SCSI_DEBUG_ERRORS
        scsis_fatal(scsis);
    }
}

void scsis_host_io_complete(SCSIS* scsis)
{
    switch (scsis->state)
    {
    case SCSIS_STATE_COMPLETE:
        scsis_pass(scsis);
        break;
    case SCSIS_STATE_READ:
    case SCSIS_STATE_WRITE:
    case SCSIS_STATE_VERIFY:
        scsis_bc_host_io_complete(scsis);
        break;
    default:
#if (SCSI_DEBUG_ERRORS)
        printf("SCSI invalid state on host io complete: %d\n\r", scsis->state);
#endif //SCSI_DEBUG_ERRORS
        scsis_fatal(scsis);
    }
}

bool scsis_ready(SCSIS* scsis)
{
    return scsis->state == SCSIS_STATE_IDLE;
}

void scsis_media_removed(SCSIS* scsis)
{
    scsis->media = NULL;
}

void scsis_storage_io_complete(SCSIS* scsis)
{
    switch (scsis->state)
    {
    case SCSIS_STATE_READ:
    case SCSIS_STATE_WRITE:
    case SCSIS_STATE_VERIFY:
        scsis_bc_storage_io_complete(scsis);
        break;
    default:
#if (SCSI_DEBUG_ERRORS)
        printf("SCSI invalid state on storage io complete: %d\n\r", scsis->state);
#endif //SCSI_DEBUG_ERRORS
        scsis_fatal(scsis);
    }
}
