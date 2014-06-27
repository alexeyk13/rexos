/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "usbd.h"
#include "usbd_core.h"
#include "usbd_core_io.h"
#include "kernel_config.h"
#include "dbg.h"
#include "mem_private.h"
#include "hw_config.h"
#include "error.h"

void usbd_on_reset(void *param);
void usbd_on_suspend(void *param);
void usbd_on_resume(void *param);

const  USBD_CALLBACKS _usbd_callbacks = {
	usbd_on_reset,
	usbd_on_suspend,
	usbd_on_resume,
	usbd_on_setup,
};

USBD* usbd_create(USB_CLASS idx, P_USB_DESCRIPTORS_TYPE descriptors, int priority)
{
	USBD* usbd = sys_alloc(sizeof(USBD));
	if (usbd)
	{
		usbd->control.buf = sys_alloc_aligned(USBD_CONTROL_BUF_SIZE, USB_DATA_ALIGN);
		if (usbd->control.buf)
		{
			usbd->idx = idx;
			usbd->descriptors = descriptors;
			usbd->state = USBD_STATE_POWERED;
			usbd->prior_state = USBD_STATE_POWERED;
			usbd->setup_state = USB_SETUP_STATE_REQUEST;

			USB_ENABLE_DEVICE_PARAMS params;
			params.cb = &_usbd_callbacks;
			params.param = usbd;
			params.descriptors = descriptors;
			usb_enable_device(idx, &params, priority);
			dlist_clear((DLIST**)&usbd->state_handlers);
			dlist_clear((DLIST**)&usbd->class_handlers);
			dlist_clear((DLIST**)&usbd->vendor_handlers);
		}
		else
		{
			sys_free(usbd);
			error_dev(ERROR_MEM_OUT_OF_SYSTEM_MEMORY, DEV_USB, idx);
		}
	}
	else
		error_dev(ERROR_MEM_OUT_OF_SYSTEM_MEMORY, DEV_USB, idx);
	return usbd;
}

void usbd_destroy(USBD* usbd)
{
	usb_disable(usbd->idx);
	sys_free(usbd->control.buf);
	sys_free(usbd);
}

USB_CLASS usbd_get_usb(USBD* usbd)
{
	return usbd->idx;
}

P_USB_CONFIGURATION_DESCRIPTOR_TYPE usbd_get_active_configuration(USBD* usbd)
{
	ASSERT(usbd->configuration);
	if (usb_get_current_speed(usbd->idx) == USB_HIGH_SPEED)
		return descriptors_get_configuration(usbd->descriptors, usbd->configuration);
	else
		return descriptors_get_other_speed_configuration(usbd->descriptors, usbd->configuration);
}

P_USB_INTERFACE_DESCRIPTOR_TYPE usbd_get_active_interface(USBD* usbd)
{
	return configuration_get_interface(usbd_get_active_configuration(usbd), usbd->iface, usbd->iface_alt);
}

void usbd_on_reset(void* param)
{
	USBD* usbd = (USBD*)param;
	usbd->configuration = 0;
	usbd->iface = 0;
	usbd->iface_alt = 0;
	usbd->prior_state = usbd->state;
	usbd->state = USBD_STATE_DEFAULT;
	usbd->setup_state = USB_SETUP_STATE_REQUEST;

	//enable EP0
	usbd->ep0_size = usbd->descriptors->p_device->bMaxPacketSize0;
	usb_ep_enable(usbd->idx, EP_IN0, usbd_on_in_writed, usbd, usbd->ep0_size, EP_TYPE_CONTROL);
	usb_ep_enable(usbd->idx, EP_OUT0, usbd_on_out_readed, usbd, usbd->ep0_size, EP_TYPE_CONTROL);
	usb_read(usbd->idx, EP_OUT0, usbd->control.buf, usbd->ep0_size);

	usbd_inform_state_changed(usbd);

#if (USB_DEBUG_SUCCESS_REQUESTS)
	printf("USB%d reset\n\r", usbd->idx + 1);
#endif
}

void usbd_on_suspend(void* param)
{
	USBD* usbd = (USBD*)param;
	if (usbd->prior_state != USBD_STATE_SUSPENDED)
	{
		usbd->prior_state = usbd->state;
		usbd->state = USBD_STATE_SUSPENDED;

		usbd_inform_state_changed(usbd);

#if (USB_DEBUG_SUCCESS_REQUESTS)
		printf("USB%d suspend\n\r", usbd->idx + 1);
#endif
	}
}

void usbd_on_resume(void* param)
{
	USBD* usbd = (USBD*)param;
	usbd->state = usbd->prior_state;

	usbd_inform_state_changed(usbd);

#if (USB_DEBUG_SUCCESS_REQUESTS)
	printf("USB%d resumed\n\r", usbd->idx + 1);
#endif
}

void usbd_register_state_callback(USBD* usbd, USBD_STATE_CB* state_handler)
{
	dlist_add_tail((DLIST**)&usbd->state_handlers, (DLIST*)state_handler);
}

void usbd_register_class_callback(USBD *usbd, USBD_SETUP_CB* class_handler)
{
	dlist_add_tail((DLIST**)&usbd->class_handlers, (DLIST*)class_handler);
}

void usbd_register_vendor_callback(USBD *usbd, USBD_SETUP_CB* vendor_handler)
{
	dlist_add_tail((DLIST**)&usbd->vendor_handlers, (DLIST*)vendor_handler);
}

void usbd_unregister_state_callback(USBD* usbd, USBD_STATE_CB* state_handler)
{
	dlist_remove((DLIST**)&usbd->state_handlers, (DLIST*)state_handler);
}

void usbd_unregister_class_callback(USBD* usbd, USBD_SETUP_CB* class_handler)
{
	dlist_remove((DLIST**)&usbd->class_handlers, (DLIST*)class_handler);
}

void usbd_unregister_vendor_callback(USBD* usbd, USBD_SETUP_CB* vendor_handler)
{
	dlist_remove((DLIST**)&usbd->vendor_handlers, (DLIST*)vendor_handler);
}
