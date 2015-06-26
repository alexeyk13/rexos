/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef SCSIS_PRIVATE_H
#define SCSIS_PRIVATE_H

#include <stdint.h>
#include "sys_config.h"
#include "../../userspace/rb.h"
#include "../../userspace/io.h"
#include "../../userspace/scsi.h"
#include "scsis.h"

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
#define SCSI_CMD_READ_CAPACITY16                                        0x109e
#define SCSI_CMD_READ_LONG16                                            0x119e
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

//------------------------------- SCSI inquiry specific --------------------------------------------------
#define SCSI_INQUIRY_EVPD                                               (1 << 0)

#define SCSI_PERIPHERAL_QUALIFIER_MASK                                  (7 << 5)
#define SCSI_PERIPHERAL_QUALIFIER_DEVICE_CONNECTED                      (0 << 5)
#define SCSI_PERIPHERAL_QUALIFIER_DEVICE_NOT_CONNECTED                  (1 << 5)
#define SCSI_PERIPHERAL_QUALIFIER_DEVICE_NOT_SUPPORTED                  (3 << 5)

#define SCSI_PERIPHERAL_DEVICE_TYPE_MASK                                (0x1f << 0)


//codes for EPVD
#define INQUIRY_VITAL_PAGE_SUPPORTED_PAGES                              0x00
#define INQUIRY_VITAL_PAGE_SERIAL_NUM                                   0x80
#define INQUIRY_VITAL_PAGE_ASCII_OPERATIONS                             0x82
#define INQUIRY_VITAL_PAGE_DEVICE_INFO                                  0x83

#define SCSI_VERIFY_BYTCHK                                              (1 << 1)

//----------------------------- SCSI read capacity specific ------------------------------------------------
#define SCSI_READ_CAPACITY_PMI                                          (1 << 0)

//------------------------------- SCSI mode sense specific -------------------------------------------------
#define SCSI_MODE_SENSE_DBD                                             (1 << 3)
#define SCSI_MODE_SENSE_LLBAA                                           (1 << 4)

//SPC
#define MODE_SENSE_PSP_CONTROL_ALL                                      0x0aff
#define MODE_SENSE_PSP_POWER_CONDITION                                  0x1a00
#define MODE_SENSE_PSP_POWER_CONSUMPTION                                0x1a01
#define MODE_SENSE_PSP_POWER_ALL                                        0x1aff

#define MODE_SENSE_PSP_ALL_PAGES                                        0x3f00
#define MODE_SENSE_PSP_ALL_PAGES_SUBPAGES                               0x3fff

//SBC
#define MODE_SENSE_PSP_READ_WRITE_ERROR_RECOVERY                        0x0100
#define MODE_SENSE_PSP_DISCONNECT_RECONNECT                             0x0200
#define MODE_SENSE_PSP_VERIFY_ERROR_RECOVERY                            0x0700
#define MODE_SENSE_PSP_CACHING                                          0x0800
#define MODE_SENSE_PSP_CONTROL                                          0x0a00
#define MODE_SENSE_PSP_CONTROL_EX                                       0x0a01
#define MODE_SENSE_PSP_APPLICATION_TAG                                  0x0a02
#define MODE_SENSE_PSP_XOR_CONTROL                                      0x1000
#define MODE_SENSE_PSP_ENCLOSURE_SERVICE_MANAGEMENT                     0x1400
#define MODE_SENSE_PSP_INFORMATION_EXCEPTION_CONTROL                    0x1c00
#define MODE_SENSE_PSP_BACKGROUND_CONTROL                               0x1c01
#define MODE_SENSE_PSP_LOGICAL_BLOCK_PROVISIONING                       0x1c02
#define MODE_SENSE_PSP_SERVICE_ALL                                      0x1cff

//----------------------------- sense key for error recovery -----------------------------------------------
#define SENSE_KEY_NO_SENSE                                              0x00
#define SENSE_RECOVERED_ERROR                                           0x01
#define SENSE_KEY_NOT_READY                                             0x02
#define SENSE_KEY_MEDIUM_ERROR                                          0x03
#define SENSE_KEY_HARDWARE_ERROR                                        0x04
#define SENSE_KEY_ILLEGAL_REQUEST                                       0x05
#define SENSE_KEY_UNIT_ATTENTION                                        0x06
#define SENSE_KEY_DATA_PROTECT                                          0x07
#define SENSE_KEY_BLANK_CHECK                                           0x08
#define SENSE_KEY_VENDOR_SPECIFIC                                       0x09
#define SENSE_KEY_COPY_ABORTED                                          0x0a
#define SENSE_KEY_ABORTED_COMMAND                                       0x0b
#define SENSE_KEY_VOLUME_OVERFLOW                                       0x0d
#define SENSE_KEY_MISCOMPARE                                            0x0e

//----------------------------------- ASC + ASQ qualifier codes ---------------------------------------------
#define ASCQ_NO_ADDITIONAL_SENSE_INFORMATION                            0x0000
#define ASCQ_PERIPHERAL_DEVICE_WRITE_FAULT                              0x0300
#define ASCQ_LOGICAL_UNIT_COMMUNICATION_FAILURE                         0x0800
#define ASCQ_LOGICAL_UNIT_COMMUNICATION_TIMEOUT                         0x0801
#define ASCQ_WRITE_ERROR                                                0x0c00
#define ASCQ_ERROR_LOG_OVERFLOW                                         0x0a00
#define ASCQ_UNRECOVERED_READ_ERROR                                     0x1100
#define ASCQ_READ_RETRIES_EXHAUSTED                                     0x1101
#define ASCQ_MISCOMPARE_DURING_VERIFY_OPERATION                         0x1d00
#define ASCQ_INVALID_COMMAND_OPERATION_CODE                             0x2000
#define ASCQ_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE                         0x2101

#define ASCQ_INVALID_FIELD_IN_CDB                                       0x2400
#define ASCQ_CDB_DECRYPTION_ERROR                                       0x2401

#define ASCQ_INVALID_FIELD_IN_PARAMETER_LIST                            0x2600
#define ASCQ_WRITE_PROTECTED                                            0x2700
#define ASCQ_COMMAND_DEVICE_INTERNAL_RESET                              0x2904
#define ASCQ_COMMAND_SEQUENCE_ERROR                                     0x2c00
#define ASCQ_MEDIUM_NOT_PRESENT                                         0x3a00
#define ASCQ_INTERNAL_TARGET_FAILURE                                    0x4400
#define ASCQ_COMMAND_PHASE_ERROR                                        0x4a00
#define ASCQ_DATA_PHASE_ERROR                                           0x4b00

typedef struct {
    uint8_t key_sense;
    uint8_t align;
    uint16_t ascq;
} SCSIS_ERROR;

typedef struct _SCSIS {
    SCSI_STORAGE_DESCRIPTOR** storage;
    SCSI_MEDIA_DESCRIPTOR** media;
    bool storage_request;
    SCSIS_ERROR errors[SCSI_SENSE_DEPTH];
    RB rb_error;
} SCSIS;

void scsis_error_init(SCSIS* scsis);
void scsis_error(SCSIS* scsis, uint8_t key_sense, uint16_t ascq);
void scsis_error_get(SCSIS* scsis, SCSIS_ERROR error);
SCSIS_RESPONSE scsis_get_storage_descriptor(SCSIS* scsis, IO* io);
SCSIS_RESPONSE scsis_get_media_descriptor(SCSIS* scsis, IO* io);
//failure if no media inserted
SCSIS_RESPONSE scsis_get_media(SCSIS* scsis, IO* io);

#endif // SCSIS_PRIVATE_H
