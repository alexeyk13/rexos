/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef USBD_H
#define USBD_H

#include "usb.h"
#include "dev.h"
#include "types.h"
#include "usb_desc.h"
#include "dlist.h"
#include "kernel_config.h"

typedef enum {
	USBD_STATE_SUSPENDED = 0,
	USBD_STATE_POWERED,
	USBD_STATE_DEFAULT,
	USBD_STATE_ADDRESSED,
	USBD_STATE_CONFIGURED
} USBD_STATE;

typedef enum {
	USB_SETUP_STATE_REQUEST = 0,
	USB_SETUP_STATE_DATA_IN,
	//in case response is less, than request
	USB_SETUP_STATE_DATA_IN_ZLP,
	USB_SETUP_STATE_DATA_OUT,
	USB_SETUP_STATE_STATUS_IN,
	USB_SETUP_STATE_STATUS_OUT
} USB_SETUP_STATE;

typedef struct {
	SETUP setup;
	char* buf;
	int size;
}USBD_CONTROL;

//safe constant implementation of USBD_CONTROL
typedef struct {
	const SETUP setup;
	char *const buf;
	int size;
}C_USBD_CONTROL;

typedef struct {
	DLIST dlist;
	bool (*setup_handler)(C_USBD_CONTROL*, void*);
	void* param;
}USBD_SETUP_CB;

typedef struct {
	DLIST dlist;
	void (*state_handler)(USBD_STATE, USBD_STATE, void*);
	void* param;
}USBD_STATE_CB;

typedef struct {
	USB_CLASS idx;
	P_USB_DESCRIPTORS_TYPE descriptors;
	USBD_STATE state, prior_state;
	uint8_t configuration;
	uint8_t iface, iface_alt;
	//control-endpoint related
	int ep0_size;
	USB_SETUP_STATE setup_state;
	USBD_CONTROL control;
	//handlers
	USBD_STATE_CB* state_handlers;
	USBD_SETUP_CB* class_handlers;
	USBD_SETUP_CB* vendor_handlers;
}USBD;

USBD* usbd_create(USB_CLASS idx, P_USB_DESCRIPTORS_TYPE descriptors, int priority);
void usbd_destroy(USBD* usbd);

P_USB_CONFIGURATION_DESCRIPTOR_TYPE usbd_get_active_configuration(USBD* usbd);
P_USB_INTERFACE_DESCRIPTOR_TYPE usbd_get_active_interface(USBD* usbd);
USB_CLASS usbd_get_usb(USBD* usbd);

void usbd_register_state_callback(USBD* usbd, USBD_STATE_CB* state_handler);
void usbd_register_class_callback(USBD* usbd, USBD_SETUP_CB* class_handler);
void usbd_register_vendor_callback(USBD* usbd, USBD_SETUP_CB* vendor_request);
void usbd_unregister_state_callback(USBD* usbd, USBD_STATE_CB* state_handler);
void usbd_unregister_class_callback(USBD* usbd, USBD_SETUP_CB* class_handler);
void usbd_unregister_vendor_callback(USBD* usbd, USBD_SETUP_CB* vendor_handler);

#endif // USBD_H
