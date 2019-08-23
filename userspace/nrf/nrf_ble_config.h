/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: RJ (jam_roma@yahoo.com)
*/

#ifndef _NRF_BLE_CONFIG_H_
#define _NRF_BLE_CONFIG_H_

#include "nrf_radio_config.h"

// ============================= DEBUG ========================================
#if (RADIO_DEBUG)
#define BLE_DEBUG_ADV_RAW                   0
#define BLE_DEBUG_ADV_COMMON                1
#define BLE_DEBUG_ADV_DATA                  1
#endif // RADIO_DEBUG
// ============================= SETTINGS =====================================

#define RADIO_BLE_PKT_ADDR_SIZE             6

#pragma pack(push, 1)
typedef struct {
    uint8_t length;
    uint8_t type;
    uint8_t value;
} RADIO_BLE_LTV;

typedef struct {
    uint8_t type;
    uint8_t length;
    uint8_t addr[RADIO_BLE_PKT_ADDR_SIZE];
    uint8_t data;
} RADIO_BLE_PKT_HEADER;
#pragma pack(pop)

// Radio BLE Packet Type
#define BLE_TYPE_Msk(type)                      ((type) & 0x0F)
#define BLE_TYPE_ADV_IND                        0x00
#define BLE_TYPE_ADV_DIRECT_IND                 0x01
#define BLE_TYPE_ADV_NONCONN_IND                0x02
#define BLE_TYPE_SCAN_REQ                       0x03
#define BLE_TYPE_SCAN_RSP                       0x04
#define BLE_TYPE_CONNECT_REQ                    0x05
#define BLE_TYPE_ADV_SCAN_IND                   0x06
/* Reserved bits 0111-1111 */

// Advertisement segment types
#define BLE_ADV_TYPE_FLAGS                      0x1
#define BLE_ADV_TYPE_UUID16_INC                 0x2
#define BLE_ADV_TYPE_UUID16                     0x3
#define BLE_ADV_TYPE_UUID32_INC                 0x4
#define BLE_ADV_TYPE_UUID32                     0x5
#define BLE_ADV_TYPE_UUID128_INC                0x6
#define BLE_ADV_TYPE_UUID128                    0x7
#define BLE_ADV_TYPE_NAME_SHORT                 0x8
#define BLE_ADV_TYPE_NAME                       0x9
#define BLE_ADV_TYPE_TRANSMITPOWER              0xA
#define BLE_ADV_TYPE_CONNINTERVAL               0x12
#define BLE_ADV_TYPE_SERVICE_SOLICITATION16     0x14
#define BLE_ADV_TYPE_SERVICE_SOLICITATION128    0x15
#define BLE_ADV_TYPE_SERVICEDATA                0x16
#define BLE_ADV_TYPE_APPEARANCE                 0x19
#define BLE_ADV_TYPE_MANUFACTURER_SPECIFIC      0xFF

// Advertise frequency offset settings
#define BLE_ADV_CHANNEL_37                      2
#define BLE_ADV_CHANNEL_38                      26
#define BLE_ADV_CHANNEL_39                      80

// Channel frequecy offset settings
#define BLE_DATA_CHANNEL_0                      4
#define BLE_DATA_CHANNEL_1                      6
#define BLE_DATA_CHANNEL_2                      8
#define BLE_DATA_CHANNEL_3                      10
#define BLE_DATA_CHANNEL_4                      12
#define BLE_DATA_CHANNEL_5                      14
#define BLE_DATA_CHANNEL_6                      16
#define BLE_DATA_CHANNEL_7                      18
#define BLE_DATA_CHANNEL_8                      20
#define BLE_DATA_CHANNEL_9                      22
#define BLE_DATA_CHANNEL_10                     24
#define BLE_DATA_CHANNEL_11                     28
#define BLE_DATA_CHANNEL_12                     30
#define BLE_DATA_CHANNEL_13                     32
#define BLE_DATA_CHANNEL_14                     34
#define BLE_DATA_CHANNEL_15                     36
#define BLE_DATA_CHANNEL_16                     38
#define BLE_DATA_CHANNEL_17                     40
#define BLE_DATA_CHANNEL_18                     42
#define BLE_DATA_CHANNEL_19                     44
#define BLE_DATA_CHANNEL_20                     46
#define BLE_DATA_CHANNEL_21                     48
#define BLE_DATA_CHANNEL_22                     50
#define BLE_DATA_CHANNEL_23                     52
#define BLE_DATA_CHANNEL_24                     54
#define BLE_DATA_CHANNEL_25                     56
#define BLE_DATA_CHANNEL_26                     58
#define BLE_DATA_CHANNEL_27                     60
#define BLE_DATA_CHANNEL_28                     62
#define BLE_DATA_CHANNEL_29                     64
#define BLE_DATA_CHANNEL_30                     66
#define BLE_DATA_CHANNEL_31                     68
#define BLE_DATA_CHANNEL_32                     70
#define BLE_DATA_CHANNEL_33                     72
#define BLE_DATA_CHANNEL_34                     74
#define BLE_DATA_CHANNEL_35                     76
#define BLE_DATA_CHANNEL_36                     78

// ------------------------------ BLE -------------------------------------------
// Errors
#define BLE_ERROR_SUCCESS                       0x00
#define BLE_ERROR_UNKNOWN_HCI_COMMAND           0x01
#define BLE_ERROR_UNKNOWN_CONN_ID               0x02
#define BLE_ERROR_HARDWARE                      0x03
#define BLE_ERROR_PAGE_TIMEOUT                  0x04
#define BLE_ERROR_AUTHENTICATION_FAIL           0x05
#define BLE_ERROR_PIN_OR_KEY_MISSING            0x06
#define BLE_ERROR_MEMORY_CAPACITY_EXCEEDED      0x07
#define BLE_ERROR_CONN_TIMEOUT                  0x08
#define BLE_ERROR_CONN_LIMIT_EXCEEDED           0x09
#define BLE_ERROR_SYNC_CONN_LIMIT_TO_DEVICE_EXCEEDED 0x0A
#define BLE_ERROR_ACL_CONN_ALREADY_EXISTS       0x0B
#define BLE_ERROR_COMMAND_DISALLOWED            0x0C
#define BLE_ERROR_CONN_REJECTED_LIMITED_RES     0x0D
#define BLE_ERROR_CONN_REJECTED_SECURYTY        0x0E
#define BLE_ERROR_CONN_REJECTED_UNACCEPTABLE_BD_ADDR  0x0F
#define BLE_ERROR_CONN_ACCEPT_TIMEOUT_EXCEEDED  0x10
#define BLE_ERROR_UNSUPPORTED_FEATURE           0x11
#define BLE_ERROR_INVALID_HCI_COMMAND_PARAMETERS 0x12
#define BLE_ERROR_REMOTE_USER_TERMINATED_CONN   0x13
#define BLE_ERROR_REMOTE_DEVICE_TERMINATED_CONN_LOW_RES 0x14
#define BLE_ERROR_REMOTE_DEVICE_TERMINATED_CONN_POWER_OFF 0x15
#define BLE_ERROR_TERMINATED_BY_LOCAL_HOST      0x16
#define BLE_ERROR_REPEATED_ATTEMPTS             0x17
#define BLE_ERROR_PAIRING_NOT_ALLOWED           0x18
#define BLE_ERROR_UNKNOWN_LMP_PDU               0x19
#define BLE_ERROR_UNSUPPORTED_REMOTE_FEATURE    0x1A
#define BLE_ERROR_SCO_OFFSET_REJECTED           0x1B
#define BLE_ERROR_SCO_INTERVAL_REJECTED         0x1C
#define BLE_ERROR_SCO_AIR_MODE_REJECTED         0x1D
#define BLE_ERROR_INVALID_LMP_PARAMS            0x1E
#define BLE_ERROR_UNSPECIFIED_ERROR             0x1F
#define BLE_ERROR_UNSUPPORTED_LMP_PARAMETER     0x20
#define BLE_ERROR_ROLE_CHANGE_NOT_ALLOWED       0x21
#define BLE_ERROR_LMP_RESPONSE_TIMEOUT          0x22
#define BLE_ERROR_LMP_ERROR_TRANSACTION_COLLISION 0x23
#define BLE_ERROR_LMP_PDU_NOT_ALLOWED           0x24
#define BLE_ERROR_ENC_MODE_NOT_ACCEPTABLE       0x25
#define BLE_ERROR_LINK_KEY_CANT_CHANGED         0x26
#define BLE_ERROR_REQUESTED_QOS_NOT_SUPPORTED   0x27
#define BLE_ERROR_INSTANT_PASSED                0x28
#define BLE_ERROR_PAIRING_WITH_UNIT_KEY_NOT_SUPPORTED 0x29
#define BLE_ERROR_DIFFERENT_TRANSACTION_COLLISON 0x2A
#define BLE_ERROR_RESERVED1                     0x2B
#define BLE_ERROR_QOS_UNACCEPTABLE_PARAMETER    0x2C
#define BLE_ERROR_QOS_REJECTED                  0x2D
#define BLE_ERROR_CHANNEL_CLASSIFICATION_NOT_SUPPORTED 0x2E
#define BLE_ERROR_INSUFFICIENT_SECURITY         0x2F
#define BLE_ERROR_PARAMETER_OUT_OF_RANGE        0x30
#define BLE_ERROR_RESERVED2                     0x31
#define BLE_ERROR_ROLE_SWITCH_PENDING           0x32
#define BLE_ERROR_RESERVED3                     0x33
#define BLE_ERROR_RESERVED_SLOT_VIOLATION       0x34
#define BLE_ERROR_ROLE_SWITCH_FAILED            0x35
#define BLE_ERROR_EXTENDED_INQUIRY_RESPONSE_TOO_LARGE 0x36
#define BLE_ERROR_SECURE_PAIRING_NOT_SUPPORTED  0x37
#define BLE_ERROR_HOST_BUSY_PAIRING             0x38
#define BLE_ERROR_CONN_REJECTED_NO_SUITABLE_CHANNEL 0x39
#define BLE_ERROR_CONTROLLER_BUSY               0x3A
#define BLE_ERROR_UNACCEPTABLE_COON_INTERVAL    0x3B
#define BLE_ERROR_DERECTED_ADVERTISING_TIMEOUT  0x3C
#define BLE_ERROR_CONN_TERMINATED_MIC_FAILURE   0x3D
#define BLE_ERROR_CONN_FAILED_ESTABLISHED       0x3E
#define BLE_ERROR_MAC_CONNECTION_FAILED         0x3F


#endif /* _NRF_BLE_CONFIG_H_ */
