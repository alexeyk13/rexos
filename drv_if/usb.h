/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef USB_H
#define USB_H

#include "types.h"
#include "dev.h"
#include "usb_desc.h"

typedef enum {
	EP_OUT0 = 0x0,
	EP_OUT1,
	EP_OUT2,
	EP_OUT3,
	EP_OUT4,
	EP_OUT5,
	EP_OUT6,
	EP_OUT7,
	EP_OUT8,
	EP_OUT9,
	EP_OUT10,
	EP_OUT11,
	EP_OUT12,
	EP_OUT13,
	EP_OUT14,
	EP_OUT15,
	EP_IN0 = 0x80,
	EP_IN1,
	EP_IN2,
	EP_IN3,
	EP_IN4,
	EP_IN5,
	EP_IN6,
	EP_IN7,
	EP_IN8,
	EP_IN9,
	EP_IN10,
	EP_IN11,
	EP_IN12,
	EP_IN13,
	EP_IN14,
	EP_IN15
} EP_CLASS;

#define IS_EP_IN(ep)											((ep) & 0x80)
#define IS_EP_OUT(ep)										(((ep) & 0x80) == 0)
#define EP_NUM(ep)											((ep) & 0x7f)
#define EP_IN(ep)												((ep) | 0x80)
#define EP_OUT(ep)											((ep) & 0x7f)

typedef enum {
	EP_TYPE_CONTROL = 0,
	EP_TYPE_ISOCHRON,
	EP_TYPE_BULK,
	EP_TYPE_INTERRUPT
} EP_TYPE_CLASS;

typedef enum {
	USB_LOW_SPEED = 0,
	USB_FULL_SPEED,
	USB_HIGH_SPEED,
	USB_SUPER_SPEED
} USB_SPEED;

typedef void (*USB_HANDLER)(void*);
typedef void (*USB_EP_HANDLER)(EP_CLASS, void*);

typedef struct {
	USB_HANDLER usbd_on_reset;
	USB_HANDLER usbd_on_suspend;
	USB_HANDLER usbd_on_resume;
	USB_HANDLER usbd_on_setup;
}USBD_CALLBACKS;

typedef struct {
	const USBD_CALLBACKS* cb;
	void* param;
	const USB_DESCRIPTORS_TYPE* descriptors;
}USB_ENABLE_DEVICE_PARAMS;

//requests to hw driver
void usb_enable_device(USB_CLASS idx, USB_ENABLE_DEVICE_PARAMS* params, int priority);
void usb_disable(USB_CLASS idx);
void usb_set_address(USB_CLASS idx, uint8_t address);
USB_SPEED usb_get_current_speed(USB_CLASS idx);

void usb_ep_enable(USB_CLASS idx, EP_CLASS ep, USB_EP_HANDLER cb, void* param, int max_packet_size, EP_TYPE_CLASS type);
void usb_ep_disable(USB_CLASS idx, EP_CLASS ep);
void usb_ep_stall(USB_CLASS idx, EP_CLASS ep);
void usb_ep_clear_stall(USB_CLASS idx, EP_CLASS ep);
bool usb_ep_is_stall(USB_CLASS idx, EP_CLASS ep);

void usb_read(USB_CLASS idx, EP_CLASS ep, char* buf, int size);
void usb_write(USB_CLASS idx, EP_CLASS ep, char* buf, int size);

#endif // USB_DD_H
