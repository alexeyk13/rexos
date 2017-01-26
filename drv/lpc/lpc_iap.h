/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef LPC_IAP_H
#define LPC_IAP_H

#include <stdbool.h>

#define IAP_CMD_INIT                                49
#define IAP_CMD_PREPARE_SECTORS_FOR_WRITE           50
#define IAP_CMD_COPY_RAM_TO_FLASH                   51
#define IAP_CMD_ERASE_SECTORS                       52
#define IAP_CMD_BLANK_CHECK_SECTORS                 53
#define IAP_CMD_READ_PART_ID                        54
#define IAP_CMD_READ_BOOT_CODE_VERSION              55
#define IAP_CMD_COMPARE                             56
#define IAP_CMD_REINVOKE_ISP                        57
#define IAP_CMD_READ_DEVICE_SERIAL                  58
#define IAP_CMD_ERASE_PAGE                          59
#define IAP_CMD_SET_ACTIVE_BOOT_FLASH               60
#define IAP_CMD_WRITE_EEPROM                        61
#define IAP_CMD_READ_EEPROM                         62

#define IAP_RESULT_SUCCESS                          0x00000000
#define IAP_RESULT_INVALID_COMMAND                  0x00000001
#define IAP_RESULT_SRC_ADDRESS_ERROR                0x00000002
#define IAP_RESULT_DST_ADDRESS_ERROR                0x00000003
#define IAP_RESULT_SRC_ADDRESS_NOT_MAPPED           0x00000004
#define IAP_RESULT_DST_ADDRESS_NOT_MAPPED           0x00000005
#define IAP_RESULT_COUNT_ERROR                      0x00000006
#define IAP_RESULT_INVALID_SECTOR                   0x00000007
#define IAP_RESULT_SECTOR_NOT_BLANK                 0x00000008
#define IAP_RESULT_SECTOR_NOT_PREPARED_FOR_WRITE    0x00000009
#define IAP_RESULT_COMPARE_ERROR                    0x0000000a
#define IAP_RESULT_BUSY                             0x0000000b
#define IAP_RESULT_PARAM_ERROR                      0x0000000c
#define IAP_RESULT_ADDRESS_ERROR                    0x0000000d
#define IAP_RESULT_ADDRESS_NOT_MAPPED               0x0000000e
#define IAP_RESULT_CMD_LOCKED                       0x0000000f
#define IAP_RESULT_INVALID_CODE                     0x00000010
#define IAP_RESULT_INVALID_BAUD_RATE                0x00000011
#define IAP_RESULT_INVALID_STOP_BIT                 0x00000012
#define IAP_RESULT_CODE_READ_PROTECTION             0x00000013
#define IAP_RESULT_INVALID_FLASH_UNIT               0x00000014
#define IAP_RESULT_USER_CODE_CHECKSUM               0x00000015
#define IAP_RESULT_ERROR_SETTING_ACTIVE_PARTITION   0x00000016

typedef struct {
    unsigned int req[6];
    unsigned int resp[6];
} LPC_IAP_TYPE;

bool lpc_iap(LPC_IAP_TYPE* params, unsigned int cmd);

#endif // LPC_IAP_H
