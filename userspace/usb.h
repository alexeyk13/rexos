/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2016, Alexey Kramarenko
    All rights reserved.
*/

#ifndef USB_H
#define USB_H

#include <stdbool.h>
#include <stdint.h>
#include "process.h"
#include "ipc.h"

//--------------------------------------------------- USB general --------------------------------------------------------------

typedef enum {
    USB_SET_ADDRESS = IPC_USER,
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
    USB_SUPER_SPEED,
    USB_LOW_SPEED_ALT,
    USB_FULL_SPEED_ALT,
    USB_HIGH_SPEED_ALT,
    USB_SUPER_SPEED_ALT
} USB_SPEED;

typedef enum {
    USB_TEST_MODE_NORMAL = 0,
    USB_TEST_MODE_J,
    USB_TEST_MODE_K,
    USB_TEST_MODE_SE0_NAK,
    USB_TEST_MODE_PACKET,
    USB_TEST_MODE_FORCE_ENABLE_HS,
    USB_TEST_MODE_FORCE_ENABLE_FS
} USB_TEST_MODES;

typedef enum {
    USB_0 = 0,
    USB_1
} USB_PORT_TYPE;

#define USB_HANDLE_DEVICE                                       0xff
#define USB_HANDLE_HOST                                         0xfe
#define USB_HANDLE_OTG                                          0xfd

#define USB_HANDLE(port, num)                                   (((port) << 8) | num)
#define USB_PORT(handle)                                        ((handle) >> 8)
#define USB_NUM(handle)                                         ((handle) & 0xff)

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

typedef enum {
    USB_DEVICE_DESCRIPTOR_TYPE = 1,
    USB_CONFIGURATION_DESCRIPTOR_TYPE,
    USB_STRING_DESCRIPTOR_TYPE,
    USB_INTERFACE_DESCRIPTOR_TYPE,
    USB_ENDPOINT_DESCRIPTOR_TYPE,
    USB_DEVICE_QUALIFIER_DESCRIPTOR_TYPE,
    USB_OTHER_SPEED_CONFIGURATION_DESCRIPTOR_TYPE,
    USB_INTERFACE_POWER_DESCRIPTOR_TYPE,
    USB_OTG_DESCRIPTOR_TYPE,
    USB_DEBUG_DESCRIPTOR_TYPE,
    USB_INTERFACE_ASSOCIATION_DESCRIPTOR_TYPE
} USB_DESCRIPTOR_TYPE;

#define USB_FUNCTIONAL_DESCRIPTOR                               0x21

//used for composite USB with IAD
#define USB_MISCELLANEOUS_DEVICE_CLASS                          0xef
#define USB_MISCELLANEOUS_COMMON_SUBCLASS                       0x02
#define USB_IAD_PROTOCOL                                        0x01

#define USB_DEVICE_REMOTE_WAKEUP_FEATURE_INDEX                  1
#define USB_ENDPOINT_HALT_FEATURE_INDEX                         0
#define USB_TEST_MODE_FEATURE                                   2

#define USB_EP_IN                                               0x80
#define USB_EP_NUM(ep)                                          ((ep) & 0x7f)

#define USB_EP_BM_ATTRIBUTES_CONTROL                            0x0
#define USB_EP_BM_ATTRIBUTES_ISOCHRONOUS                        0x1
#define USB_EP_BM_ATTRIBUTES_BULK                               0x2
#define USB_EP_BM_ATTRIBUTES_INTERRUPT                          0x3
#define USB_EP_BM_ATTRIBUTES_TYPE_MASK                          0x3

#pragma pack(push, 1)

typedef struct {
    uint8_t bmRequestType;
    uint8_t bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} SETUP;

typedef struct {
    uint8_t  bLength;                                           //Number Size of this descriptor in bytes
    uint8_t  bDescriptorType;                                   //Constant DEVICE Descriptor Type
    uint8_t  data[256];
} USB_GENERIC_DESCRIPTOR;

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
} USB_DEVICE_DESCRIPTOR;

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
} USB_QUALIFIER_DESCRIPTOR;

typedef struct {
    uint8_t  bLength;                                           //Size of this descriptor in bytes
    uint8_t  bDescriptorType;                                   //CONFIGURATION Descriptor Type
    uint16_t wTotalLength;                                      //Total length of data returned for this configuration
    uint8_t  bNumInterfaces;                                    //Number of interfaces supported by this configuration
    uint8_t  bConfigurationValue;                               //Value to use as an argument to the SetConfiguration() request to select this configuration
    uint8_t  iConfiguration;                                    //Index of string descriptor describing this configuration
    uint8_t  bmAttributes;                                      //Configuration characteristics
    uint8_t  bMaxPower;                                         //Maximum power consumption of the USB    device. Expressed in 2 mA units (i.e., 50 = 100 mA).
} USB_CONFIGURATION_DESCRIPTOR;

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
} USB_INTERFACE_DESCRIPTOR;

typedef struct {
    uint8_t  bLength;                                           //Size of this descriptor in bytes
    uint8_t  bDescriptorType;                                   //ENDPOINT Descriptor Type
    uint8_t  bEndpointAddress;                                  //The address of the endpoint on the USB device
    uint8_t  bmAttributes;                                      //This field describes the endpoint’s attributes when it is    configured using the bConfigurationValue.
    uint16_t wMaxPacketSize;                                    //Maximum packet size this endpoint is capable of sending or receiving when this configuration is selected.
    uint8_t  bInterval;                                         //Interval for polling endpoint for data transfers.
} USB_ENDPOINT_DESCRIPTOR;

typedef struct {
    uint8_t  bLength;                                           /*Size of this descriptor in bytes*/
    uint8_t  bDescriptorType;                                   /*STRING Descriptor Type*/
    uint16_t data[126];
} USB_STRING_DESCRIPTOR;

typedef struct {
    uint8_t bLength;                                            /*Size of this descriptor in bytes*/
    uint8_t bDescriptorType;                                    /*INTERFACE ASSOCIATION Descriptor*/
    uint8_t bFirstInterface;                                    /*Interface number of the first interface that is associated with this function*/
    uint8_t bInterfaceCount;                                    /*Number of contiguous interfaces that are associated with this function*/
    uint8_t bFunctionClass;                                     /*Class code (assigned by USB-IF). A value of zero is not allowed in this descriptor*/
                                                                /*If this field is FFH, the function class is vendor-specific*/
                                                                /*All other values are reserved for assignment by the USB-IF*/
    uint8_t bFunctionSubClass;                                  /*Subclass code (assigned by USB-IF). If the bFunctionClass field is not set to FFH all*/
                                                                /*values are reserved for assignment by the USB-IF*/

    uint8_t bFunctionProtocol;                                  /*Protocol code (assigned by USB-IF). These codes are qualified by the values of the*/
                                                                /*bFunctionClass and bFunctionSubClass fields*/
    uint8_t iFunction;                                          /*Index of string descriptor describing this function*/
} USB_INTERFACE_ASSOCIATION_DESCRIPTOR;

USB_INTERFACE_DESCRIPTOR* usb_get_first_interface(const USB_CONFIGURATION_DESCRIPTOR* cfg);
USB_INTERFACE_DESCRIPTOR* usb_get_next_interface(const USB_CONFIGURATION_DESCRIPTOR* cfg, const USB_INTERFACE_DESCRIPTOR* start);
USB_INTERFACE_DESCRIPTOR* usb_find_interface(const USB_CONFIGURATION_DESCRIPTOR* cfg, uint8_t num);
void* usb_interface_get_first_descriptor(const USB_CONFIGURATION_DESCRIPTOR* cfg, const USB_INTERFACE_DESCRIPTOR* start, unsigned int type);
void* usb_interface_get_next_descriptor(const USB_CONFIGURATION_DESCRIPTOR* cfg, const void *start, unsigned int type);

//--------------------------------------------------- USB device ---------------------------------------------------------------

typedef enum {
    USBD_ALERT = IPC_USER,
    USBD_REGISTER_DESCRIPTOR,                                    /* register USB device descriptor of type USBD_DESCRIPTOR_TYPE */
    USBD_UNREGISTER_DESCRIPTOR,                                  /* unregister USB device descriptor*/
    USBD_REGISTER_HANDLER,                                       /* register USB device state handler for USBD_ALERTS */
    USBD_UNREGISTER_HANDLER,                                     /* unregister USB device state handler */
    USBD_VENDOR_REQUEST,                                         /* IPC callback requset from usb device to vendor handler. */
    USBD_GET_STATE,                                              /* return current USB device state in terms of USBD_ALERTS */
    USBD_REGISTER_CONFIGURATION,
    USBD_UNREGISTER_CONFIGURATION,

    USBD_MAX
}USBD_IPCS;

typedef enum {
    USBD_ALERT_RESET = 0,
    USBD_ALERT_SUSPEND,
    USBD_ALERT_RESUME,
    USBD_ALERT_CONFIGURED
} USBD_ALERTS;

typedef enum {
    USBD_FEATURE_ENDPOINT_HALT = 0,
    USBD_FEATURE_DEVICE_REMOTE_WAKEUP,
    USBD_FEATURE_TEST_MODE,
    USBD_FEATURE_SELF_POWERED
} USBD_FEATURES;

typedef struct {
    uint16_t lang, index;
} USBD_DESCRIPTOR_REGISTER_STRUCT;

#pragma pack(pop)

#define USBD_IFACE(iface_num, item)                     (((iface_num) << 16) | (item & 0xffff))
#define USBD_IFACE_NUM(iface)                           ((iface) >> 16)
#define USBD_IFACE_ITEM(iface)                          ((iface) & 0xffff)

bool usbd_register_descriptor(HANDLE usbd, const void* d, unsigned int index, unsigned int lang);
bool usbd_register_const_descriptor(HANDLE usbd, const void* d, unsigned int index, unsigned int lang);
bool usbd_register_ascii_string(HANDLE usbd, unsigned int index, unsigned int lang, const char* str);
HANDLE usbd_create(USB_PORT_TYPE port, unsigned int process_size, unsigned int priority);
bool usbd_register_configuration(HANDLE usbd, uint16_t cfg, uint16_t iface, void* d, unsigned int size);
bool usbd_unregister_configuration(HANDLE usbd, uint16_t cfg, uint16_t iface);

#endif // USB_H
