/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "usb_desc.h"
#include "usbd.h"
#include "usbd_core.h"
#include "kernel_config.h"
#include "dbg.h"
#include "string.h"
#include "event.h"

#if (USB_DEBUG_ERRORS)

const char* const states[] = {"SETUP", "DATA IN", "DATA IN ZLP", "DATA OUT", "STATUS IN", "STATUS OUT"};

void dbg_invalid_state(USB_SETUP_STATE expected, USB_SETUP_STATE got)
{
	printf("Invalid SETUP state: expected %s, got %s\n\r", states[expected], states[got]);
}
#endif

void usbd_on_setup(void* param)
{
	USBD* usbd = (USBD*)param;
	memcpy(&usbd->control.setup, usbd->control.buf, sizeof(SETUP));
	usbd->control.size = 0;
	//we always must be ready for b2b setup packet
	usb_read(usbd->idx, EP_OUT0, usbd->control.buf, usbd->ep0_size);
#if (USB_DEBUG_FLOW)
	printf("USB SETUP\r\n");
	printf("bmRequestType: %X\n\r", usbd->control.setup.bmRequestType);
	printf("bRequest: %X\n\r", usbd->control.setup.bRequest);
	printf("wValue: %X\n\r", usbd->control.setup.wValue);
	printf("wIndex: %X\n\r", usbd->control.setup.wIndex);
	printf("wLength: %X\n\r", usbd->control.setup.wLength);
#endif
	bool res = false;
	DLIST_ENUM de;
	USBD_SETUP_CB* cur;

	//Back2Back setup received
	if (usbd->setup_state != USB_SETUP_STATE_REQUEST)
		usbd->setup_state = USB_SETUP_STATE_REQUEST;

	switch (usbd->control.setup.bmRequestType & BM_REQUEST_TYPE)
	{
	case BM_REQUEST_TYPE_STANDART:
		switch (usbd->control.setup.bmRequestType & BM_REQUEST_RECIPIENT)
		{
		case BM_REQUEST_RECIPIENT_DEVICE:
			res = usbd_device_request(usbd);
			break;
		case BM_REQUEST_RECIPIENT_INTERFACE:
			res = usbd_interface_request(usbd);
			break;
		case BM_REQUEST_RECIPIENT_ENDPOINT:
			res = usbd_endpoint_request(usbd);
			break;
		}
		break;
	case BM_REQUEST_TYPE_CLASS:
		dlist_enum_start((DLIST**)&usbd->class_handlers, &de);
		while (dlist_enum(&de, (DLIST**)&cur))
			if (cur->setup_handler((C_USBD_CONTROL*)&usbd->control, cur->param))
			{
				res = true;
				break;
			}
		break;
	case BM_REQUEST_TYPE_VENDOR:
		dlist_enum_start((DLIST**)&usbd->vendor_handlers, &de);
		while (dlist_enum(&de, (DLIST**)&cur))
			if (cur->setup_handler((C_USBD_CONTROL*)&usbd->control, cur->param))
			{
				res = true;
				break;
			}
		break;
	}

	if (usbd->control.size > usbd->control.setup.wLength)
		usbd->control.size = usbd->control.setup.wLength;
	//success. start transfers
	if (res)
	{
		if ((usbd->control.setup.bmRequestType & BM_REQUEST_DIRECTION) == BM_REQUEST_DIRECTION_HOST_TO_DEVICE)
		{
			if (usbd->control.size)
			{
				usbd->setup_state = USB_SETUP_STATE_DATA_OUT;
				usb_read(usbd->idx, EP_OUT0, usbd->control.buf, usbd->control.size);
			}
			//data stage is optional
			else
			{
				usbd->setup_state = USB_SETUP_STATE_STATUS_IN;
				usb_write(usbd->idx, EP_IN0, NULL, 0);
			}
		}
		else
		{
			//response less, than required and multiples of EP0SIZE - we need to send ZLP on end of transfers
			if (usbd->control.size < usbd->control.setup.wLength && (usbd->control.size % usbd->ep0_size) == 0)
			{
				if (usbd->control.size)
				{
					usbd->setup_state = USB_SETUP_STATE_DATA_IN_ZLP;
					usb_write(usbd->idx, EP_IN0, usbd->control.buf, usbd->control.size);
				}
				//if no data at all, but request success, we will send ZLP right now
				else
				{
					usbd->setup_state = USB_SETUP_STATE_DATA_IN;
					usb_write(usbd->idx, EP_IN0, NULL, 0);
				}
			}
			else if (usbd->control.size)
			{
				usbd->setup_state = USB_SETUP_STATE_DATA_IN;
				usb_write(usbd->idx, EP_IN0, usbd->control.buf, usbd->control.size);
			}
			//data stage is optional
			else
			{
				usbd->setup_state = USB_SETUP_STATE_STATUS_OUT;
				usb_read(usbd->idx, EP_OUT0, NULL, 0);
			}
		}
	}
	else
	{
		if ((usbd->control.setup.bmRequestType & BM_REQUEST_RECIPIENT) == BM_REQUEST_RECIPIENT_ENDPOINT)
			usb_ep_stall(usbd->idx, usbd->control.setup.wIndex);
		else
		{
			usb_ep_stall(usbd->idx, EP_IN0);
			usb_ep_stall(usbd->idx, EP_OUT0);
		}
#if (USB_DEBUG_ERRORS)
		printf("Unhandled USB%d SETUP:\n\r", usbd->idx + 1);
		printf("bmRequestType: %X\n\r", usbd->control.setup.bmRequestType);
		printf("bRequest: %X\n\r", usbd->control.setup.bRequest);
		printf("wValue: %X\n\r", usbd->control.setup.wValue);
		printf("wIndex: %X\n\r", usbd->control.setup.wIndex);
		printf("wLength: %X\n\r", usbd->control.setup.wLength);
#endif
	}
}

void usbd_on_in_writed(EP_CLASS ep, void* param)
{
	USBD* usbd = (USBD*)param;
#if (USB_DEBUG_FLOW)
	printf("USB EP0 IN TX\r\n");
#endif
	switch (usbd->setup_state)
	{
	case USB_SETUP_STATE_DATA_IN_ZLP:
		//TX ZLP and switch to normal state
		usbd->setup_state = USB_SETUP_STATE_DATA_IN;
		usb_write(usbd->idx, EP_IN0, NULL, 0);
		break;
	case USB_SETUP_STATE_DATA_IN:
		usbd->setup_state = USB_SETUP_STATE_STATUS_OUT;
		usb_read(usbd->idx, EP_OUT0, NULL, 0);
		break;
	case USB_SETUP_STATE_STATUS_IN:
		usbd->setup_state = USB_SETUP_STATE_REQUEST;
		usb_read(USB_1, EP_OUT0, usbd->control.buf, usbd->ep0_size);
		usbd_request_completed(usbd);
		break;
	default:
		usbd->setup_state = USB_SETUP_STATE_REQUEST;
		usb_read(USB_1, EP_OUT0, usbd->control.buf, usbd->ep0_size);
#if (USB_DEBUG_ERRORS)
		dbg_invalid_state(USB_SETUP_STATE_DATA_IN, usbd->setup_state);
#endif
		break;
	}
}

void usbd_on_out_readed(EP_CLASS ep, void* param)
{
	USBD* usbd = (USBD*)param;
#if (USB_DEBUG_FLOW)
	printf("USB EP0 OUT RX\n\r");
#endif
	switch (usbd->setup_state)
	{
	case USB_SETUP_STATE_DATA_OUT:
		usbd->setup_state = USB_SETUP_STATE_STATUS_IN;
		usb_write(usbd->idx, EP_IN0, NULL, 0);
		break;
	case USB_SETUP_STATE_STATUS_OUT:
		usbd->setup_state = USB_SETUP_STATE_REQUEST;
		usb_read(USB_1, EP_OUT0, usbd->control.buf, usbd->ep0_size);
		usbd_request_completed(usbd);
		break;
	default:
		usbd->setup_state = USB_SETUP_STATE_REQUEST;
		usb_read(USB_1, EP_OUT0, usbd->control.buf, usbd->ep0_size);
#if (USB_DEBUG_ERRORS)
		dbg_invalid_state(USB_SETUP_STATE_DATA_OUT, usbd->setup_state);
#endif
		break;
	}
}
