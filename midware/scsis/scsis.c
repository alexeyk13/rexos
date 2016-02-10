/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2016, Alexey Kramarenko
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
#include "../../userspace/endian.h"
#include <string.h>

static void scsis_request_media(SCSIS* scsis)
{
    //TODO: only if not removable
    scsis->need_media = true;
    if (scsis->state == SCSIS_STATE_IDLE)
        storage_get_media_descriptor(scsis->storage_descriptor->hal, scsis->storage_descriptor->storage, scsis->storage_descriptor->user, scsis->io);
}

SCSIS* scsis_create(SCSIS_CB cb_host, SCSIS_CB cb_storage, void* param, SCSI_STORAGE_DESCRIPTOR *storage_descriptor)
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
    scsis->storage_descriptor = storage_descriptor;
    scsis->media = NULL;
    scsis->state = SCSIS_STATE_IDLE;
    scsis_error_init(scsis);
    scsis_request_media(scsis);
    return scsis;
}

void scsis_destroy(SCSIS* scsis)
{
    io_destroy(scsis->io);
    scsis_media_removed(scsis);
    free(scsis);
}

void scsis_reset(SCSIS* scsis)
{
    scsis->state = SCSIS_STATE_IDLE;
}

static inline void scsis_request_cmd_internal(SCSIS* scsis, uint8_t* req)
{
    switch (req[0])
    {
    case SCSI_SPC_CMD_MODE_SENSE6:
        scsis_pc_mode_sense6(scsis, req);
        break;
    case SCSI_SPC_CMD_MODE_SELECT6:
        scsis_pc_mode_select6(scsis, req);
        break;
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
    case SCSI_SBC_CMD_WRITE6:
        scsis_bc_write6(scsis, req);
        break;
    case SCSI_SBC_CMD_WRITE10:
        scsis_bc_write10(scsis, req);
        break;
    case SCSI_SBC_CMD_WRITE12:
        scsis_bc_write12(scsis, req);
        break;
#if (SCSI_VERIFY_SUPPORTED)
    case SCSI_SBC_CMD_VERIFY10:
        scsis_bc_verify10(scsis, req);
        break;
    case SCSI_SBC_CMD_VERIFY12:
        scsis_bc_verify12(scsis, req);
        break;
    case SCSI_SBC_CMD_WRITE_AND_VERIFY10:
        scsis_bc_write_verify10(scsis, req);
        break;
    case SCSI_SBC_CMD_WRITE_AND_VERIFY12:
        scsis_bc_write_verify12(scsis, req);
        break;
#endif //SCSI_VERIFY_SUPPORTED
#if (SCSI_LONG_LBA)
    case SCSI_SPC_CMD_MODE_SENSE10:
        scsis_pc_mode_sense10(scsis, req);
        break;
    case SCSI_SPC_CMD_MODE_SELECT10:
        scsis_pc_mode_select10(scsis, req);
        break;
    case SCSI_SBC_CMD_READ16:
        scsis_bc_read16(scsis, req);
        break;
    case SCSI_SBC_CMD_WRITE16:
        scsis_bc_write16(scsis, req);
        break;
#if (SCSI_VERIFY_SUPPORTED)
    case SCSI_SBC_CMD_VERIFY16:
        scsis_bc_verify16(scsis, req);
        break;
    case SCSI_SBC_CMD_WRITE_AND_VERIFY16:
        scsis_bc_write_verify16(scsis, req);
        break;
#endif //SCSI_VERIFY_SUPPORTED
    case SCSI_SBC_CMD_EXT_7F:
        switch (be2short(req + 2))
        {
        case SCSI_SBC_CMD_EXT_7F_READ32:
            scsis_bc_read32(scsis, req);
            break;
        case SCSI_SBC_CMD_EXT_7F_WRITE32:
            scsis_bc_write32(scsis, req);
            break;
#if (SCSI_VERIFY_SUPPORTED)
        case SCSI_SBC_CMD_EXT_7F_VERIFY32:
            scsis_bc_verify32(scsis, req);
            break;
        case SCSI_SBC_CMD_EXT_7F_WRITE_AND_VERIFY32:
            scsis_bc_write_verify32(scsis, req);
            break;
#endif //SCSI_VERIFY_SUPPORTED
        default:
#if (SCSI_DEBUG_ERRORS)
            printf("SCSI: unknown cmd 0x7f action: %02xh\n", be2short(req + 2));
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
        printf("SCSI: unknown cmd opcode: %02xh\n", req[0]);
#endif //SCSI_DEBUG_ERRORS
        scsis_fail(scsis, SENSE_KEY_ILLEGAL_REQUEST, ASCQ_INVALID_COMMAND_OPERATION_CODE);
        break;
    }
}

bool scsis_ready(SCSIS* scsis)
{
    return (scsis->state == SCSIS_STATE_IDLE && scsis->need_media == false);
}

bool scsis_request_cmd(SCSIS* scsis, uint8_t* req)
{
    if (scsis_ready(scsis))
    {
        scsis_request_cmd_internal(scsis, req);
        return true;
    }
    //request is queued, when ready scsi stack will call host by callback
    return false;
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
        //completed before. Just ignore
        //TODO: or not?
        scsis->state = SCSIS_STATE_IDLE;
    }
}

static inline void scsis_get_media_descriptor(SCSIS* scsis, int size)
{
    scsis->need_media = false;
    do {
        if (size < (int)sizeof(STORAGE_MEDIA_DESCRIPTOR))
            //unsupported request? No media.
            break;

        if ((scsis->media = malloc(scsis->io->data_size)) == NULL)
            //out of memory, no media
            break;
        memcpy(scsis->media, io_data(scsis->io), scsis->io->data_size);
    } while (false);
    //media descriptor responded, inform host on ready state
    scsis_host_request(scsis, SCSIS_REQUEST_READY);
}

void scsis_request(SCSIS* scsis, IPC* ipc)
{
    if (HAL_GROUP(ipc->cmd) != scsis->storage_descriptor->hal)
    {
        error(ERROR_NOT_SUPPORTED);
        return;
    }
    switch (HAL_ITEM(ipc->cmd))
    {
    //TODO: storage IO here
    case STORAGE_GET_MEDIA_DESCRIPTOR:
        scsis_get_media_descriptor(scsis, (int)ipc->param3);
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
    }
}

void scsis_media_removed(SCSIS* scsis)
{
    free(scsis->media);
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
        break;
        //TODO: this will be refactored
    }
}
