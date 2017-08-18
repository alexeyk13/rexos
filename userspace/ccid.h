/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef CCID_H
#define CCID_H

#include <stdint.h>
#include "usb.h"

#pragma pack(push, 1)

#define CCID_INTERFACE_CLASS                            0x0b

#define CCID_CMD_ABORT                                  0x01
#define CCID_CMD_GET_CLOCK_FREQUENCIES                  0x02
#define CCID_CMD_GET_DATA_RATES                         0x03

typedef struct {
    uint8_t bLength;                                                                    /* Size of this descriptor in bytes */
    uint8_t bDescriptorType;                                                            /* functional descriptor type */
    uint16_t bcdCCID;                                                                   /* Integrated Circuit(s) Cards Interface Devices (CCID)
                                                                                           Specification Release Number in Binary-Coded decimal (i.e.,
                                                                                           2.10 is 0210h) */
    uint8_t bMaxSlotIndex;                                                              /* The index of the highest available slot on this device. All
                                                                                           slots are consecutive starting at 00h.
                                                                                           i.e. 0Fh = 16 slots on this device numbered 00h to 0Fh */
    uint8_t bVoltageSupport;                                                            /* This value indicates what voltages the CCID can supply to its slots.
                                                                                           It is a bitwise OR operation performed on the following values:
                                                                                           - 01h 5.0V
                                                                                           - 02h 3.0V
                                                                                           - 04h 1.8V
                                                                                            Other bits are RFU */
    uint32_t dwProtocols;                                                               /* Upper Word- is RFU = 0000h
                                                                                           Lower Word- Encodes the supported protocol types.
                                                                                           A '1' in a given bit position indicates support for the
                                                                                           associated ISO protocol.
                                                                                           0001h = Protocol T=0
                                                                                           0002h = Protocol T=1
                                                                                           All other bits are reserved and must be set to zero. The field
                                                                                           is intended to correspond to the PCSC specification
                                                                                           definitions. See PCSC Part3. Table 3-1 Tag 0x0120.
                                                                                           Example: 00000003h indicates support for T = 0 and T = 1 */
    uint32_t dwDefaultClock;                                                            /* Default ICC clock frequency in KHz. This is an integer value.
                                                                                           Example: 3.58 MHz is encoded as the integer value 3580.
                                                                                           (00000DFCh)
                                                                                           This is used in ETU and waiting time calculations. It is the
                                                                                           clock frequency used when reading the ATR data */
    uint32_t dwMaximumClock;                                                            /* Maximum supported ICC clock frequency in KHz. This is an
                                                                                           integer value.
                                                                                           Example: 14.32 MHz is encoded as the integer value 14320.
                                                                                           (000037F0h) */
    uint8_t bNumClockSupported;                                                         /* The number of clock frequencies that are supported by
                                                                                           the CCID. If the value is 00h, the supported clock
                                                                                           frequencies are assumed to be the default clock
                                                                                           frequency defined by dwDefaultClock and the maximum
                                                                                           clock frequency defined by dwMaximumClock.
                                                                                           The reader must implement the command
                                                                                           PC_to_RDR_SetDataRateAndClockFrequency if more
                                                                                           than one clock frequency is supported */
    uint32_t dwDataRate;                                                                /* Default ICC I/O data rate in bps. This is an integer value
                                                                                           Example: 9600 bps is encoded as the integer value
                                                                                           9600. (00002580h) */
    uint32_t dwMaxDataRate;                                                             /* Maximum supported ICC I/O data rate in bps
                                                                                           Example: 115.2Kbps is encoded as the integer value
                                                                                           115200. (0001C200h) */
    uint8_t bNumDataRatesSupported;                                                     /* The number of data rates that are supported by the CCID.
                                                                                           If the value is 00h, all data rates between the default data
                                                                                           rate dwDataRate and the maximum data rate
                                                                                           dwMaxDataRate are supported */
    uint32_t dwMaxIFSD;                                                                 /* Indicates the maximum IFSD supported by CCID for protocol T=1 */
    uint32_t dwSynchProtocol;                                                           /* Upper Word- is RFU = 0000h
                                                                                           Lower Word- encodes the supported protocotypes.
                                                                                           A '1' in a given bit position indicates supporfor the associated protocol.
                                                                                           - 0001h indicates support for the 2-wire protocol 1
                                                                                           - 0002h indicates support for the 3-wire protocol 1
                                                                                           - 0004h indicates support for the I2C protocol 1
                                                                                           All other values are outside of this specification, and
                                                                                           must be handled by vendor-supplied drivers */
    uint32_t dwMechanical;                                                              /* The value is a bitwise OR operation performed on the
                                                                                           following values:
                                                                                           - 00000000h No special characteristics
                                                                                           - 00000001h Card accept mechanism
                                                                                           - 00000002h Card ejection mechanism
                                                                                           - 00000004h Card capture mechanism
                                                                                           - 00000008h Card lock/unlock mechanism */
    uint32_t dwFeatures;                                                                /* This value indicates what intelligent features the CCID has */
    uint32_t dwMaxCCIDMessageLength;                                                    /* For extended APDU level the value shall be between 261 + 10
                                                                                           (header) and 65544 +10, otherwise the minimum value is the
                                                                                           wMaxPacketSize of the Bulk-OUT endpoint */
    uint8_t bClassGetResponse;                                                          /* Significant only for CCID that offers an APDU level for
                                                                                           exchanges.
                                                                                           Indicates the default class value used by the CCID when it
                                                                                           sends a Get Response command to perform the transportation
                                                                                           of an APDU by T=0 protocol.
                                                                                           Value FFh indicates that the CCID echoes the class of the
                                                                                           APDU  */
    uint8_t bClassEnvelope;                                                             /* Significant only for CCID that offers an extended APDU level
                                                                                           for exchanges.
                                                                                           Indicates the default class value used by the CCID when it
                                                                                           sends an Envelope command to perform the transportation of
                                                                                           an extended APDU by T=0 protocol.
                                                                                           Value FFh indicates that the CCID echoes the class of the
                                                                                           APDU */
    uint16_t wLcdLayout;                                                                /* Number of lines and characters for the LCD display used to
                                                                                           send messages for PIN entry.
                                                                                           XX: number of lines
                                                                                           YY: number of characters per line.
                                                                                           XXYY=0000h no LCD */
    uint8_t bPINSupport;                                                                /* This value indicates what PIN support features the CCID has.
                                                                                           The value is a bitwise OR operation performed on the following
                                                                                           values:
                                                                                           01h PIN Verification supported
                                                                                           02h PIN Modification supported */
    uint8_t bMaxCCIDBusySlots;                                                          /* Maximum number5 of slots which can be simultaneously busy */
} CCID_DESCRIPTOR;

#define CCID_VOLTAGE_5_0V                               0x1
#define CCID_VOLTAGE_3_0V                               0x2
#define CCID_VOLTAGE_1_8V                               0x4

#define CCID_PROTOCOL_T0                                0x0001
#define CCID_PROTOCOL_T1                                0x0002

#define CCID_HW_PROTOCOL_2WIRE                          0x0001
#define CCID_HW_PROTOCOL_3WIRE                          0x0002
#define CCID_HW_PROTOCOL_I2C                            0x0004

#define CCID_MECHANICAL_NO                              0x00000000
#define CCID_MECHANICAL_ACCEPT                          0x00000001
#define CCID_MECHANICAL_EJECT                           0x00000002
#define CCID_MECHANICAL_CAPTURE                         0x00000004
#define CCID_MECHANICAL_LOCK                            0x00000008

#define CCID_FEATURE_NO                                 0x00000000
#define CCID_FEATURE_AUTO_CONFIG                        0x00000002
#define CCID_FEATURE_AUTO_ACTIVATE                      0x00000004
#define CCID_FEATURE_AUTO_VOLTAGE                       0x00000008
#define CCID_FEATURE_AUTO_CLOCK                         0x00000010
#define CCID_FEATURE_AUTO_BAUD                          0x00000020
#define CCID_FEATURE_AUTO_PPS                           0x00000040
#define CCID_FEATURE_AUTO_PPS_ACTIVE                    0x00000080
#define CCID_FEATURE_AUTO_IFCD                          0x00000400
#define CCID_FEATURE_CAN_STOP                           0x00000100

#define CCID_FEATURE_TPDU                               0x00010000
#define CCID_FEATURE_APDU                               0x00020000
#define CCID_FEATURE_EXT_APDU                           0x00040000

#define CCID_PIN_NO_SUPPORTED                           0x00
#define CCID_PIN_VERIFY                                 0x01
#define CCID_PIN_MODIFY                                 0x02

typedef struct {
    uint8_t bMessageType;
    uint32_t dwLength;
    uint8_t bSlot;
    uint8_t bSeq;
    uint8_t msg_specific[3];
} CCID_MESSAGE;

typedef struct {
    uint8_t bMessageType;
    uint8_t bmSlotICCState;
} CCID_NOTIFY_SLOT_CHANGE;

typedef struct {
    uint8_t bMessageType;
    uint32_t dwLength;
    uint8_t bSlot;
    uint8_t bSeq;
    uint8_t bStatus;
    uint8_t bError;
    uint8_t bClockStatus;
} CCID_SLOT_STATUS;

typedef struct {
    uint8_t bMessageType;
    uint32_t dwLength;
    uint8_t bSlot;
    uint8_t bSeq;
    uint8_t bStatus;
    uint8_t bError;
    uint8_t bChainParameter;
} CCID_DATA_BLOCK;

typedef struct {
    uint8_t bMessageType;
    uint32_t dwLength;
    uint8_t bSlot;
    uint8_t bSeq;
    uint8_t bStatus;
    uint8_t bError;
    uint8_t bProtocolNum;
} CCID_PARAMS;

//CCID bulk out
#define PC_TO_RDR_ICC_POWER_ON                          0x62
#define PC_TO_RDR_ICC_POWER_OFF                         0x63
#define PC_TO_RDR_GET_SLOT_STATUS                       0x65
#define PC_TO_RDR_XFER_BLOCK                            0x6f
#define PC_TO_RDR_GET_PARAMETERS                        0x6c
#define PC_TO_RDR_RESET_PARAMETERS                      0x6d
#define PC_TO_RDR_SET_PARAMETERS                        0x61
#define PC_TO_RDR_ESCAPE                                0x6b
#define PC_TO_RDR_ICC_CLOCK                             0x6e
#define PC_TO_RDR_T0APDU                                0x6a
#define PC_TO_RDR_SECURE                                0x69
#define PC_TO_RDR_MECHANICAL                            0x71
#define PC_TO_RDR_ABORT                                 0x72
#define PC_TO_RDR_SET_DATA_RATE_AND_CLOCK               0x73

//CCID bulk in
#define RDR_TO_PC_DATA_BLOCK                            0x80
#define RDR_TO_PC_SLOT_STATUS                           0x81
#define RDR_TO_PC_PARAMETERS                            0x82
#define RDR_TO_PC_ESCAPE                                0x83
#define RDR_TO_PC_RATE_AND_CLOCK                        0x84

//CCID interrupt in
#define RDR_TO_PC_NOTIFY_SLOT_CHANGE                    0x50
#define RDR_TO_PC_HARDWARE_ERROR                        0x51

typedef enum {
    //requests from host
    USB_CCID_APDU = IPC_USER,
    USB_CCID_POWER_ON,
    USB_CCID_POWER_OFF,
    USB_CCID_GET_PARAMS,
    USB_CCID_SET_PARAMS,
    USB_CCID_RESET_PARAMS,
    //announce from card
    USB_CCID_CARD_INSERTED,
    USB_CCID_CARD_REMOVED
} USB_CCID_IPC;

#define CCID_SLOT_ERROR_CMD_ABORTED                     0xff
#define CCID_SLOT_ERROR_ICC_MUTE                        0xfe
#define CCID_SLOT_ERROR_XFR_PARITY_ERROR                0xfd
#define CCID_SLOT_ERROR_XFR_OVERRUN                     0xfc
#define CCID_SLOT_ERROR_HW_ERROR                        0xfb
#define CCID_SLOT_ERROR_BAD_ATR_TS                      0xf8
#define CCID_SLOT_ERROR_BAD_ATR_TCK                     0xf7
#define CCID_SLOT_ERROR_ICC_PROTOCOL_NOT_SUPPORT        0xf6
#define CCID_SLOT_ERROR_ICC_CLASS_NOT_SUPPORT           0xf5
#define CCID_SLOT_ERROR_PROCEDURE_BYTE_CONFLICT         0xf4
#define CCID_SLOT_ERROR_DEACTIVATED_PROTOCOL            0xf3
#define CCID_SLOT_ERROR_BUSY_WITH_AUTO_SEQUENCE         0xf2
#define CCID_SLOT_ERROR_PIN_TIMEOUT                     0xf0
#define CCID_SLOT_ERROR_PIN_CANCELLED                   0xef
#define CCID_SLOT_ERROR_SLOT_BUSY                       0xe0
#define CCID_SLOT_ERROR_CMD_NOT_SUPPORTED               0x00

#define CCID_SLOT_ICC_STATE_CHANGED                     (1 << 1)

#define CCID_SLOT_STATUS_ICC_PRESENT_AND_ACTIVE         (0)
#define CCID_SLOT_STATUS_ICC_PRESENT_AND_INACTIVE       (1)
#define CCID_SLOT_STATUS_ICC_NOT_PRESENT                (2)

#define CCID_SLOT_STATUS_COMMAND_NO_ERROR               (0)
#define CCID_SLOT_STATUS_COMMAND_FAIL                   (1)
#define CCID_SLOT_STATUS_COMMAND_TIME_EXTENSION         (2)

#define CCID_CLOCK_STATUS_RUNNING                       0
#define CCID_CLOCK_STATUS_STOPPED_H                     1
#define CCID_CLOCK_STATUS_STOPPED_L                     2
#define CCID_CLOCK_STATUS_STOPPED_UNKNOWN               3

#define CCID_EXT_APDU_BEGIN_END                         0x00
#define CCID_EXT_APDU_BEGIN                             0x01
#define CCID_EXT_APDU_END                               0x02
#define CCID_EXT_APDU_MORE                              0x03
#define CCID_EXT_APDU_NULL                              0x10

typedef struct {
    uint8_t bmFindexDindex;                             /* B7-4 - FI - Index into the table 7 in ISO/IEC  7816-3:1997 selecting a clock rate conversion factor
                                                           B3-0 - DI - Index into the table 8 in ISO/IEC  7816-3:1997 selecting a baud rate conversion factor */
    uint8_t bmTCCKST0;                                  /* For T=0 ,B0 - 0b, B7-2 - 000000b
                                                           B1 - Convention used (b1=0 for direct, b1=1 for inverse) */
    uint8_t bGuardTimeT0;                               /* Extra Guardtime between two characters. Add 0 to 254 etu to the normal guardtime of 12etu.
                                                           FFh is the same as 00h */
    uint8_t bWaitingIntegerT0;                          /* WI for T=0 used to define WWT */
    uint8_t bClockStop;                                 /* ICC Clock Stop Support
                                                            00 = Stopping the Clock is not allowed
                                                            01 = Stop with Clock signal Low
                                                            02 = Stop with Clock signal High
                                                            03 = Stop with Clock either High or Low */
} CCID_T0_PARAMS;

typedef struct {
    uint8_t bmFindexDindex;                             /* B7-4 - FI - Index into the table 7 in ISO/IEC  7816-3:1997 selecting a clock rate conversion factor
                                                           B3-0 - DI - Index into the table 8 in ISO/IEC  7816-3:1997 selecting a baud rate conversion factor */
    uint8_t bmTCCKST1;                                  /* For T=1, B7-2 - 000100b
                                                           B0 - Checksum type (b0=0 for LRC, b0=1 for CRC
                                                           B1 - Convention used (b1=0 for direct, b1=1 for inverse) */
    uint8_t bGuardTimeT1;                               /* Extra Guardtime (0 to 254 etu between two characters). If value is FFh, then guardtime is reduced by 1 */
    uint8_t bmWaitingIntegersT1;                        /* B7-4 = BWI
                                                           B3-0 = CWI */
    uint8_t bClockStop;                                 /* ICC Clock Stop Support
                                                            00 = Stopping the Clock is not allowed
                                                            01 = Stop with Clock signal Low
                                                            02 = Stop with Clock signal High
                                                            03 = Stop with Clock either High or Low */
    uint8_t bIFSC;                                      /*  Size of negotiated IFSC */
    uint8_t bNadValue;                                  /* Nad value used by CCID */
} CCID_T1_PARAMS;

typedef enum {
    CCID_T_0 = 0,
    CCID_T_1,
    CCID_T_MAX
} CCID_PROTOCOL;

#pragma pack(pop)

#endif // CCID_H

