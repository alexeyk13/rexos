/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "usbd_core.h"
#include "kernel_config.h"
#include "usbd.h"
#include "dbg.h"
#include "hw_config.h"
#include <string.h>

#if (USB_DEBUG_SUCCESS_REQUESTS)
const char* const DESCRIPTOR_NAMES[]			= {"DEVICE", "CONFIGURATION", "STRING", "", "", "DEVICE QUALIFIER", "OTHER SPEED CONFIGURATION"};
#endif

void usbd_inform_state_changed(USBD* usbd)
{
	USBD_STATE_CB* cur;
	DLIST_ENUM de;
	dlist_enum_start((DLIST**)&usbd->state_handlers, &de);
	while (dlist_enum(&de, (DLIST**)&cur))
		cur->state_handler(usbd->state, usbd->prior_state, cur->param);
}

static inline bool usbd_device_get_status(USBD* usbd)
{
	usbd->control.buf[0] = 0;
	usbd->control.buf[1] = 0;
	usbd->control.size = 2;
#if (USB_DEBUG_SUCCESS_REQUESTS)
	printf("USB: get device status\n\r");
#endif
	return true;
}

static inline void usbd_set_address(USBD* usbd)
{
	usb_set_address(usbd->idx, usbd->control.setup.wValue);
	switch (usbd->state)
	{
	case USBD_STATE_DEFAULT:
		usbd->prior_state = usbd->state;
		usbd->state = USBD_STATE_ADDRESSED;
		break;
	case USBD_STATE_ADDRESSED:
		if (usbd->control.setup.wValue == 0)
		{
			usbd->prior_state = usbd->state;
			usbd->state = USBD_STATE_DEFAULT;
		}
		break;
	default:
		break;
	}
#if (USB_DEBUG_SUCCESS_REQUESTS)
	printf("USB: set address %d\n\r", usbd->control.setup.wValue);
#endif
}

static inline bool usbd_get_descriptor(USBD* usbd)
{
	bool res = false;
	//can be call in any device state
	int index = usbd->control.setup.wValue & 0xff;
	int i;
	switch (usbd->control.setup.wValue >> 8)
	{
	case USB_DEVICE_DESCRIPTOR_INDEX:
		usbd->control.size = usbd->descriptors->p_device->bLength;
		memcpy(usbd->control.buf, (char*)usbd->descriptors->p_device, usbd->control.size);
		res = true;
		break;
	case USB_CONFIGURATION_DESCRIPTOR_INDEX:
		if (index < usbd->descriptors->num_of_configurations)
		{
			if (usb_get_current_speed(usbd->idx) == USB_HIGH_SPEED)
			{
				usbd->control.size = usbd->descriptors->pp_configurations[index]->wTotalLength;
				memcpy(usbd->control.buf, (char*)usbd->descriptors->pp_configurations[index], usbd->control.size);
				res = true;
			}
			else if (usbd->descriptors->pp_fs_configurations)
			{
				usbd->control.size = usbd->descriptors->pp_fs_configurations[index]->wTotalLength;
				memcpy(usbd->control.buf, (char*)usbd->descriptors->pp_fs_configurations[index], usbd->control.size);
				res = true;
			}
		}
		break;
	case USB_STRING_DESCRIPTOR_INDEX:
		//languages list request
		if (index == 0)
		{
			usbd->control.size = usbd->descriptors->p_string_0->bLength;
			memcpy(usbd->control.buf, (char*)usbd->descriptors->p_string_0, usbd->control.size);
			res = true;
		}
		else if (index <= usbd->descriptors->num_of_strings)
		{
			//try to locate language
			for (i = 0; i < usbd->descriptors->num_of_languages; ++i)
				if (usbd->descriptors->p_string_0->data[i] == usbd->control.setup.wIndex)
				{
					usbd->control.size = usbd->descriptors->ppp_strings[i][index - 1]->bLength;
					memcpy(usbd->control.buf, (char*)usbd->descriptors->ppp_strings[i][index - 1], usbd->control.size);
					res = true;
					break;
				}
		}
		break;
	case USB_DEVICE_QUALIFIER_DESCRIPTOR_INDEX:
		if (usb_get_current_speed(usbd->idx) == USB_HIGH_SPEED && usbd->descriptors->p_usb_qualifier_fs)
		{
			usbd->control.size = usbd->descriptors->p_usb_qualifier_fs->bLength;
			memcpy(usbd->control.buf, (char*)usbd->descriptors->p_usb_qualifier_fs, usbd->control.size);
			res = true;
		}
		else if (usb_get_current_speed(usbd->idx) != USB_HIGH_SPEED && usbd->descriptors->p_usb_qualifier_hs)
		{
			usbd->control.size = usbd->descriptors->p_usb_qualifier_hs->bLength;
			memcpy(usbd->control.buf, (char*)usbd->descriptors->p_usb_qualifier_hs, usbd->control.size);
			res = true;
		}
		break;
	case USB_OTHER_SPEED_CONFIGURATION_DESCRIPTOR_INDEX:
		if (index < usbd->descriptors->num_of_configurations)
		{
			if (usb_get_current_speed(usbd->idx) == USB_HIGH_SPEED)
			{
				if (usbd->descriptors->pp_fs_configurations)
				{
					usbd->control.size = usbd->descriptors->pp_fs_configurations[index]->wTotalLength;
					memcpy(usbd->control.buf, (char*)usbd->descriptors->pp_fs_configurations[index], usbd->control.size);
					res = true;
				}
			}
			else
			{
				usbd->control.size = usbd->descriptors->pp_configurations[index]->wTotalLength;
				memcpy(usbd->control.buf, (char*)usbd->descriptors->pp_configurations[index], usbd->control.size);
				res = true;
			}
		}
		break;
	}

#if (USB_DEBUG_SUCCESS_REQUESTS)
	if (res)
	{
		printf("USB: get %s descriptor ", DESCRIPTOR_NAMES[(usbd->control.setup.wValue >> 8) - 1]);
		switch (usbd->control.setup.wValue >> 8)
		{
		case USB_CONFIGURATION_DESCRIPTOR_INDEX:
		case USB_OTHER_SPEED_CONFIGURATION_DESCRIPTOR_INDEX:
			printf("%d", index + 1);
			break;
		case USB_STRING_DESCRIPTOR_INDEX:
			if (index == 0)
				printf("- lang idx");
			else
				printf("lang: 0x%04x, id: %d", usbd->control.setup.wIndex, index);
			break;
		}
		printf("\n\r");
	}
#endif
	return res;
}

static inline bool usbd_get_configuration(USBD* usbd)
{
	usbd->control.buf[0] = (char)usbd->configuration;
	usbd->control.size = 1;
#if (USB_DEBUG_SUCCESS_REQUESTS)
	printf("USB: get configuration\n\r");
#endif
	return true;
}

static inline bool usbd_set_configuration(USBD* usbd)
{
	bool res = false;
	int i;
	if (usbd->control.setup.wValue <= usbd->descriptors->num_of_configurations)
	{
		//read USB 2.0 specification for more details
		if (usbd->state == USBD_STATE_CONFIGURED)
		{
			for (i = 1; i < usbd_get_active_interface(usbd)->bNumEndpoints; ++i)
			{
				usb_ep_disable(usbd->idx, EP_IN(i));
				usb_ep_disable(usbd->idx, EP_OUT(i));
			}
			usbd->configuration = 0;
			usbd->iface = 0;
			usbd->iface_alt = 0;

			usbd->prior_state = usbd->state;
			usbd->state = USBD_STATE_ADDRESSED;
			usbd_inform_state_changed(usbd);
		}
		if (usbd->state == USBD_STATE_ADDRESSED && usbd->control.setup.wValue)
		{
			usbd->configuration = usbd->control.setup.wValue;
			usbd->iface = 0;
			usbd->iface_alt = 0;
			usbd->prior_state = usbd->state;
			usbd->state = USBD_STATE_CONFIGURED;
			usbd_inform_state_changed(usbd);
		}
#if (USB_DEBUG_SUCCESS_REQUESTS)
		printf("USB: set configuration %d\n\r", usbd->control.setup.wValue);
#endif
		res = true;
	}
	return res;
}

bool usbd_device_request(USBD* usbd)
{
	bool res = false;
	switch (usbd->control.setup.bRequest)
	{
	case USB_REQUEST_GET_STATUS:
		res = usbd_device_get_status(usbd);
		break;
	case USB_REQUEST_CLEAR_FEATURE:
		break;
	case USB_REQUEST_SET_FEATURE:
		break;
	case USB_REQUEST_SET_ADDRESS:
#if !(USB_ADDRESS_POST_PROCESSING)
		usbd_set_address(usbd);
#endif
		res = true;
		break;
	case USB_REQUEST_GET_DESCRIPTOR:
		res = usbd_get_descriptor(usbd);
		break;
	//not implemented for standart request, because descriptors r/o (in flash)
	//case USB_REQUEST_SET_DESCRIPTOR:
	case USB_REQUEST_GET_CONFIGURATION:
		res = usbd_get_configuration(usbd);
		break;
	case USB_REQUEST_SET_CONFIGURATION:
		res = usbd_set_configuration(usbd);
		break;
	}

	return res;
}

static inline bool usbd_interface_get_status(USBD* usbd)
{
	bool res = false;
	if (usbd->state == USBD_STATE_CONFIGURED && usbd_get_active_configuration(usbd)->bNumInterfaces > usbd->control.setup.wIndex)
	{
		usbd->control.buf[0] = 0;
		usbd->control.buf[1] = 0;
		usbd->control.size = 2;
		res = true;
	}

#if (USB_DEBUG_SUCCESS_REQUESTS)
	if (res)
		printf("USB: get interface status %d\n\r", usbd->control.setup.wIndex);
#endif
	return res;
}

static inline bool usbd_interface_set_clear_feature(USBD* usbd)
{
	bool res = false;
	if (usbd->state == USBD_STATE_CONFIGURED && usbd_get_active_configuration(usbd)->bNumInterfaces > usbd->control.setup.wIndex)
	{
		usbd->control.size = 0;
		res = true;
	}

#if (USB_DEBUG_SUCCESS_REQUESTS)
	if (res)
		printf("USB: interface set/clear feature %d\n\r", usbd->control.setup.wIndex);
#endif
	return res;
}

static inline bool usbd_get_interface(USBD* usbd)
{
	bool res = false;
	if (usbd->state == USBD_STATE_CONFIGURED && usbd->control.setup.wIndex == usbd->iface)
	{
		usbd->control.buf[0] = (char)usbd->iface_alt;
		usbd->control.size = 1;
		res = true;
	}

#if (USB_DEBUG_SUCCESS_REQUESTS)
	if (res)
		printf("USB: get interface %d\n\r", usbd->control.setup.wIndex);
#endif
	return res;
}

static inline bool usbd_set_interface(USBD* usbd)
{
	bool res = false;
	if (usbd->state == USBD_STATE_CONFIGURED && configuration_get_interface(usbd_get_active_configuration(usbd), usbd->control.setup.wIndex, usbd->control.setup.wValue) != NULL)
	{
		usbd->control.size = 0;
		res = true;
		usbd->iface = usbd->control.setup.wIndex;
		usbd->iface_alt = usbd->control.setup.wValue;
		usbd_inform_state_changed(usbd);
	}

#if (USB_DEBUG_SUCCESS_REQUESTS)
	if (res)
		printf("USB: set interface %d, alt %d\n\r", usbd->control.setup.wIndex, usbd->control.setup.wValue);
#endif
	return res;
}

bool usbd_interface_request(USBD* usbd)
{
	bool res = false;
	switch (usbd->control.setup.bRequest)
	{
	case USB_REQUEST_GET_STATUS:
		res = usbd_interface_get_status(usbd);
		break;
	//no features. just check correct iface and state
	case USB_REQUEST_CLEAR_FEATURE:
	case USB_REQUEST_SET_FEATURE:
		res = usbd_interface_set_clear_feature(usbd);
		break;
	case USB_REQUEST_GET_INTERFACE:
		res = usbd_get_interface(usbd);
		break;
	case USB_REQUEST_SET_INTERFACE:
		res = usbd_get_interface(usbd);
		break;
	}

	return res;
}

static inline bool usbd_ep_get_status(USBD* usbd)
{
	usbd->control.buf[0] = usb_ep_is_stall(usbd->idx, usbd->control.setup.wIndex) ? 1 : 0;
	usbd->control.buf[1] = 0;
	usbd->control.size = 2;
#if (USB_DEBUG_SUCCESS_REQUESTS)
	printf("USB: get endpoint %02X status\n\r", usbd->control.setup.wIndex);
#endif
	return true;
}

static inline bool usbd_ep_clear_feature(USBD* usbd)
{
	bool res = false;

	//the only feature supported for endpoint
	if (usbd->control.setup.wValue == USB_ENDPOINT_HALT_FEATURE_INDEX)
	{
		usb_ep_clear_stall(usbd->idx, usbd->control.setup.wIndex);
		res = true;
	}
#if (USB_DEBUG_SUCCESS_REQUESTS)
	printf("USB: clear stall %02X status\n\r", usbd->control.setup.wIndex);
#endif
	return res;
}

static inline bool usbd_ep_set_feature(USBD* usbd)
{
	bool res = false;

	//the only feature supported for endpoint
	if (usbd->control.setup.wValue == USB_ENDPOINT_HALT_FEATURE_INDEX)
	{
		usb_ep_stall(usbd->idx, usbd->control.setup.wIndex);
		res = true;
	}
#if (USB_DEBUG_SUCCESS_REQUESTS)
	printf("USB: set stall %02X status\n\r", usbd->control.setup.wIndex);
#endif
	return res;
}

bool usbd_endpoint_request(USBD* usbd)
{
	bool res = false;
	switch (usbd->control.setup.bRequest)
	{
	case USB_REQUEST_GET_STATUS:
		res = usbd_ep_get_status(usbd);
		break;
	case USB_REQUEST_CLEAR_FEATURE:
		res = usbd_ep_clear_feature(usbd);
		break;
	case USB_REQUEST_SET_FEATURE:
		res = usbd_ep_set_feature(usbd);
		break;
	case USB_REQUEST_SYNCH_FRAME:
		break;
	}

	return res;
}

void usbd_request_completed(USBD* usbd)
{
	switch (usbd->control.setup.bRequest)
	{
	//the only operation, required postprocessing is SET_ADDRESS
	case USB_REQUEST_SET_ADDRESS:
#if (USB_ADDRESS_POST_PROCESSING)
		usbd_set_address(usbd);
#endif
		break;
	}
}
