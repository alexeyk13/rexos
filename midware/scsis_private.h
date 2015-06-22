/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef SCSIS_PRIVATE_H
#define SCSIS_PRIVATE_H

#include <stdint.h>
#include "sys_config.h"
#include "../userspace/rb.h"

//sense key for error recovery
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

//ASC + ASQ qualifier codes
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
#define ASCQ_COMMAND_PHASE_ERROR                                        0x4a00
#define ASCQ_DATA_PHASE_ERROR                                           0x4b00


typedef struct {
    uint8_t key_sense;
    uint8_t align;
    uint16_t ascq;
} SCSIS_ERROR;

typedef struct _SCSIS {
    void* storage;
    void* media;
    SCSIS_ERROR errors[SCSI_SENSE_DEPTH];
    RB rb_error;
} SCSIS;

void scsis_error_init(SCSIS* scsis);
void scsis_error_reset(SCSIS* scsis);
void scsis_error(SCSIS* scsis, uint8_t key_sense, uint16_t ascq);
void scsis_error_get(SCSIS* scsis, SCSIS_ERROR error);

#endif // SCSIS_PRIVATE_H
