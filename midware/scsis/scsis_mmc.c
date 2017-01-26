/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "scsis_mmc.h"
#include "scsis_private.h"
#include "../../userspace/stdio.h"
#include "../../userspace/endian.h"
#include "sys_config.h"
#include <string.h>

#define NOTIFICATION_CLASS_REQUEST_UNSUPPORTED_MASK                                 (1 << 0)
#define NOTIFICATION_CLASS_REQUEST_OPERATIONAL_CHANGE_MASK                          (1 << 1)
#define NOTIFICATION_CLASS_REQUEST_POWER_MANAGEMENT_MASK                            (1 << 2)
#define NOTIFICATION_CLASS_REQUEST_EXTERNAL_MASK                                    (1 << 3)
#define NOTIFICATION_CLASS_REQUEST_MEDIA_MASK                                       (1 << 4)
#define NOTIFICATION_CLASS_REQUEST_MULTI_HOST_MASK                                  (1 << 5)
#define NOTIFICATION_CLASS_REQUEST_DEVICE_BUSY_MASK                                 (1 << 6)
#define NOTIFICATION_CLASS_REQUEST_RFU_MASK                                         (1 << 7)

#define NOTIFICATION_CLASS_REQUEST_UNSUPPORTED                                      0
#define NOTIFICATION_CLASS_REQUEST_OPERATIONAL_CHANGE                               1
#define NOTIFICATION_CLASS_REQUEST_POWER_MANAGEMENT                                 2
#define NOTIFICATION_CLASS_REQUEST_EXTERNAL                                         3
#define NOTIFICATION_CLASS_REQUEST_MEDIA                                            4
#define NOTIFICATION_CLASS_REQUEST_MULTI_HOST                                       5
#define NOTIFICATION_CLASS_REQUEST_DEVICE_BUSY                                      6
#define NOTIFICATION_CLASS_REQUEST_RFU                                              7

#define GET_EVENT_STATUS_NEA                                                        (1 << 7)

static void scsis_mmc_lba_to_msf(uint32_t lba, uint8_t* m, uint8_t* s, uint8_t* f)
{
    uint32_t lba_tmp, m_tmp, s_tmp;
    lba_tmp = lba + 150;
    m_tmp = lba_tmp / (60 * 75);
    *m = m_tmp;
    m_tmp = m_tmp * 60 * 75;
    s_tmp = (lba_tmp - m_tmp) / 75;
    *s = s_tmp;
    s_tmp = s_tmp * 75;
    *f = lba_tmp - m_tmp - s_tmp;
}

void scsis_mmc_read_toc(SCSIS* scsis, uint8_t* req)
{
    uint8_t* resp;
    uint8_t msf, fmt, m, s, f;
    uint16_t len;
    msf = (req[1] >> 1) & 1;
    fmt = (req[2] & 0xf);
    len = be2short(req + 7);
    if (fmt > 0x1)
    {
        scsis_fail(scsis, SENSE_KEY_ILLEGAL_REQUEST, ASCQ_INVALID_FIELD_IN_CDB);
        return;
    }
    if (!scsis_get_media(scsis))
        return;
#if (SCSI_DEBUG_REQUESTS)
    printf("SCSI Read TOC Format: %d%s\n", fmt, msf ? ", MSF" : "");
#endif //SCSI_DEBUG_REQUESTS

    scsis->io->data_size = 20;
    memset(io_data(scsis->io), 0, scsis->io->data_size);
    if (scsis->io->data_size > len)
        scsis->io->data_size = len;
    resp = io_data(scsis->io);

    //0012 - size
    short2be(resp, 0x12);
    //first track
    resp[2] = 1;
    //last track
    resp[3] = 1;

    //track 1
    //ADR, CONTROL
    resp[5] = 0x14;
    //track number
    resp[6] = 1;
    if (msf)
        //00:02:00
        resp[10] = 0x02;
    //LBA = 0x00000000

    //lead OUT
    //ADR, CONTROL
    resp[13] = 0x14;
    //track number
    resp[14] = 0xaa;
    if (msf)
    {
        scsis_mmc_lba_to_msf(scsis->media->num_sectors, &m, &s, &f);
        //MM:SS:FF
        resp[17] = m;
        resp[18] = s;
        resp[19] = f;
    }
    //LBA
    else
        int2be(resp + 16, scsis->media->num_sectors);

    scsis->state = SCSIS_STATE_COMPLETE;
    scsis_cb_host(scsis, SCSIS_RESPONSE_WRITE, scsis->io->data_size);
}

unsigned int scsis_mmc_append_feature_profile_list(SCSIS* scsis, uint8_t* buf)
{
    //Feature: profile list
    short2be(buf, SCSIS_MMC_FEATURE_PROFILE_LIST);
    //Persistent, Current
    buf[2] = (1 << 0) | (1 << 1);
    buf[3] = 4;
    short2be(buf + 4, SCSIS_MMC_PROFILE_DVD_ROM);
    //current if media present
    buf[6] = scsis->media ? (1 << 0) : 0;
    buf[7] = 0;
    return 8;
}

unsigned int scsis_mmc_append_feature_core(SCSIS* scsis, uint8_t* buf)
{
    //Feature: profile list
    short2be(buf, SCSIS_MMC_FEATURE_CORE);
    //Persistent, Current, Version = 2
    buf[2] = (1 << 0) | (1 << 1) | (2 << 2);
    buf[3] = 8;
    //Physical interface - USB
    short2be(buf + 4, 8);
    //DBE=1
    buf[8] = (1 << 0);
    buf[9] = buf[10] = buf[11] = 0;
    return 12;
}

void scsis_mmc_get_configuration(SCSIS* scsis, uint8_t* req)
{
    uint16_t start_feature, len;
    uint8_t rt;
    uint8_t* resp;
    rt = req[1] & 3;
    start_feature = be2short(req + 2);
    len = be2short(req + 7);
#if (SCSI_DEBUG_REQUESTS)
    printf("SCSI Get Configuration: rt: %d, start: %#04X\n", rt, start_feature);
#endif //SCSI_DEBUG_REQUESTS

    //fill header in any case
    scsis->io->data_size = 8;
    memset(io_data(scsis->io), 0, scsis->io->data_size);
    resp = io_data(scsis->io);

    if (scsis->media)
        //DVD-ROM profile by default
        short2be(resp + 6, 0x0010);

    //respond only selected feature
    if (rt == 2)
    {
        switch (start_feature)
        {
        case SCSIS_MMC_FEATURE_PROFILE_LIST:
            scsis->io->data_size += scsis_mmc_append_feature_profile_list(scsis, resp + scsis->io->data_size);
            break;
        case SCSIS_MMC_FEATURE_CORE:
            scsis->io->data_size += scsis_mmc_append_feature_core(scsis, resp + scsis->io->data_size);
            break;
        default:
            break;
        }
    }
    else
    {
        scsis->io->data_size += scsis_mmc_append_feature_profile_list(scsis, resp + scsis->io->data_size);
        scsis->io->data_size += scsis_mmc_append_feature_core(scsis, resp + scsis->io->data_size);
    }

    if (scsis->io->data_size > len)
        scsis->io->data_size = len;
    int2be(resp, scsis->io->data_size - 4);

    scsis->state = SCSIS_STATE_COMPLETE;
    scsis_cb_host(scsis, SCSIS_RESPONSE_WRITE, scsis->io->data_size);
}

void scsis_mmc_get_event_status_notification(SCSIS* scsis, uint8_t* req)
{
    uint16_t len;
    uint8_t* resp;
#if (SCSI_DEBUG_REQUESTS)
    printf("SCSI Get Event Status Notification: %s\n", (req[1] & (1 << 0)) ? "poll" : "async");
#endif //SCSI_DEBUG_REQUESTS
    if ((req[1] & (1 << 0)) == 0)
    {
        scsis_fail(scsis, SENSE_KEY_ILLEGAL_REQUEST, ASCQ_INVALID_FIELD_IN_CDB);
        return;
    }
    len = be2short(req + 7);

    //prepare header - no failed requests should be reported
    scsis->io->data_size = 4;
    resp = io_data(scsis->io);
    //event header supported classes mask
    resp[3] = NOTIFICATION_CLASS_REQUEST_MEDIA_MASK;

    if (((req[4] & NOTIFICATION_CLASS_REQUEST_MEDIA_MASK) == 0) ||
        (req[4] & (NOTIFICATION_CLASS_REQUEST_UNSUPPORTED_MASK | NOTIFICATION_CLASS_REQUEST_OPERATIONAL_CHANGE_MASK |
         NOTIFICATION_CLASS_REQUEST_POWER_MANAGEMENT_MASK | NOTIFICATION_CLASS_REQUEST_EXTERNAL_MASK)))
        //NEA
        resp[2] = 1 << 7;
    else
    {
        resp[2] = NOTIFICATION_CLASS_REQUEST_MEDIA;
        scsis->io->data_size += 4;
        //event code
        if (scsis->media_status_changed)
            resp[4] = (scsis->media) ? 2 : 3;
        else
            resp[4] = 0;
        //media present bit
        resp[5] = (scsis->media) ? (1 << 1) : 0;
        //start/stop slot only for changer
        resp[6] = resp[7] = 0;
    }

    if (scsis->io->data_size > len)
        scsis->io->data_size = len;
    short2be(resp, scsis->io->data_size - 4);
    scsis->state = SCSIS_STATE_COMPLETE;
    scsis_cb_host(scsis, SCSIS_RESPONSE_WRITE, scsis->io->data_size);
}
