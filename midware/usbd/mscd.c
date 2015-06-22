/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "mscd.h"
#include "../../userspace/sys.h"
#include "../../userspace/stdio.h"
#include "../../userspace/io.h"
#include "../../userspace/uart.h"
#include "../../userspace/stdlib.h"
#include "../../userspace/msc.h"
#include "../../userspace/scsi.h"
#include "../scsis.h"
#include <stdint.h>
#include "usb.h"
#include "sys_config.h"

typedef enum  {
    MSCD_STATE_CBW = 0,
    MSCD_STATE_DATA_IN,
    MSCD_STATE_DATA_IN_ZLP,
    MSCD_STATE_STORAGE,
    MSCD_STATE_DATA_OUT,
    MSCD_STATE_CSW
} MSCD_STATE;

typedef struct {
    uint8_t ep_num, iface_num;
    uint16_t ep_size;
    IO* control;
    IO* data;
    MSCD_STATE state;
    unsigned int residue;
    SCSIS_RESPONSE resp;

    //for multiple LUN each scsi for each LUN
    SCSIS* scsis;
} MSCD;

static void mscd_destroy(MSCD* mscd)
{
    scsis_destroy(mscd->scsis);
    io_destroy(mscd->control);
    io_destroy(mscd->data);
    free(mscd);
}

static void mscd_fatal(USBD* usbd, MSCD* mscd)
{
    usbd_usb_ep_set_stall(usbd, mscd->ep_num);
    usbd_usb_ep_set_stall(usbd, USB_EP_IN | mscd->ep_num);
    mscd->state = MSCD_STATE_CBW;
}

static void mscd_flush(USBD* usbd, MSCD* mscd)
{
    usbd_usb_ep_flush(usbd, mscd->ep_num);
    usbd_usb_ep_flush(usbd, USB_EP_IN | mscd->ep_num);
    scsis_reset(mscd->scsis);
    mscd->state = MSCD_STATE_CBW;
}

void mscd_class_configured(USBD* usbd, USB_CONFIGURATION_DESCRIPTOR_TYPE* cfg)
{
    USB_INTERFACE_DESCRIPTOR_TYPE* iface;
    USB_ENDPOINT_DESCRIPTOR_TYPE* ep;
    unsigned int ep_num, ep_size, iface_num;
    ep_num = 0;

    //check control/data ep here
    for (iface = usb_get_first_interface(cfg); iface != NULL; iface = usb_get_next_interface(cfg, iface))
    {
        ep = (USB_ENDPOINT_DESCRIPTOR_TYPE*)usb_interface_get_first_descriptor(cfg, iface, USB_ENDPOINT_DESCRIPTOR_INDEX);
        if (ep != NULL && iface->bInterfaceClass == MSC_INTERFACE_CLASS)
        {
            ep_num = USB_EP_NUM(ep->bEndpointAddress);
            ep_size = ep->wMaxPacketSize;
            iface_num = iface->bInterfaceNumber;
            break;
        }
    }

    //No MSC interface
    if (ep_num == 0)
        return;
    MSCD* mscd = (MSCD*)malloc(sizeof(MSCD));
    if (mscd == NULL)
        return;

    mscd->iface_num = iface_num;
    mscd->ep_num = ep_num;
    mscd->ep_size = ep_size;

    mscd->state = MSCD_STATE_CBW;
    mscd->control = io_create(ep_size);
    mscd->data = io_create(USBD_MSC_MAX_IO_SIZE + sizeof(SCSI_STACK));
    mscd->scsis = scsis_create();
    if (mscd->control == NULL || mscd->data == NULL || mscd->scsis == NULL)
    {
        mscd_destroy(mscd);
        return;
    }

#if (USBD_MSC_DEBUG_REQUESTS)
    printf("Found USB MSCD class, EP%d, size: %d, iface: %d\n\r", ep_num, ep_size, iface_num);
#endif //USBD_CDC_DEBUG_REQUESTS

    usbd_register_interface(usbd, iface_num, &__MSCD_CLASS, mscd);
    usbd_register_endpoint(usbd, iface_num, ep_num);

    usbd_usb_ep_open(usbd, USB_EP_IN | ep_num, USB_EP_BULK, ep_size);
    usbd_usb_ep_open(usbd, ep_num, USB_EP_BULK, ep_size);

    usbd_usb_ep_read(usbd, mscd->ep_num, mscd->control, mscd->ep_size);
}

void mscd_class_reset(USBD* usbd, void* param)
{
    MSCD* mscd = (MSCD*)param;

    usbd_unregister_endpoint(usbd, mscd->iface_num, mscd->ep_num);
    usbd_unregister_interface(usbd, mscd->iface_num, &__MSCD_CLASS);
    usbd_usb_ep_close(usbd, mscd->ep_num);
    usbd_usb_ep_close(usbd, USB_EP_IN | mscd->ep_num);
    mscd_destroy(mscd);
}

void mscd_class_suspend(USBD* usbd, void* param)
{
    MSCD* mscd = (MSCD*)param;
    mscd_flush(usbd, mscd);
}

void mscd_class_resume(USBD* usbd, void* param)
{
    MSCD* mscd = (MSCD*)param;
    usbd_usb_ep_read(usbd, mscd->ep_num, mscd->control, mscd->ep_size);
}

static inline int mscd_get_max_lun(USBD* usbd, MSCD* mscd, IO* io)
{
    *((uint8_t*)io_data(io)) = USBD_MSC_LUN_COUNT - 1;
    io->data_size = sizeof(uint8_t);
#if (USBD_MSC_DEBUG_REQUESTS)
    printf("MSCD: Get Max Lun - %d\n\r", USBD_MSC_LUN_COUNT - 1);
#endif //USBD_CDC_DEBUG_REQUESTS
    return sizeof(uint8_t);
}

static int mscd_mass_storage_reset(USBD* usbd, MSCD* mscd)
{
    mscd_flush(usbd, mscd);
    usbd_usb_ep_read(usbd, mscd->ep_num, mscd->control, mscd->ep_size);
#if (USBD_MSC_DEBUG_REQUESTS)
    printf("MSCD: Mass Storage Reset\n\r");
#endif //USBD_CDC_DEBUG_REQUESTS
    return 0;
}

int mscd_class_setup(USBD* usbd, void* param, SETUP* setup, IO* io)
{
    MSCD* mscd = (MSCD*)param;
    int res = -1;
    switch (setup->bRequest)
    {
    case MSC_BO_GET_MAX_LUN:
        res = mscd_get_max_lun(usbd, mscd, io);
        break;
    case MSC_BO_MASS_STORAGE_RESET:
        res = mscd_mass_storage_reset(usbd, mscd);
        break;
    }
    return res;
}

static void mscd_csw(USBD* usbd, MSCD* mscd)
{
    uint32_t tag;
    CBW* cbw = io_data(mscd->control);
    CSW* csw = io_data(mscd->control);
    tag = cbw->dCBWTag;

    csw->dCSWSignature = MSC_CSW_SIGNATURE;
    csw->dCSWTag = tag;
    csw->dCSWDataResidue = mscd->residue;
    switch (mscd->resp)
    {
    case SCSIS_RESPONSE_PASS:
        csw->bCSWStatus = MSC_CSW_COMMAND_PASSED;
        break;
    case SCSIS_RESPONSE_FAIL:
        csw->bCSWStatus = MSC_CSW_COMMAND_FAILED;
        break;
    default:
        csw->bCSWStatus = MSC_CSW_PHASE_ERROR;
    }
    mscd->control->data_size = sizeof(CSW);
    mscd->state = MSCD_STATE_CSW;
    usbd_usb_ep_write(usbd, mscd->ep_num, mscd->control);
}

static void mscd_request_processed(USBD* usbd, MSCD* mscd)
{
    CBW* cbw = io_data(mscd->control);
    if (MSC_CBW_FLAG_DATA_IN(cbw->bmCBWFlags) && cbw->dCBWDataTransferLength)
    {
        mscd->state = MSCD_STATE_DATA_IN;
        if (mscd->data->data_size < cbw->dCBWDataTransferLength)
        {
            mscd->residue = cbw->dCBWDataTransferLength - mscd->data->data_size;
            if (mscd->data->data_size == 0)
                mscd->state = MSCD_STATE_DATA_IN_ZLP;
        }
        mscd->data->data_size = cbw->dCBWDataTransferLength - mscd->residue;
        usbd_usb_ep_write(usbd, mscd->ep_num, mscd->data);
    }
    else
        mscd_csw(usbd, mscd);
}

static void mscd_scsi_request(USBD* usbd, MSCD* mscd)
{
    CBW* cbw = io_data(mscd->control);
    mscd->resp = scsis_request(mscd->scsis, cbw->CBWCB, mscd->data);
    if (mscd->resp == SCSIS_RESPONSE_STORAGE_REQUEST)
    {
        mscd->state = MSCD_STATE_STORAGE;
        usbd_io_user(usbd, mscd->iface_num, cbw->bCBWLUN, HAL_CMD(HAL_USBD_IFACE, USB_MSC_STORAGE_REQUEST), mscd->control, 0);
    }
    else
        mscd_request_processed(usbd, mscd);
}

static inline void mscd_cbw(USBD* usbd, MSCD* mscd)
{
    CBW* cbw = io_data(mscd->control);
    if (mscd->control->data_size < sizeof(CBW) || cbw->dCBWSignature != MSC_CBW_SIGNATURE)
    {
#if (USBD_MSC_DEBUG_REQUESTS)
        printf("MSCD: Fatal - invalid CBW\n\r");
#endif //USBD_CDC_DEBUG_REQUESTS
        mscd_fatal(usbd, mscd);
    }
#if (USBD_MSC_DEBUG_IO)
    printf("USB_MSC: dCBWTag: %d\n\r", cbw->dCBWTag);
    printf("USB_MSC: dCBWDataTransferLength: %d\n\r", cbw->dCBWDataTransferLength);
    printf("USB_MSC: dCBWDataFlags: %02x\n\r", cbw->bmCBWFlags);
    printf("USB_MSC: dCBWLUN: %d\n\r", cbw->bCBWLUN);
#endif //USBD_MSC_DEBUG_IO
    mscd->residue = 0;
    if (MSC_CBW_FLAG_DATA_IN(cbw->bmCBWFlags) || cbw->dCBWDataTransferLength == 0)
    {
        mscd->data->data_size = 0;
        mscd_scsi_request(usbd, mscd);
    }
    else
    {
        mscd->state = MSCD_STATE_DATA_OUT;
        if (cbw->dCBWDataTransferLength > USBD_MSC_MAX_IO_SIZE)
            mscd->residue = cbw->dCBWDataTransferLength - USBD_MSC_MAX_IO_SIZE;
        usbd_usb_ep_read(usbd, mscd->ep_num, mscd->data, cbw->dCBWDataTransferLength - mscd->residue);
    }
}

static inline void mscd_usb_rx(USBD* usbd, MSCD* mscd)
{
    switch (mscd->state)
    {
    case MSCD_STATE_CBW:
        mscd_cbw(usbd, mscd);
        break;
    case MSCD_STATE_DATA_OUT:
        mscd_scsi_request(usbd, mscd);
        break;
    default:
#if (USBD_MSC_DEBUG_REQUESTS)
        printf("MSCD: invalid stage on read - %d\n\r", mscd->state);
#endif //USBD_CDC_DEBUG_REQUESTS
        mscd_fatal(usbd, mscd);
    }
}

static inline void mscd_usb_tx(USBD* usbd, MSCD* mscd)
{
    switch (mscd->state)
    {
    case MSCD_STATE_CSW:
        mscd->state = MSCD_STATE_CBW;
        usbd_usb_ep_read(usbd, mscd->ep_num, mscd->control, mscd->ep_size);
        break;
    case MSCD_STATE_DATA_IN:
        if (mscd->residue && ((mscd->data->data_size % mscd->ep_size) == 0))
        {
            mscd->state = MSCD_STATE_DATA_IN_ZLP;
            mscd->data->data_size = 0;
            usbd_usb_ep_write(usbd, mscd->ep_num, mscd->data);
            break;
        }
        //follow down
    case MSCD_STATE_DATA_IN_ZLP:
        mscd_csw(usbd, mscd);
        break;
    default:
#if (USBD_MSC_DEBUG_REQUESTS)
        printf("MSCD: invalid stage on write - %d\n\r", mscd->state);
#endif //USBD_CDC_DEBUG_REQUESTS
        mscd_fatal(usbd, mscd);
    }
}

static inline bool mscd_driver_event(USBD* usbd, MSCD* mscd, IPC* ipc)
{
    bool need_post = false;
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_READ:
        mscd_usb_rx(usbd, mscd);
        break;
    case IPC_WRITE:
        mscd_usb_tx(usbd, mscd);
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
        need_post = true;
    }
    return need_post;
}

static inline void mscd_storage_response(USBD* usbd, MSCD* mscd, int param3)
{
    if (mscd->state != MSCD_STATE_STORAGE)
    {
        //generally not error - response can arrive after mass storage reset
        mscd_flush(usbd, mscd);
        usbd_usb_ep_read(usbd, mscd->ep_num, mscd->control, mscd->ep_size);
        return;
    }
    if (param3 < 0)
    {
        mscd->resp = SCSIS_RESPONSE_PHASE_ERROR;
        mscd_csw(usbd, mscd);
    }
    else
    {
        mscd->resp = scsis_storage_response(mscd->scsis, mscd->data);
        mscd_csw(usbd, mscd);
    }
}

bool mscd_class_request(USBD* usbd, void* param, IPC* ipc)
{
    MSCD* mscd = (MSCD*)param;
    bool need_post = false;
    if (HAL_GROUP(ipc->cmd) == HAL_USB)
        need_post = mscd_driver_event(usbd, mscd, ipc);
    else
        switch (HAL_ITEM(ipc->cmd))
        {
        case USB_MSC_STORAGE_REQUEST:
            mscd_storage_response(usbd, mscd, (int)ipc->param3);
            break;
        default:
            error(ERROR_NOT_SUPPORTED);
            need_post = true;
        }
    return need_post;
}

const USBD_CLASS __MSCD_CLASS = {
    mscd_class_configured,
    mscd_class_reset,
    mscd_class_suspend,
    mscd_class_resume,
    mscd_class_setup,
    mscd_class_request,
};
