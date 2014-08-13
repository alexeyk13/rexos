/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef CDC_H
#define CDC_H

#include "../../userspace/process.h"

#define COMMUNICATION_DEVICE_CLASS                                                      0x02

#define CDC_COMM_INTERFACE_CLASS                                                        0x02
//comm subclass
#define CDC_DIRECT_LINE                                                                 0x01
#define CDC_ACM                                                                         0x02
#define CDC_TELEPHONE                                                                   0x03
#define CDC_MULTI_CHANNEL                                                               0x04
#define CDC_CAPI                                                                        0x05
#define CDC_ETHERNET                                                                    0x06
#define CDC_ATM                                                                         0x07
#define CDC_WIRELESS_HANDSET                                                            0x08
#define CDC_DEVICE_MANAGEMENT                                                           0x09
#define CDC_MOBILE_DIRECT_LINE                                                          0x0a
#define CDC_OBEX                                                                        0x0b
#define CDC_ETHERNET_EMULATION                                                          0x0c
#define CDC_NETWORK_CONTROL                                                             0x0d

//comm protocol
#define CDC_CP_GENERIC                                                                  0x00
#define CDC_CP_V250                                                                     0x01
#define CDC_CP_PCCA_101                                                                 0x02
#define CDC_CP_PCCA_101_0                                                               0x03
#define CDC_CP_GSM_707                                                                  0x04
#define CDC_CP_3GPP_2707                                                                0x05
#define CDC_CP_CDMA                                                                     0x06
#define CDC_CP_USB_EEM                                                                  0x07
#define CDC_CP_EXTERNAL                                                                 0xfe
#define CDC_CP_VENDOR                                                                   0xff

#define CDC_DATA_INTERFACE_CLASS                                                        0x0a

//data protocol
#define CDC_DP_GENERIC                                                                  0x00
#define CDC_DP_NETWORK_TRANSFER_BLOCK                                                   0x01
#define CDC_DP_ISDN_BRI                                                                 0x30
#define CDC_DP_HDLC                                                                     0x31
#define CDC_DP_TRANSPARENT                                                              0x32
#define CDC_DP_Q921                                                                     0x50
#define CDC_DP_Q931                                                                     0x51
#define CDC_DP_Q921TM                                                                   0x52
#define CDC_DP_V42BIS                                                                   0x90
#define CDC_DP_EURO_ISDN                                                                0x91
#define CDC_DP_V24                                                                      0x92
#define CDC_DP_CAPI                                                                     0x93
#define CDC_DP_HOST_BASED                                                               0xfd
#define CDC_DP_EXTERNAL                                                                 0xfe
#define CDC_DP_VENDOR                                                                   0xff

//cdc functional descriptors
#define CS_INTERFACE                                                                    0x24
#define CS_ENDPOINT                                                                     0x25

#define CDC_DESCRIPTOR_HEADER                                                           0x00
#define CDC_DESCRIPTOR_CALL_MANAGEMENT                                                  0x01
#define CDC_DESCRIPTOR_ACM                                                              0x02
#define CDC_DESCRIPTOR_DIRECT_LINE                                                      0x03
#define CDC_DESCRIPTOR_TELEPHONE_RINGER                                                 0x04
#define CDC_DESCRIPTOR_TELEPHONE_CALL_REPORTER                                          0x05
#define CDC_DESCRIPTOR_UNION                                                            0x06
#define CDC_DESCRIPTOR_COINTRY_SELECTION                                                0x07
#define CDC_DESCRIPTOR_TELEPHONE_OPERATIONAL_MODE                                       0x08
#define CDC_DESCRIPTOR_USB_TERMINAL                                                     0x09
#define CDC_DESCRIPTOR_NETWORK_TERMINAL                                                 0x0a
#define CDC_DESCRIPTOR_PROTOCOL_UNIT                                                    0x0b
#define CDC_DESCRIPTOR_EXTENSION_UNIT                                                   0x0c
#define CDC_DESCRIPTOR_MULTI_CHANNEL                                                    0x0d
#define CDC_DESCRIPTOR_CAPI                                                             0x0e
#define CDC_DESCRIPTOR_ETHERNET                                                         0x0f
#define CDC_DESCRIPTOR_ATM                                                              0x10
#define CDC_DESCRIPTOR_WIRELESS_HEADSET                                                 0x11
#define CDC_DESCRIPTOR_MOBILE                                                           0x12
#define CDC_DESCRIPTOR_MDLM                                                             0x13
#define CDC_DESCRIPTOR_DEVICE                                                           0x14
#define CDC_DESCRIPTOR_OBEX                                                             0x15
#define CDC_DESCRIPTOR_COMMAND_SET                                                      0x16
#define CDC_DESCRIPTOR_COMMAND_SET_DETAIL                                               0x17
#define CDC_DESCRIPTOR_TELEPHONE_CONTROL                                                0x18
#define CDC_DESCRIPTOR_OBEX_SERVICE                                                     0x19
#define CDC_DESCRIPTOR_NCM                                                              0x1a

#pragma pack(push, 1)

typedef struct {
    uint8_t bFunctionLength;                                                            /* Size of this descriptor in bytes */
    uint8_t bDescriptorType;                                                            /* CS_INTERFACE descriptor type */
    uint8_t bDescriptorSybType;                                                         /* Header function descriptor subtype */
    uint16_t bcdCDC;                                                                    /* USB Class Definitions for Communications Device Specification
                                                                                           release number in binary-coded decimal.*/
} CDC_HEADER_DESCRIPTOR_TYPE;

typedef struct {
    uint8_t bFunctionLength;                                                            /* Size of this descriptor in bytes */
    uint8_t bDescriptorType;                                                            /* CS_INTERFACE descriptor type */
    uint8_t bDescriptorSybType;                                                         /* Header function descriptor subtype */
    uint8_t bControlInterface;                                                          /* The interface number of the Communications
                                                                                           or Data Class interface, designated as the
                                                                                           controlling interface for the union.*/
    //uint8_t bSubordinateInterface[] is following

} CDC_UNION_DESCRIPTOR_TYPE;

typedef struct {
    uint8_t bFunctionLength;                                                            /* Size of this descriptor in bytes */
    uint8_t bDescriptorType;                                                            /* CS_INTERFACE descriptor type */
    uint8_t bDescriptorSybType;                                                         /* Header function descriptor subtype */
    uint8_t bmCapabilities;                                                             /* The capabilities that this configuration supports:
                                                                                           D7..D2:
                                                                                            RESERVED (Reset to zero)
                                                                                           D1:
                                                                                           0 - Device sends/receives call management
                                                                                           information only over the Communications Class
                                                                                           interface.
                                                                                           1 - Device can send/receive call management
                                                                                           information over a Data Class interface.
                                                                                           D0:
                                                                                           0 - Device does not handle call management
                                                                                           itself.
                                                                                           1 - Device handles call management itself.
                                                                                           The previous bits, in combination, identify which call
                                                                                           management scenario is used. If bit D0 is reset to 0, then the
                                                                                           value of bit D1 is ignored. In this case, bit D1 is reset to zero
                                                                                           for future compatibility. */
    uint8_t  bDataInterface;                                                            /* Interface number of Data Class interface optionally used for
                                                                                           call management. */
} CDC_CALL_MANAGEMENT_DESCRIPTOR_TYPE;

typedef struct {
    uint8_t bFunctionLength;                                                            /* Size of this descriptor in bytes */
    uint8_t bDescriptorType;                                                            /* CS_INTERFACE descriptor type */
    uint8_t bDescriptorSybType;                                                         /* Header function descriptor subtype */
    uint8_t bmCapabilities;                                                             /* The capabilities that this configuration supports. (A bit value
                                                                                           of zero means that the request is not supported.)
                                                                                           D7..D4:
                                                                                            RESERVED (Reset to zero)
                                                                                           D3:
                                                                                           1 - Device supports the notification
                                                                                           Network_Connection.
                                                                                           D2:
                                                                                            1 - Device supports the request Send_Break
                                                                                           D1:
                                                                                           1 - Device supports the request combination of
                                                                                           Set_Line_Coding, Set_Control_Line_State,
                                                                                           Get_Line_Coding, and the notification
                                                                                           Serial_State.
                                                                                           D0:
                                                                                           1 - Device supports the request combination of
                                                                                           Set_Comm_Feature, Clear_Comm_Feature, and
                                                                                           Get_Comm_Feature.
                                                                                           The previous bits, in combination, identify which
                                                                                           requests/notifications are supported by a
                                                                                           CommunicationsClass interface with the SubClass code of
                                                                                           Abstract Control Model. */


} CDC_ACM_DESCRIPTOR_TYPE;

#pragma pack(pop)

extern const REX __CDC;

#endif // CDC_H
