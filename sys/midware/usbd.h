/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef USBD_H
#define USBD_H

#include "../../userspace/process.h"

typedef struct {
    unsigned int total_size;                                             //now and follow - header is included
    unsigned int qualifier_descriptor_offset;                            //0 if not present
    unsigned int configuration_descriptors_offset;
    unsigned int other_speed_configuration_descriptors_offset;           //0 if not present
    //data is following
} USB_DESCRIPTORS_HEADER;

typedef struct {
    unsigned int lang_id;
    unsigned int index;
    unsigned int offset;
} USB_STRING_DESCRIPTOR_OFFSET;

typedef struct {
    unsigned int total_size;
    unsigned int count;
    //offsets follow
    //data follow
} USB_STRING_DESCRIPTORS_HEADER;

typedef enum {
    USBD_ALERT_RESET,
    USBD_ALERT_SUSPEND,
    USBD_ALERT_WAKEUP,
    USBD_ALERT_CONFIGURATION_SET,
    USBD_ALERT_FEATURE_SET,
    USBD_ALERT_FEATURE_CLEARED,
    USBD_ALERT_INTERFACE_SET
} USBD_ALERTS;

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

extern const REX __USBD;

#endif // USBD_H
