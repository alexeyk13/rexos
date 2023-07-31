/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: Alexey E. Kramarenko (alexeyk13@yandex.ru)
*/

#ifndef USB_DESC_H
#define USB_DESC_H

#include "../../rexos/userspace/usb.h"
#include "../../rexos/userspace/cdc_acm.h"

#pragma pack(push, 1)

const USB_DEVICE_DESCRIPTOR __DEVICE_DESCRIPTOR = {
    sizeof(USB_DEVICE_DESCRIPTOR),                                            /*bLength */
    USB_DEVICE_DESCRIPTOR_TYPE,                                               /*bDescriptorType*/
    0x0200,                                                                   /*bcdUSB */
    USB_MISCELLANEOUS_DEVICE_CLASS,                                           /*bDeviceClass*/
    USB_MISCELLANEOUS_COMMON_SUBCLASS,                                        /*bDeviceSubClass*/
    USB_IAD_PROTOCOL,                                                         /*bDeviceProtocol*/
    64,                                                                       /*bMaxPacketSize*/
    0x0525,                                                                   /*idVendor*/
    0xa4aa,                                                                   /*idProduct*/
    0x0100,                                                                   /*bcdDevice release*/
    1,                                                                        /*Index of manufacturer  string*/
    2,                                                                        /*Index of product string*/
    3,                                                                        /*Index of serial number string*/
    1                                                                         /*bNumConfigurations*/
};

typedef struct {
    USB_CONFIGURATION_DESCRIPTOR configuration;
    USB_INTERFACE_ASSOCIATION_DESCRIPTOR comm_iad;
    USB_INTERFACE_DESCRIPTOR comm_interface;
    CDC_HEADER_DESCRIPTOR cdc_header;
    CDC_ACM_DESCRIPTOR cdc_acm;
    CDC_UNION_DESCRIPTOR cdc_union;
    uint8_t bSubordinateInterface0;
    CDC_CALL_MANAGEMENT_DESCRIPTOR call_management;
    USB_ENDPOINT_DESCRIPTOR comm_endpoint;
    USB_INTERFACE_DESCRIPTOR data_interface;
    USB_ENDPOINT_DESCRIPTOR data_endpoints[2];
} CONFIGURATION;

const CONFIGURATION __CONFIGURATION_DESCRIPTOR = {
    //CONFIGURATION descriptor
    {
        sizeof(USB_CONFIGURATION_DESCRIPTOR),                               /*bLength*/
        USB_CONFIGURATION_DESCRIPTOR_TYPE,                                  /*bDescriptorType*/
        sizeof(CONFIGURATION),                                              /*wTotalLength*/
        2,                                                                  /*bNumInterfaces*/
        1,                                                                  /*bConfigurationValue*/
        4,                                                                  /*iConfiguration*/
        0x80,                                                               /*bmAttributes*/
        250                                                                 /*bMaxPower*/
    },
    //COMM IAD
    {
        sizeof(USB_INTERFACE_ASSOCIATION_DESCRIPTOR),                       /*bLength*/
        USB_INTERFACE_ASSOCIATION_DESCRIPTOR_TYPE,                          /*bDescriptorType*/
        0,                                                                  /*bFirstInterface*/
        2,                                                                  /*bInterfaceCount*/
        CDC_COMM_INTERFACE_CLASS,                                           /*bFunctionClass*/
        CDC_ACM,                                                            /*bFunctionSubClass*/
        CDC_CP_V250,                                                        /*bFunctionProtocol*/
        0                                                                   /*iFunction*/
    },
    //comm INTERFACE descriptor
    {
        sizeof(USB_INTERFACE_DESCRIPTOR),                                   /*bLength*/
        USB_INTERFACE_DESCRIPTOR_TYPE,                                      /*bDescriptorType*/
        0,                                                                  /*bInterfaceNumber*/
        0,                                                                  /*bAlternateSetting*/
        1,                                                                  /*bNumEndpoints*/
        CDC_COMM_INTERFACE_CLASS,                                           /*bInterfaceClass*/
        CDC_ACM,                                                            /*bInterfaceSubClass*/
        CDC_CP_V250,                                                        /*bInterfaceProtocol*/
        4                                                                   /*iInterface*/
    },
    //CDC header descriptor
    {
        sizeof(CDC_HEADER_DESCRIPTOR),                                      /* bFunctionLength */
        CS_INTERFACE,                                                       /* bDescriptorType */
        CDC_DESCRIPTOR_HEADER,                                              /* bDescriptorSybType */
        0x120                                                               /* bcdCDC */
    },
    //CDC ACM descriptor
    {
        sizeof(CDC_ACM_DESCRIPTOR),                                         /* bFunctionLength */
        CS_INTERFACE,                                                       /* bDescriptorType */
        CDC_DESCRIPTOR_ACM,                                                 /* bDescriptorSybType */
        0x2                                                                 /* bmCapabilities */
    },
    //CDC union descriptor
    {
        sizeof(CDC_UNION_DESCRIPTOR) + 1,                                   /* bFunctionLength */
        CS_INTERFACE,                                                       /* bDescriptorType */
        CDC_DESCRIPTOR_UNION,                                               /* bDescriptorSybType */
        0,                                                                  /* bControlInterface */
    },
    1,                                                                      /* bSubordinateInterface 0 */
    //call management
    {
        sizeof(CDC_CALL_MANAGEMENT_DESCRIPTOR),                             /* bFunctionLength */
        CS_INTERFACE,                                                       /* bDescriptorType */
        CDC_DESCRIPTOR_CALL_MANAGEMENT,                                     /* bDescriptorSybType */
        3,                                                                  /* bmCapabilities */
        1                                                                   /* bDataInterface */
    },
    //comm endpoint descriptor
    {
        sizeof(USB_ENDPOINT_DESCRIPTOR),                                    /*bLength*/
        USB_ENDPOINT_DESCRIPTOR_TYPE,                                       /*bDescriptorType*/
        0x80 | 0x2,                                                         /*bEndpointAddress*/
        0x03,                                                               /*bmAttributes*/
        16,                                                                 /*wMaxPacketSize*/
        5                                                                   /*bInterval*/
    },
    //data INTERFACE descriptor
    {
        sizeof(USB_INTERFACE_DESCRIPTOR),                                   /*bLength*/
        USB_INTERFACE_DESCRIPTOR_TYPE,                                      /*bDescriptorType*/
        1,                                                                  /*bInterfaceNumber*/
        0,                                                                  /*bAlternateSetting*/
        2,                                                                  /*bNumEndpoints*/
        CDC_DATA_INTERFACE_CLASS,                                           /*bInterfaceClass*/
        0x00,                                                               /*bInterfaceSubClass*/
        CDC_DP_GENERIC,                                                     /*bInterfaceProtocol*/
        0                                                                   /*iInterface*/
    },
    //endpoints descriptors
    {
        {
            sizeof(USB_ENDPOINT_DESCRIPTOR),                                /*bLength*/
            USB_ENDPOINT_DESCRIPTOR_TYPE,                                   /*bDescriptorType*/
            0x80 | 0x1,                                                     /*bEndpointAddress*/
            0x02,                                                           /*bmAttributes*/
            64,                                                             /*wMaxPacketSize*/
            0                                                               /*bInterval*/
        },
        {
            sizeof(USB_ENDPOINT_DESCRIPTOR),                                /*bLength*/
            USB_ENDPOINT_DESCRIPTOR_TYPE,                                   /*bDescriptorType*/
            0x00 | 0x1,                                                     /*bEndpointAddress*/
            0x02,                                                           /*bmAttributes*/
            64,                                                             /*wMaxPacketSize*/
            0                                                               /*bInterval*/
        }
    }
};

const char __STRING_WLANGS[] = {
    1 * 2 + 2,                                                              /*bLength*/
    USB_STRING_DESCRIPTOR_TYPE,                                             /*bDescriptorType*/
    0x09, 0x04                                                              // 0x409 - English
};

const char __STRING_MANUFACTURER[] = {
    6 * 2 + 2,                                                              /*bLength*/
    USB_STRING_DESCRIPTOR_TYPE,                                             /*bDescriptorType*/
    'R', 0,
    'E', 0,
    'x', 0,
    ' ', 0,
    'O', 0,
    'S', 0
};

const char __STRING_PRODUCT[] = {
    7 * 2 + 2,                                                              /*bLength*/
    USB_STRING_DESCRIPTOR_TYPE,                                             /*bDescriptorType*/
    'U', 0,
    'S', 0,
    'B', 0,
    ' ', 0,
    'C', 0,
    'D', 0,
    'C', 0
};

const char __STRING_SERIAL[] = {
    12 * 2 + 2,                                                             /*bLength*/
    USB_STRING_DESCRIPTOR_TYPE,                                             /*bDescriptorType*/
    '0', 0,
    '1', 0,
    '2', 0,
    '3', 0,
    '4', 0,
    '5', 0,
    '6', 0,
    '7', 0,
    '8', 0,
    '9', 0,
    'A', 0,
    'B', 0
};

const char __STRING_DEFAULT[] = {
    7 * 2 + 2,                                                              /*bLength*/
    USB_STRING_DESCRIPTOR_TYPE,                                             /*bDescriptorType*/
    'D', 0,
    'e', 0,
    'f', 0,
    'a', 0,
    'u', 0,
    'l', 0,
    't', 0
};

#pragma pack(pop)

#endif // USB_DESC_H
