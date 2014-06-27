/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef USB_MSC_H
#define USB_MSC_H

#include "dev.h"
#include "scsi.h"
#include "usbd.h"

#define CBW_SIZE				31
#define CSW_SIZE				13
#define CBW_MAGIC				0x43425355
#define MAX_CB_SIZE			0x10

#define CSW_STATUS_OK		0
#define CSW_STATUS_FAILED	1
#define CSW_STATUS_ERROR	2

#define MSC_RESET				0xff
#define MSC_GET_MAX_LUN		0xfe

typedef enum {
	MSC_STATE_CBW = 0,
	MSC_STATE_DATA,
	MSC_STATE_CSW,
	MSC_STATE_CSW_SENT
}MSC_STATE;

#pragma pack(push, 1)

typedef struct {
	uint32_t	dCBWSignature;
	uint32_t dCBWTag;
	uint32_t	dCBWDataTransferLength;
	uint8_t	bmCBWFlags;
	uint8_t	bCBWLUN;
	uint8_t	bCBWCBLength;
	uint8_t	CBWCB[16];
}CBW;

typedef struct {
	uint32_t	dCSWSignature;
	uint32_t dCSWTag;
	uint32_t	dCSWDataResidue;
	uint8_t	bCSWStatus;
}CSW;

#pragma pack(pop)

typedef struct {
	USBD* usbd;
	EP_CLASS ep_num;
	int ep_size, block_size;
	HANDLE event, thread, queue;
	MSC_STATE state;
	SCSI* scsi;
	unsigned int scsi_requested, scsi_transferred;
	bool transfer_completed;
	char* current_buf;
	CBW cbw;
	uint8_t csw_status;
	USBD_SETUP_CB setup_cb;
	USBD_STATE_CB state_cb;
} USB_MSC;

USB_MSC* usb_msc_create(USBD* usbd, EP_CLASS ep_num, STORAGE *storage, SCSI_DESCRIPTOR* scsi_descriptor);
void usb_msc_destroy(USB_MSC* msc);
void usb_msc_reset(USB_MSC* msc);

#endif // USB_MSC_H
