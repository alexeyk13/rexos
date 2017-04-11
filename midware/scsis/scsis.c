/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "scsis.h"
#include "scsis_private.h"
#include "scsis_pc.h"
#include "scsis_bc.h"
#include "scsis_sat.h"
#include "scsis_mmc.h"
#include "sys_config.h"
#include "../../userspace/stdlib.h"
#include "../../userspace/stdio.h"
#include "../../userspace/endian.h"
#include "../../userspace/error.h"
#include <string.h>

static void scsis_request_media(SCSIS* scsis)
{
    scsis->need_media = true;
    if (scsis->state == SCSIS_STATE_IDLE)
    {
        if (scsis->io != NULL)
            storage_get_media_descriptor(scsis->storage_descriptor->hal, scsis->storage_descriptor->storage, scsis->storage_descriptor->user, scsis->io);
        else
            scsis_cb_host(scsis, SCSIS_RESPONSE_NEED_IO, 0);
    }
}

static void scsis_media_removed(SCSIS* scsis)
{
    free(scsis->media);
    scsis->media = NULL;
}

SCSIS* scsis_create(SCSIS_CB cb_host, void* param, unsigned int id, SCSI_STORAGE_DESCRIPTOR *storage_descriptor)
{
    SCSIS* scsis = malloc(sizeof(SCSIS));
    if (scsis == NULL)
        return NULL;
    scsis->cb_host = cb_host;
    scsis->param = param;
    scsis->id = id;
    scsis->storage_descriptor = storage_descriptor;
    scsis->io = NULL;
    scsis->media = NULL;
    scsis->state = SCSIS_STATE_IDLE;
    return scsis;
}

void scsis_destroy(SCSIS* scsis)
{
    if (scsis->storage_descriptor->flags & SCSI_STORAGE_DESCRIPTOR_REMOVABLE)
        storage_cancel_notify_state_change(scsis->storage_descriptor->hal, scsis->storage_descriptor->storage, scsis->storage_descriptor->user);
    scsis_media_removed(scsis);
    free(scsis);
}

void scsis_init(SCSIS* scsis)
{
    scsis_error_init(scsis);
#if (SCSI_MMC)
    scsis->media_status_changed = false;
#endif //SCSI_MMC
    if (scsis->storage_descriptor->flags & SCSI_STORAGE_DESCRIPTOR_REMOVABLE)
        storage_request_notify_state_change(scsis->storage_descriptor->hal, scsis->storage_descriptor->storage, scsis->storage_descriptor->user);
    else
        scsis_request_media(scsis);
}

void scsis_reset(SCSIS* scsis)
{
    scsis->need_media = false;
    scsis->io = NULL;
    scsis->state = SCSIS_STATE_IDLE;
}

void scsis_request_cmd(SCSIS* scsis, IO* io, uint8_t* req)
{
    scsis->io = io;
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
    case SCSI_SPC_CMD_PREVENT_ALLOW_MEDIUM_REMOVAL:
        scsis_pc_prevent_allow_medium_removal(scsis, req);
        break;
    case SCSI_SBC_CMD_READ_CAPACITY10:
        scsis_bc_read_capacity10(scsis, req);
        break;
    case SCSI_SBC_CMD_START_STOP_UNIT:
        scsis_bc_start_stop_unit(scsis, req);
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
#if (SCSI_MMC)
    case SCSI_MMC_CMD_READ_TOC:
        scsis_mmc_read_toc(scsis, req);
        break;
    case SCSI_MMC_CMD_GET_CONFIGURATION:
        scsis_mmc_get_configuration(scsis, req);
        break;
    case SCSI_MMC_CMD_GET_EVENT_STATUS_NOTIFICATION:
        scsis_mmc_get_event_status_notification(scsis, req);
        break;
#endif //SCSI_MMC
    default:
#if (SCSI_DEBUG_ERRORS)
        printf("SCSI: unknown cmd opcode: %02xh\n", req[0]);
#endif //SCSI_DEBUG_ERRORS
        scsis_fail(scsis, SENSE_KEY_ILLEGAL_REQUEST, ASCQ_INVALID_COMMAND_OPERATION_CODE);
        break;
    }
}

void scsis_host_io_complete(SCSIS* scsis, int resp_size)
{
    if (scsis->state == SCSIS_STATE_COMPLETE)
        scsis_pass(scsis);
    else
        scsis_bc_host_io_complete(scsis, resp_size);
}

void scsis_host_give_io(SCSIS* scsis, IO* io)
{
    scsis->io = io;
    scsis_request_media(scsis);
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
        if (scsis->storage_descriptor->hidden_sectors > scsis->media->num_sectors)
        {
            --scsis->media->num_sectors_hi;
            scsis->media->num_sectors = 0xffffffff - scsis->storage_descriptor->hidden_sectors + scsis->media->num_sectors + 1;
        }
        else
            scsis->media->num_sectors -= scsis->storage_descriptor->hidden_sectors;
    } while (false);
    //media descriptor responded, inform host on ready state
    scsis->io = NULL;
    scsis_cb_host(scsis, SCSIS_RESPONSE_RELEASE_IO, 0);
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
    case IPC_READ:
    case IPC_WRITE:
        //IDLE - means resetted before - ignore IO messages
        if (scsis->state != SCSIS_STATE_IDLE)
            scsis_bc_storage_io_complete(scsis, (int)ipc->param3);
        break;
    case STORAGE_GET_MEDIA_DESCRIPTOR:
        scsis_get_media_descriptor(scsis, (int)ipc->param3);
        break;
    case STORAGE_NOTIFY_STATE_CHANGE:
        if (scsis->storage_descriptor->flags & SCSI_STORAGE_DESCRIPTOR_REMOVABLE)
        {
#if (SCSI_MMC)
            scsis->media_status_changed = true;
#endif //SCSI_MMC
            if (scsis->media != NULL)
                scsis_media_removed(scsis);
            else
                scsis_request_media(scsis);
            storage_request_notify_state_change(scsis->storage_descriptor->hal, scsis->storage_descriptor->storage, scsis->storage_descriptor->user);
        }
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
    }
}
