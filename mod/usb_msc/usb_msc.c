/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "usb_msc.h"
#include "usb_msc_io.h"
#include "dbg.h"
#include "kernel_config.h"
#include "mem_private.h"
#include "hw_config.h"
#include "event.h"
#include "thread.h"
#include "queue.h"
#include "error.h"

#define MSC_CONTROL_SIZE	CBW_SIZE

static const STORAGE_HOST_IO_CB _storage_cb = {
	on_storage_buffer_filled,
	on_storage_request_buffers
};

bool usb_request_msc_reset(C_USBD_CONTROL* control, USB_MSC* msc)
{
	usb_msc_reset(msc);
#if (USB_DEBUG_SUCCESS_REQUESTS)
	printf("MSC reset\n\r");
#endif
	control->size = 0;
	return true;
}

bool usb_request_msc_get_max_lun(C_USBD_CONTROL* control, USB_MSC* msc)
{
	control->buf[0] = 0;
	control->size = 1;
#if (USB_DEBUG_SUCCESS_REQUESTS)
	printf("MSC get max LUN (0) \n\r");
#endif
	return true;
}

bool usb_msc_setup_request(C_USBD_CONTROL* control, void* param)
{
	bool res = false;
	if ((control->setup.bmRequestType & BM_REQUEST_TYPE) == BM_REQUEST_TYPE_CLASS &&
		 (control->setup.bmRequestType & BM_REQUEST_RECIPIENT) == BM_REQUEST_RECIPIENT_INTERFACE)
	{
		USB_MSC* msc = (USB_MSC*)param;
		switch (control->setup.bRequest)
		{
		case MSC_RESET:
			res = usb_request_msc_reset(control, msc);
			break;
		case MSC_GET_MAX_LUN:
			res = usb_request_msc_get_max_lun(control, msc);
			break;
		default:
			break;
		}
	}
	return res;
}

void usb_msc_enable(USB_MSC* msc)
{
	usb_msc_reset(msc);
	msc->ep_size = interface_get_endpoint(usbd_get_active_interface(msc->usbd), EP_OUT(msc->ep_num))->wMaxPacketSize;
	usb_ep_enable(usbd_get_usb(msc->usbd), EP_IN(msc->ep_num), on_msc_sent, (void*)msc, msc->ep_size, EP_TYPE_BULK);
	usb_ep_enable(usbd_get_usb(msc->usbd), EP_OUT(msc->ep_num), on_msc_received, (void*)msc, msc->ep_size, EP_TYPE_BULK);

	msc->current_buf = queue_allocate_buffer_ms(msc->queue, INFINITE);
	usb_read(usbd_get_usb(msc->usbd), EP_OUT(msc->ep_num), msc->current_buf, CBW_SIZE);
}

void usb_msc_disable(USB_MSC* msc)
{
	usb_ep_disable(usbd_get_usb(msc->usbd), EP_IN(msc->ep_num));
	usb_ep_disable(usbd_get_usb(msc->usbd), EP_OUT(msc->ep_num));
	usb_msc_reset(msc);
}

void usb_msc_reset(USB_MSC* msc)
{
	if (msc->state != MSC_STATE_CBW)
	{
		scsi_reset(msc->scsi);
		msc->state = MSC_STATE_CBW;
		if (msc->current_buf)
		{
			queue_release_buffer(msc->queue, msc->current_buf);
			msc->current_buf = NULL;
		}
		while (!queue_is_empty(msc->queue))
			queue_release_buffer(msc->queue, queue_pull_ms(msc->queue, INFINITE));
	}
}

void usb_msc_state_changed(USBD_STATE state, USBD_STATE prior_state, void* param)
{
	USB_MSC* msc = (USB_MSC*)param;
	if (state == USBD_STATE_CONFIGURED)
		usb_msc_enable(msc);
	else if (prior_state == USBD_STATE_CONFIGURED)
		usb_msc_disable(msc);
}

USB_MSC* usb_msc_create(USBD *usbd, EP_CLASS ep_num, STORAGE* storage, SCSI_DESCRIPTOR *scsi_descriptor)
{
	USB_MSC* msc = (USB_MSC*)sys_alloc(sizeof(USB_MSC));
	if (msc)
	{
		msc->block_size = storage_get_device_descriptor(storage)->sector_size * USB_MSC_SECTORS_IN_BLOCK;
		msc->queue = queue_create_aligned(msc->block_size, USB_MSC_BUFFERS_IN_QUEUE, USB_DATA_ALIGN);

		STORAGE_QUEUE_PARAMS params;
		params.queue = msc->queue;
		params.block_size = msc->block_size;
		params.host_io_cb = (P_STORAGE_HOST_IO_CB)&_storage_cb;
		params.host_io_param = msc;
		storage_allocate_queue(storage, &params);

		msc->scsi = scsi_create(storage, scsi_descriptor);
		msc->event = event_create();
		msc->ep_num = ep_num;
		msc->usbd = usbd;
		msc->thread = thread_create_and_run("USB MSC", USB_MSC_THREAD_STACK_SIZE, USB_MSC_THREAD_PRIORITY, usb_msc_thread, msc);

		msc->setup_cb.param = msc;
		msc->setup_cb.setup_handler = usb_msc_setup_request;
		usbd_register_class_callback(msc->usbd, &msc->setup_cb);

		msc->state_cb.state_handler = usb_msc_state_changed;
		msc->state_cb.param = msc;
		usbd_register_state_callback(msc->usbd, &msc->state_cb);

		msc->ep_size = 0;
		msc->current_buf = NULL;
	}
	else
		error_dev(ERROR_MEM_OUT_OF_SYSTEM_MEMORY, DEV_USB, usbd_get_usb(msc->usbd));
	return msc;
}

void usb_msc_destroy(USB_MSC* msc)
{
	thread_destroy(msc->thread);
	event_destroy(msc->event);
	scsi_destroy(msc->scsi);
	queue_destroy(msc->queue);
	sys_free(msc);
}
