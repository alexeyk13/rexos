/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef CDC_ACM_H
#define CDC_ACM_H

#include <stdint.h>
#include "ipc.h"
#include "uart.h"

typedef enum {
    USB_CDC_ACM_SET_BAUDRATE = IPC_USER,
    USB_CDC_ACM_GET_BAUDRATE,
    USB_CDC_ACM_BAUDRATE_REQUEST,
    USB_CDC_ACM_SEND_BREAK,
    USB_CDC_ACM_BREAK_REQUEST
} USB_CDC_ACM_REQUESTS;

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
} CDC_HEADER_DESCRIPTOR;

typedef struct {
    uint8_t bFunctionLength;                                                            /* Size of this descriptor in bytes */
    uint8_t bDescriptorType;                                                            /* CS_INTERFACE descriptor type */
    uint8_t bDescriptorSybType;                                                         /* Header function descriptor subtype */
    uint8_t bControlInterface;                                                          /* The interface number of the Communications
                                                                                           or Data Class interface, designated as the
                                                                                           controlling interface for the union.*/
    //uint8_t bSubordinateInterface[] is following

} CDC_UNION_DESCRIPTOR;

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
} CDC_CALL_MANAGEMENT_DESCRIPTOR;

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


} CDC_ACM_DESCRIPTOR;

typedef struct {
    uint32_t dwDTERate;                                                                 /*  Data terminal rate, in bits per second. */
    uint8_t bCharFormat;                                                                /*  Stop bits
                                                                                                0 - 1 Stop bit
                                                                                                1 - 1.5 Stop bits
                                                                                                2 - 2 Stop bits*/
    uint8_t bParityType;                                                                /*  Parity
                                                                                                0 - None
                                                                                                1 - Odd
                                                                                                2 - Even
                                                                                                3 - Mark
                                                                                                4 - Space */

    uint8_t bDataBits;                                                                  /* Data bits (5, 6, 7, 8 or 16). */
} LINE_CODING_STRUCT;

#pragma pack(pop)

#define SEND_ENCAPSULATED_COMMAND                                                       0x00
#define GET_ENCAPSULATED_RESPONSE                                                       0x01
#define SET_COMM_FEATURE                                                                0x02
#define GET_COMM_FEATURE                                                                0x03
#define CLEAR_COMM_FEATURE                                                              0x04

#define SET_AUX_LINE_STATE                                                              0x10
#define SET_HOOK_STATE                                                                  0x11
#define PULSE_SETUP                                                                     0x12
#define SEND_PULSE                                                                      0x13
#define SET_PULSE_TIME                                                                  0x14
#define RING_AUX_JACK                                                                   0x15

#define SET_LINE_CODING                                                                 0x20
#define GET_LINE_CODING                                                                 0x21
#define SET_CONTROL_LINE_STATE                                                          0x22
#define CDC_ACM_SEND_BREAK                                                              0x23

#define SET_RINGER_PARMS                                                                0x30
#define GET_RINGER_PARMS                                                                0x31
#define SET_OPERATION_PARMS                                                             0x32
#define GET_OPERATION_PARMS                                                             0x33
#define SET_LINE_PARMS                                                                  0x34
#define GET_LINE_PARMS                                                                  0x35
#define DIAL_DIGITS                                                                     0x36
#define SET_UNIT_PARAMETER                                                              0x37
#define GET_UNIT_PARAMETER                                                              0x38
#define CLEAR_UNIT_PARAMETER                                                            0x39
#define GET_PROFILE                                                                     0x3a

#define SET_ETHERNET_MULTICAST_FILTERS                                                  0x40
#define SET_ETHERNET_POWER_MANAGEMENT_PATTERN_FILTER                                    0x41
#define GET_ETHERNET_POWER_MANAGEMENT_PATTERN_FILTER                                    0x42
#define SET_ETHERNET_PACKET_FILTER                                                      0x43
#define GET_ETHERNET_STATISTIC                                                          0x44

#define SET_ATM_DATA_FORMAT                                                             0x50
#define GET_ATM_DEVICE_STATISTICS                                                       0x51
#define SET_ATM_DEFAULT_VC                                                              0x52
#define GET_ATM_VC_STATISTICS                                                           0x53

#define GET_NTB_PARAMETERS                                                              0x80
#define GET_NET_ADDRESS                                                                 0x81
#define SET_NET_ADDRESS                                                                 0x82
#define GET_NTB_FORMAT                                                                  0x83
#define SET_NTB_FORMAT                                                                  0x84
#define GET_NTB_INPUT_SIZE                                                              0x85
#define SET_NTB_INPUT_SIZE                                                              0x86
#define GET_MAX_DATAGRAM_SIZE                                                           0x87
#define SET_MAX_DATAGRAM_SIZE                                                           0x88
#define GET_CRC_MODE                                                                    0x89
#define SET_CRC_MODE                                                                    0x8a

//CDC notifications
#define CDC_NETWORK_CONNECTION                                                          0x00
#define CDC_RESPONSE_AVAILABLE                                                          0x01
#define CDC_AUX_JACK_HOOK_STATE                                                         0x08
#define CDC_RING_DETECT                                                                 0x09
#define CDC_SERIAL_STATE                                                                0x20
#define CDC_CALL_STATE_CHANGE                                                           0x28
#define CDC_LINE_STATE_CHANGE                                                           0x23

//serial state notify
#define CDC_SERIAL_STATE_DCD                                                            (1 << 0)
#define CDC_SERIAL_STATE_DSR                                                            (1 << 1)
#define CDC_SERIAL_STATE_BREAK                                                          (1 << 2)
#define CDC_SERIAL_STATE_RINGING                                                        (1 << 3)
#define CDC_SERIAL_STATE_FRAME_ERROR                                                    (1 << 4)
#define CDC_SERIAL_STATE_PARITY_ERROR                                                   (1 << 5)
#define CDC_SERIAL_STATE_OVERRUN                                                        (1 << 6)

#endif // CDC_ACM_H
