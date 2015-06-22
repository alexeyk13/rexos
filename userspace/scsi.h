/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef SCSI_H
#define SCSI_H

#include <stdint.h>

#define SCSI_CMD_TEST_UNIT_READY                                        0x00
#define SCSI_CMD_REQUEST_SENSE                                          0x03
#define SCSI_CMD_FORMAT_UNIT                                            0x04
#define SCSI_CMD_REASSIGN_BLOCKS                                        0x07
#define SCSI_CMD_READ6                                                  0x08
#define SCSI_CMD_WRITE6                                                 0x0a

#define SCSI_CMD_INQUIRY                                                0x12
#define SCSI_CMD_VERIFY6                                                0x13
#define SCSI_CMD_MODE_SELECT6                                           0x15
#define SCSI_CMD_MODE_SENSE6                                            0x1a
#define SCSI_CMD_START_STOP_UNIT                                        0x1b
#define SCSI_CMD_RECEIVE_DIAGNOSTIC_RESULTS                             0x1c
#define SCSI_CMD_SEND_DIAGNOSTIC                                        0x1d
#define SCSI_CMD_PREVENT_ALLOW_MEDIUM_REMOVAL                           0x1e

#define SCSI_CMD_READ_FORMAT_CAPACITY                                   0x23
#define SCSI_CMD_READ_CAPACITY10                                        0x25
#define SCSI_CMD_READ10                                                 0x28
#define SCSI_CMD_WRITE10                                                0x2a
#define SCSI_CMD_WRITE_AND_VERIFY10                                     0x2e
#define SCSI_CMD_VERIFY10                                               0x2f

#define SCSI_CMD_PREFETCH10                                             0x34
#define SCSI_CMD_SYNCHRONIZE_CACHE10                                    0x35
#define SCSI_CMD_READ_DEFECT_DATA10                                     0x37
#define SCSI_CMD_WRITE_BUFFER                                           0x3b
#define SCSI_CMD_READ_BUFFER                                            0x3c
#define SCSI_CMD_READ_LONG10                                            0x3e
#define SCSI_CMD_WRITE_LONG10                                           0x3f
#define SCSI_CMD_WRITE_SAME10                                           0x41
#define SCSI_CMD_LOG_SELECT                                             0x4c
#define SCSI_CMD_LOG_SENSE                                              0x4d

#define SCSI_CMD_XD_WRITE10                                             0x50
#define SCSI_CMD_XP_WRITE10                                             0x51
#define SCSI_CMD_XD_READ10                                              0x52
#define SCSI_CMD_XD_WRITE_READ10                                        0x53
#define SCSI_CMD_MODE_SELECT10                                          0x55
#define SCSI_CMD_MODE_SENSE10                                           0x5a
#define SCSI_CMD_PERSISTENT_RESERVE_IN                                  0x5e
#define SCSI_CMD_PERSISTENT_RESERVE_OUT                                 0x5f

#define SCSI_CMD_READ32                                                 0x7f
#define SCSI_CMD_WRITE32                                                0x7f
#define SCSI_CMD_WRITE_AND_VERIFY32                                     0x7f
#define SCSI_CMD_VERIFY32                                               0x7f
#define SCSI_CMD_WRITE_SAME32                                           0x7f
#define SCSI_CMD_XD_READ32                                              0x7f
#define SCSI_CMD_XD_WRITE32                                             0x7f
#define SCSI_CMD_XP_WRITE32                                             0x7f
#define SCSI_CMD_XD_WRITE_READ32                                        0x7f

#define SCSI_CMD_EXTENDED_COPY                                          0x83
#define SCSI_CMD_RECEIVE_COPY_RESULTS                                   0x84
#define SCSI_CMD_READ16                                                 0x88
#define SCSI_CMD_WRITE16                                                0x8a
#define SCSI_CMD_READ_ATTRIBUTE                                         0x8c
#define SCSI_CMD_WRITE_ATTRIBUTE                                        0x8d
#define SCSI_CMD_WRITE_AND_VERIFY16                                     0x8e
#define SCSI_CMD_VERIFY16                                               0x8f

#define SCSI_CMD_PREFETCH16                                             0x90
#define SCSI_CMD_SYNCHRONIZE_CACHE16                                    0x91
#define SCSI_CMD_WRITE_SAME16                                           0x93
#define SCSI_CMD_READ_CAPACITY16                                        0x9e
#define SCSI_CMD_READ_LONG16                                            0x9e
#define SCSI_CMD_WRITE_LONG16                                           0x9f

#define SCSI_CMD_REPORT_LUNS                                            0xa0
#define SCSI_CMD_REPORT_ALIASES                                         0xa3
#define SCSI_CMD_REPORT_PRIORITY                                        0xa3
#define SCSI_CMD_REPORT_SUPPORTED_OPERATION_CODES                       0xa3
#define SCSI_CMD_REPORT_SUPPORTED_TASK_MANAGEMENT_FUNCTIONS             0xa3
#define SCSI_CMD_REPORT_TARGET_PORT_GROUPS                              0xa3
#define SCSI_CMD_REPORT_TIMESTAMP                                       0xa3
#define SCSI_CMD_REPORT_DEVICE_IDENTIFIER                               0xa3
#define SCSI_CMD_CHANGE_ALIASES                                         0xa4
#define SCSI_CMD_SET_DEVICE_IDENTIFIER                                  0xa4
#define SCSI_CMD_SET_PRIORITY                                           0xa4
#define SCSI_CMD_SET_TARGET_PORT_GROUPS                                 0xa4
#define SCSI_CMD_SET_TIMESTAMP                                          0xa4
#define SCSI_CMD_READ12                                                 0xa8
#define SCSI_CMD_WRITE12                                                0xaa
#define SCSI_CMD_READ_MEDIA_SERIAL_NUMBER                               0xab
#define SCSI_CMD_WRITE_AND_VERIFY12                                     0xae
#define SCSI_CMD_VERIFY12                                               0xaf

#define SCSI_CMD_READ_DEFECT_DATA12                                     0xb7

#define SCSI_INQUIRY_CMD_DT                                             (1 << 1)
#define SCSI_INQUIRY_EVPD                                               (1 << 0)

#define SCSI_VERIFY_BYTCHK                                              (1 << 1)

//codes for EPVD
#define INQUIRY_VITAL_PAGE_SUPPORTED_PAGES                              0x00
#define INQUIRY_VITAL_PAGE_SERIAL_NUM                                   0x80
#define INQUIRY_VITAL_PAGE_ASCII_OPERATIONS                             0x82
#define INQUIRY_VITAL_PAGE_DEVICE_INFO                                  0x83

#pragma pack(push, 1)

typedef enum {
    SCSI_REQUEST_READ = 0,
    SCSI_REQUEST_WRITE,
    SCSI_REQUEST_VERIFY,
    SCSI_REQUEST_STORAGE_INFO,
    SCSI_REQUEST_MEDIA_INFO
} SCSI_REQUEST;

typedef struct {
    SCSI_REQUEST request;
    unsigned int addr, count;
} SCSI_STACK;

#pragma pack(pop)

#endif // SCSI_H
