/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef USB_H
#define USB_H

#include "sys.h"
#include <stdbool.h>
#include <stdint.h>
#include "lib.h"

//--------------------------------------------------- USB general --------------------------------------------------------------

typedef enum {
    USB_SET_ADDRESS = HAL_IPC(HAL_USB),
    USB_GET_SPEED,
    USB_EP_SET_STALL,
    USB_EP_CLEAR_STALL,
    USB_EP_IS_STALL,
    USB_RESET,
    USB_SUSPEND,
    USB_WAKEUP,
    USB_SETUP,
    USB_SET_TEST_MODE,

    USB_HAL_MAX
}USB_IPCS;

typedef enum {
    USB_EP_CONTROL = 0,
    USB_EP_ISOCHRON,
    USB_EP_BULK,
    USB_EP_INTERRUPT
} USB_EP_TYPE;

typedef enum {
    USB_LOW_SPEED = 0,
    USB_FULL_SPEED,
    USB_HIGH_SPEED,
    USB_SUPER_SPEED
} USB_SPEED;

typedef enum {
    USB_TEST_MODE_NORMAL = 0,
    USB_TEST_MODE_J,
    USB_TEST_MODE_K,
    USB_TEST_MODE_SE0_NAK,
    USB_TEST_MODE_PACKET,
    USB_TEST_MODE_FORCE_ENABLE
} USB_TEST_MODES;

#define USB_HANDLE_DEVICE                                       0xff

#define USB_MAX_EP0_SIZE                                        64

#define BM_REQUEST_DIRECTION                                    0x80
#define BM_REQUEST_DIRECTION_HOST_TO_DEVICE                     0x00
#define BM_REQUEST_DIRECTION_DEVICE_TO_HOST                     0x80

#define BM_REQUEST_TYPE                                         0x60
#define BM_REQUEST_TYPE_STANDART                                0x00
#define BM_REQUEST_TYPE_CLASS                                   0x20
#define BM_REQUEST_TYPE_VENDOR                                  0x40

#define BM_REQUEST_RECIPIENT                                    0x0f
#define BM_REQUEST_RECIPIENT_DEVICE                             0x00
#define BM_REQUEST_RECIPIENT_INTERFACE                          0x01
#define BM_REQUEST_RECIPIENT_ENDPOINT                           0x02

#define USB_REQUEST_GET_STATUS                                  0
#define USB_REQUEST_CLEAR_FEATURE                               1
#define USB_REQUEST_SET_FEATURE                                 3
#define USB_REQUEST_SET_ADDRESS                                 5
#define USB_REQUEST_GET_DESCRIPTOR                              6
#define USB_REQUEST_SET_DESCRIPTOR                              7
#define USB_REQUEST_GET_CONFIGURATION                           8
#define USB_REQUEST_SET_CONFIGURATION                           9
#define USB_REQUEST_GET_INTERFACE                               10
#define USB_REQUEST_SET_INTERFACE                               11
#define USB_REQUEST_SYNCH_FRAME                                 12

#define USB_DEVICE_DESCRIPTOR_INDEX                             1
#define USB_CONFIGURATION_DESCRIPTOR_INDEX                      2
#define USB_STRING_DESCRIPTOR_INDEX                             3
#define USB_INTERFACE_DESCRIPTOR_INDEX                          4
#define USB_ENDPOINT_DESCRIPTOR_INDEX                           5
#define USB_DEVICE_QUALIFIER_DESCRIPTOR_INDEX                   6
#define USB_OTHER_SPEED_CONFIGURATION_DESCRIPTOR_INDEX          7
#define USB_INTERFACE_POWER_DESCRIPTOR_INDEX                    8

#define USB_FUNCTIONAL_DESCRIPTOR                               0x21

#define USB_DEVICE_REMOTE_WAKEUP_FEATURE_INDEX                  1
#define USB_ENDPOINT_HALT_FEATURE_INDEX                         0
#define USB_TEST_MODE_FEATURE                                   2

#define USB_EP_IN                                               0x80
#define USB_EP_NUM(ep)                                          ((ep) & 0x7f)

#pragma pack(push, 1)

typedef struct {
    uint8_t bmRequestType;
    uint8_t bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} SETUP;

#define USB_DEVICE_DESCRIPTOR_SIZE                              18
#define USB_QUALIFIER_DESCRIPTOR_SIZE                           10
#define USB_CONFIGURATION_DESCRIPTOR_SIZE                       9
#define USB_INTERFACE_DESCRIPTOR_SIZE                           9
#define USB_ENDPOINT_DESCRIPTOR_SIZE                            7

typedef struct {
    uint8_t  bLength;                                           //Number Size of this descriptor in bytes
    uint8_t  bDescriptorType;                                   //Constant DEVICE Descriptor Type
    uint8_t  data[256];
} USB_DESCRIPTOR_TYPE, *P_USB_DESCRIPTOR_TYPE;

typedef struct {
    uint8_t  bLength;                                           //Number Size of this descriptor in bytes
    uint8_t  bDescriptorType;                                   //Constant DEVICE Descriptor Type
    uint16_t bcdUSB;                                            // BCD USB Specification Release Number in Binary-Coded Decimal (i.e., 2.10 is 210H).
    uint8_t  bDeviceClass;                                      //Class code (assigned by the USB-IF).
    uint8_t  bDeviceSubClass;                                   //Subclass code (assigned by the USB-IF).
    uint8_t  bDeviceProtocol;                                   //Protocol code (assigned by the USB-IF).
    uint8_t  bMaxPacketSize0;                                   //Maximum packet size for endpoint zero (only 8, 16, 32, or 64 are valid)
    uint16_t idVendor;                                          //Vendor ID (assigned by the USB-IF)
    uint16_t idProduct;                                         //Product ID (assigned by the manufacturer)
    uint16_t bcdDevice;                                         //BCD Device release number in binary-coded
    uint8_t  iManufacturer;                                     //Index of string descriptor describing manufacturer
    uint8_t  iProduct;                                          //Index of string descriptor describing product
    uint8_t  iSerialNumber;                                     //Index of string descriptor describing the device’s serial number
    uint8_t  bNumConfigurations;                                //Number of possible configurations
} USB_DEVICE_DESCRIPTOR_TYPE, *P_USB_DEVICE_DESCRIPTOR_TYPE;

typedef struct {
    uint8_t  bLength;                                           //Size of descriptor
    uint8_t  bDescriptorType;                                   //Device Qualifier Type
    uint16_t bcdUSB;                                            //USB specification version number (e.g., 0200H for V2.00 )
    uint8_t  bDeviceClass;                                      //Class Code
    uint8_t  bDeviceSubClass;                                   //SubClass Code
    uint8_t  bDeviceProtocol;                                   //Protocol Code
    uint8_t  bMaxPacketSize0;                                   //Number Maximum packet size for other speed
    uint8_t  bNumConfigurations;                                //Number Number of Other-speed Configurations
    uint8_t  bReserved;                                         //Reserved for future use, must be zero
} USB_QUALIFIER_DESCRIPTOR_TYPE, *P_USB_QUALIFIER_DESCRIPTOR_TYPE;

typedef struct {
    uint8_t  bLength;                                           //Size of this descriptor in bytes
    uint8_t  bDescriptorType;                                   //CONFIGURATION Descriptor Type
    uint16_t wTotalLength;                                      //Total length of data returned for this configuration
    uint8_t  bNumInterfaces;                                    //Number of interfaces supported by this configuration
    uint8_t  bConfigurationValue;                               //Value to use as an argument to the SetConfiguration() request to select this configuration
    uint8_t  iConfiguration;                                    //Index of string descriptor describing this configuration
    uint8_t  bmAttributes;                                      //Configuration characteristics
    uint8_t  bMaxPower;                                         //Maximum power consumption of the USB    device. Expressed in 2 mA units (i.e., 50 = 100 mA).
} USB_CONFIGURATION_DESCRIPTOR_TYPE, *P_USB_CONFIGURATION_DESCRIPTOR_TYPE, **PP_USB_CONFIGURATION_DESCRIPTOR_TYPE;

typedef struct {
    uint8_t  bLength;                                           //Size of this descriptor in bytes
    uint8_t  bDescriptorType;                                   //INTERFACE Descriptor Type
    uint8_t  bInterfaceNumber;                                  //Number of this interface. Zero-based
    uint8_t  bAlternateSetting;                                 //Value used to select this alternate setting for the interface identified in the prior field
    uint8_t  bNumEndpoints;                                     //Number of endpoints used by this interface (excluding endpoint zero).
    uint8_t  bInterfaceClass;                                   //Class code (assigned by the USB-IF).
    uint8_t  bInterfaceSubClass;                                //Subclass code (assigned by the USB-IF).
    uint8_t  bInterfaceProtocol;                                //Protocol code (assigned by the USB).
    uint8_t  iInterface;                                        //Index of string descriptor describing this    interface
} USB_INTERFACE_DESCRIPTOR_TYPE, *P_USB_INTERFACE_DESCRIPTOR_TYPE;

typedef struct {
    uint8_t  bLength;                                           //Size of this descriptor in bytes
    uint8_t  bDescriptorType;                                   //ENDPOINT Descriptor Type
    uint8_t  bEndpointAddress;                                  //The address of the endpoint on the USB device
    uint8_t  bmAttributes;                                      //This field describes the endpoint’s attributes when it is    configured using the bConfigurationValue.
    uint16_t wMaxPacketSize;                                    //Maximum packet size this endpoint is capable of sending or receiving when this configuration is selected.
    uint8_t  bInterval;                                         //Interval for polling endpoint for data transfers.
} USB_ENDPOINT_DESCRIPTOR_TYPE, *P_USB_ENDPOINT_DESCRIPTOR_TYPE;

typedef struct {
    uint8_t  bLength;                                           //Size of this descriptor in bytes
    uint8_t  bDescriptorType;                                   //STRING Descriptor Type
    uint16_t data[126];
} USB_STRING_DESCRIPTOR_TYPE, *P_USB_STRING_DESCRIPTOR_TYPE, **PP_USB_STRING_DESCRIPTOR_TYPE, ***PPP_USB_STRING_DESCRIPTOR_TYPE;

//--------------------------------------------------- USB device ---------------------------------------------------------------

typedef enum {
    USBD_ALERT = HAL_IPC(HAL_USBD),
    USBD_REGISTER_DESCRIPTOR,                                    /* register USB device descriptor of type USBD_DESCRIPTOR_TYPE */
    USBD_UNREGISTER_DESCRIPTOR,                                  /* unregister USB device descriptor*/
    USBD_REGISTER_HANDLER,                                       /* register USB device state handler for USBD_ALERTS */
    USBD_UNREGISTER_HANDLER,                                     /* unregister USB device state handler */
    USBD_REGISTER_VENDOR,                                        /* register USB device vendor-specific requests handler */
    USBD_UNREGISTER_VENDOR,                                      /* unregister USB device vendor-specific requests handler */
    USBD_INTERFACE_REQUEST,                                      /* request from application to USB device interface */
    USBD_VENDOR_REQUEST,                                         /* IPC callback requset from usb device to vendor handler. */
    USBD_GET_STATE,                                              /* return current USB device state in terms of USBD_ALERTS */

    USBD_MAX
}USBD_IPCS;

typedef enum {
    USBD_ALERT_RESET,
    USBD_ALERT_SUSPEND,
    USBD_ALERT_RESUME,
    USBD_ALERT_CONFIGURED
} USBD_ALERTS;

typedef enum {
    USBD_STATE_DEFAULT = 0,
    USBD_STATE_ADDRESSED,
    USBD_STATE_CONFIGURED
} USBD_STATE;

typedef enum {
    USBD_FEATURE_ENDPOINT_HALT = 0,
    USBD_FEATURE_DEVICE_REMOTE_WAKEUP,
    USBD_FEATURE_TEST_MODE,
    USBD_FEATURE_SELF_POWERED
} USBD_FEATURES;

typedef enum {
    USB_DESCRIPTOR_DEVICE_FS = 0,
    USB_DESCRIPTOR_DEVICE_HS,
    USB_DESCRIPTOR_CONFIGURATION_FS,
    USB_DESCRIPTOR_CONFIGURATION_HS,
    USB_DESCRIPTOR_STRING
} USBD_DESCRIPTOR_TYPE;

#define USBD_FLAG_PERSISTENT_DESCRIPTOR                 (1 << 0)

typedef struct {
    uint8_t flags;
    uint16_t lang, index;
    //data or pointer is following
} USBD_DESCRIPTOR_REGISTER_STRUCT;

#define USBD_DESCRIPTOR_REGISTER_STRUCT_SIZE_ALIGNED    ((sizeof(USBD_DESCRIPTOR_REGISTER_STRUCT) + 3) & ~3)

#define USBD_HANDLE_DEVICE                               0x0
#define USBD_HANDLE_INTERFACE                            0x100

//--------------------------------------------------- CDC class ---------------------------------------------------------------
typedef enum {
    USB_CDC_SEND_BREAK = 0,
    USB_CDC_MAX
} USB_CDC_REQUESTS;


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
#define SEND_BREAK                                                                      0x23

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

//--------------------------------------------------- USB lib -----------------------------------------------------------------

typedef struct {
    bool (*lib_usb_register_persistent_descriptor)(USBD_DESCRIPTOR_TYPE, unsigned int, unsigned int, const void *);
    USB_INTERFACE_DESCRIPTOR_TYPE* (*lib_usb_get_first_interface)(const USB_CONFIGURATION_DESCRIPTOR_TYPE*);
    USB_INTERFACE_DESCRIPTOR_TYPE* (*lib_usb_get_next_interface)(const USB_CONFIGURATION_DESCRIPTOR_TYPE*, const USB_INTERFACE_DESCRIPTOR_TYPE*);
    USB_DESCRIPTOR_TYPE* (*lib_usb_interface_get_first_descriptor)(const USB_CONFIGURATION_DESCRIPTOR_TYPE*, const USB_INTERFACE_DESCRIPTOR_TYPE*, unsigned int);
    USB_DESCRIPTOR_TYPE* (*lib_usb_interface_get_next_descriptor)(const USB_CONFIGURATION_DESCRIPTOR_TYPE*, const USB_DESCRIPTOR_TYPE*, unsigned int);
} LIB_USB;

__STATIC_INLINE bool usb_register_persistent_descriptor(USBD_DESCRIPTOR_TYPE type, unsigned int index, unsigned int lang, const void *descriptor)
{
    LIB_CHECK_RET(LIB_ID_USB);
    return ((const LIB_USB*)__GLOBAL->lib[LIB_ID_USB])->lib_usb_register_persistent_descriptor(type, index, lang, descriptor);
}

__STATIC_INLINE USB_INTERFACE_DESCRIPTOR_TYPE* usb_get_first_interface(const USB_CONFIGURATION_DESCRIPTOR_TYPE* cfg)
{
    LIB_CHECK_RET(LIB_ID_USB);
    return ((const LIB_USB*)__GLOBAL->lib[LIB_ID_USB])->lib_usb_get_first_interface(cfg);
}

__STATIC_INLINE USB_INTERFACE_DESCRIPTOR_TYPE* usb_get_next_interface(const USB_CONFIGURATION_DESCRIPTOR_TYPE* cfg, const USB_INTERFACE_DESCRIPTOR_TYPE* start)
{
    LIB_CHECK_RET(LIB_ID_USB);
    return ((const LIB_USB*)__GLOBAL->lib[LIB_ID_USB])->lib_usb_get_next_interface(cfg, start);
}

__STATIC_INLINE USB_DESCRIPTOR_TYPE* usb_interface_get_first_descriptor(const USB_CONFIGURATION_DESCRIPTOR_TYPE* cfg, const USB_INTERFACE_DESCRIPTOR_TYPE* start, unsigned int type)
{
    LIB_CHECK_RET(LIB_ID_USB);
    return ((const LIB_USB*)__GLOBAL->lib[LIB_ID_USB])->lib_usb_interface_get_first_descriptor(cfg, start, type);
}

__STATIC_INLINE USB_DESCRIPTOR_TYPE* usb_interface_get_next_descriptor(const USB_CONFIGURATION_DESCRIPTOR_TYPE* cfg, const USB_DESCRIPTOR_TYPE* start, unsigned int type)
{
    LIB_CHECK_RET(LIB_ID_USB);
    return ((const LIB_USB*)__GLOBAL->lib[LIB_ID_USB])->lib_usb_interface_get_next_descriptor(cfg, start, type);
}

#pragma pack(pop)

#endif // USB_H
