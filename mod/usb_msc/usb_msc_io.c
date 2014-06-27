/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "usb_msc_io.h"
#include "usb_msc.h"
#include "dbg.h"
#include "kernel_config.h"
#include "scsi.h"
#include "string.h"
#include "event.h"
#include "queue.h"
#include "../userspace/core/core.h"

static inline void msc_send_csw(USB_MSC* msc)
{
	ASSERT(!queue_is_full(msc->queue));
	msc->current_buf = queue_allocate_buffer_ms(msc->queue, INFINITE);
	CSW* csw = (CSW*)msc->current_buf;
	csw->bCSWStatus = msc->csw_status;
	csw->dCSWSignature = CBW_MAGIC;
	csw->dCSWTag = msc->cbw.dCBWTag;
	csw->dCSWDataResidue = msc->scsi_transferred;
	usb_write(usbd_get_usb(msc->usbd), EP_IN(msc->ep_num), msc->current_buf, CSW_SIZE);

#if (USB_MSC_DEBUG_FLOW)
	printf("USB_MSC: TX CSW\n\r");
#endif

#if (USB_DEBUG_ERRORS)
	if (csw->bCSWStatus == CSW_STATUS_ERROR)
		printf("MSC phase error\n\r");
#endif
}

void on_msc_received(EP_CLASS ep, void* param)
{
	USB_MSC* msc = (USB_MSC*)param;
	unsigned int block_size;
	switch (msc->state)
	{
	case MSC_STATE_CBW:
		memcpy(&msc->cbw, msc->current_buf, CBW_SIZE);
		queue_release_buffer(msc->queue, msc->current_buf);
		msc->current_buf = NULL;
		msc->scsi_requested = 0;
		msc->scsi_transferred = 0;
		event_set(msc->event);
		break;
	case MSC_STATE_DATA:
		queue_push(msc->queue, msc->current_buf);
		msc->current_buf = NULL;
		if (msc->scsi_transferred < msc->scsi_requested)
		{
			ASSERT(!queue_is_full(msc->queue));
			msc->current_buf = queue_allocate_buffer_ms(msc->queue, INFINITE);
			block_size = msc->scsi_requested - msc->scsi_transferred;
			if (block_size > msc->block_size)
				block_size = msc->block_size;
			msc->scsi_transferred += block_size;
			usb_read(usbd_get_usb(msc->usbd), EP_OUT(msc->ep_num), msc->current_buf, block_size);
		}
		else
			event_set(msc->event);
		break;
	case MSC_STATE_CSW:
		msc->state = MSC_STATE_CSW_SENT;
		msc_send_csw(msc);
		break;
	default:
		break;
	}
}

void on_msc_sent(EP_CLASS ep, void *param)
{
	USB_MSC* msc = (USB_MSC*)param;
	unsigned int block_size;
	switch (msc->state)
	{
	case MSC_STATE_DATA:
		queue_release_buffer(msc->queue, msc->current_buf);
		msc->current_buf = NULL;
		if (msc->scsi_transferred < msc->scsi_requested)
		{
			ASSERT(!queue_is_empty(msc->queue));
			msc->current_buf = queue_pull_ms(msc->queue, INFINITE);
			block_size = msc->scsi_requested - msc->scsi_transferred;
			if (block_size > msc->block_size)
				block_size = msc->block_size;
			msc->scsi_transferred += block_size;
			usb_write(usbd_get_usb(msc->usbd), EP_IN(msc->ep_num), msc->current_buf, block_size);
		}
		else
			event_set(msc->event);
		break;
	case MSC_STATE_CSW:
		msc->state = MSC_STATE_CSW_SENT;
		msc_send_csw(msc);
		break;
	case MSC_STATE_CSW_SENT:
		msc->state = MSC_STATE_CBW;
		//same buffer will be used for next cbw
		usb_read(usbd_get_usb(msc->usbd), EP_OUT(msc->ep_num), msc->current_buf, CBW_SIZE);
		break;
	default:
		break;
	}
}

void on_storage_buffer_filled(void* param, unsigned int size)
{
	USB_MSC* msc = (USB_MSC*)param;
	unsigned int block_size;
	CRITICAL_ENTER;
	msc->scsi_requested += size;
	CRITICAL_LEAVE;
	if (event_is_set(msc->event))
	{
		event_clear(msc->event);
		msc->current_buf = queue_pull_ms(msc->queue, INFINITE);
		block_size = msc->scsi_requested - msc->scsi_transferred;
		if (block_size > msc->block_size)
			block_size = msc->block_size;
		msc->scsi_transferred += block_size;

		usb_write(usbd_get_usb(msc->usbd), EP_IN(msc->ep_num), msc->current_buf, block_size);
	}
#if (USB_MSC_DEBUG_FLOW)
	printf("USB_MSC: TX %d\n\r", size);
#endif
}

void on_storage_request_buffers(void* param, unsigned int size)
{
	USB_MSC* msc = (USB_MSC*)param;
	unsigned int block_size;
	CRITICAL_ENTER;
	msc->scsi_requested += size;
	CRITICAL_LEAVE;
	if (event_is_set(msc->event))
	{
		event_clear(msc->event);
		msc->current_buf = queue_allocate_buffer_ms(msc->queue, INFINITE);
		block_size = msc->scsi_requested - msc->scsi_transferred;
		if (block_size > msc->block_size)
			block_size = msc->block_size;
		msc->scsi_transferred += block_size;
		usb_read(usbd_get_usb(msc->usbd), EP_OUT(msc->ep_num), msc->current_buf, block_size);
	}
#if (USB_MSC_DEBUG_FLOW)
	printf("USB_MSC: RX %d\n\r", size);
#endif
}

void usb_msc_thread(void* param)
{
	USB_MSC* msc = (USB_MSC*)param;
	for (;;)
	{
		event_wait_ms(msc->event, INFINITE);
		msc->state = MSC_STATE_DATA;
		//process received CBW
		if (msc->cbw.dCBWSignature == CBW_MAGIC && msc->cbw.bCBWCBLength <= MAX_CB_SIZE)
		{
			msc->csw_status = CSW_STATUS_OK;
#if (USB_MSC_DEBUG_FLOW)
			printf("USB_MSC: dCBWTag: %08x\n\r", msc->cbw.dCBWTag);
			printf("USB_MSC: dCBWDataTransferLength: %08x\n\r", msc->cbw.dCBWDataTransferLength);
			printf("USB_MSC: dCBWDataFlags: %02x\n\r", msc->cbw.bmCBWFlags);
			printf("USB_MSC: dCBWLUN: %02x\n\r", msc->cbw.bCBWLUN);
			printf("USB_MSC: dCBWCB:");
			int i;
			for (i = 0; i < msc->cbw.bCBWCBLength; ++i)
				printf(" %02x", msc->cbw.CBWCB[i]);
			printf(" (%d)\n\r", msc->cbw.bCBWCBLength);
#endif
			if (!scsi_cmd(msc->scsi, (char*)msc->cbw.CBWCB, msc->cbw.bCBWCBLength))
				msc->csw_status = CSW_STATUS_FAILED;
			//wait for transfer completed in any case
			event_wait_ms(msc->event, INFINITE);

		}
		//CBW invalid, phase ERROR
		else
		{
#if (USB_DEBUG_ERRORS)
			printf("Invalid CBW\n\r");
#endif
			msc->csw_status = CSW_STATUS_ERROR;
		}
		event_clear(msc->event);
		msc->state = MSC_STATE_CSW;
		//need zlp?
		if ((msc->scsi_transferred % msc->ep_size) == 0 && msc->cbw.dCBWDataTransferLength != 0 &&
			 msc->scsi_transferred < msc->cbw.dCBWDataTransferLength)
		{
			if (msc->cbw.bmCBWFlags & 0x80)
				usb_write(usbd_get_usb(msc->usbd), EP_IN(msc->ep_num), NULL, 0);
			else
				usb_read(usbd_get_usb(msc->usbd), EP_OUT(msc->ep_num), NULL, 0);
	#if (USB_MSC_DEBUG_FLOW)
		if (msc->cbw.bmCBWFlags & 0x80)
			printf("USB_MSC: TX ZLP\n\r");
		else
			printf("USB_MSC: RX ZLP\n\r");
	#endif
		}
		//send csw directly
		else
		{
			if (msc->cbw.bmCBWFlags & 0x80)
				on_msc_sent(EP_IN(msc->ep_num), msc);
			else
				on_msc_received(EP_OUT(msc->ep_num), msc);
		}
	}
}
