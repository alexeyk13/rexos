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
#include "../scsis/scsis.h"
#include <stdint.h>
#include "usb.h"
#include "sys_config.h"

typedef enum  {
    MSCD_STATE_CBW = 0,
    MSCD_STATE_PROCESSING,
    MSCD_STATE_DIRECT_WRITE,
    MSCD_STATE_ZLP,
    MSCD_STATE_CSW,
    MSCD_STATE_IDLE
} MSCD_STATE;

typedef struct {
    uint8_t ep_num, iface_num;
    uint16_t ep_size;
    IO* control;
    MSCD_STATE state;
    unsigned int residue, tag, csw_status;
    USBD* usbd;

    //for multiple LUN each scsi for each LUN
    SCSIS* scsis;
} MSCD;

static void mscd_destroy(MSCD* mscd)
{
    scsis_destroy(mscd->scsis);
    io_destroy(mscd->control);
    free(mscd);
}

static void mscd_fatal(USBD* usbd, MSCD* mscd)
{
    usbd_usb_ep_set_stall(usbd, mscd->ep_num);
    usbd_usb_ep_set_stall(usbd, USB_EP_IN | mscd->ep_num);
    mscd->state = MSCD_STATE_IDLE;
}

static void mscd_flush(USBD* usbd, MSCD* mscd)
{
    usbd_usb_ep_flush(usbd, mscd->ep_num);
    usbd_usb_ep_flush(usbd, USB_EP_IN | mscd->ep_num);
    scsis_reset(mscd->scsis);
    mscd->state = MSCD_STATE_IDLE;
}

static void mscd_read_cbw(USBD* usbd, MSCD* mscd)
{
    usbd_usb_ep_read(usbd, mscd->ep_num, mscd->control, mscd->ep_size);
    mscd->state = MSCD_STATE_CBW;
}

static void mscd_write_csw(USBD* usbd, MSCD* mscd)
{
    CSW* csw = io_data(mscd->control);

    csw->dCSWSignature = MSC_CSW_SIGNATURE;
    csw->dCSWTag = mscd->tag;
    csw->dCSWDataResidue = mscd->residue;
    csw->bCSWStatus = mscd->csw_status;
    mscd->control->data_size = sizeof(CSW);
    usbd_usb_ep_write(usbd, mscd->ep_num, mscd->control);
    mscd->state = MSCD_STATE_CSW;
#if (USBD_MSC_DEBUG_IO)
    printf("USB_MSC: dCSWTag: %d\n\r", csw->dCSWTag);
    printf("USB_MSC: dCSWDataResidue: %d\n\r", csw->dCSWDataResidue);
    printf("USB_MSC: bCSWStatus: %d\n\r", csw->bCSWStatus);
#endif //USBD_MSC_DEBUG_IO
}

static void mscd_request_processed(USBD* usbd, MSCD* mscd)
{
    CBW* cbw = io_data(mscd->control);
    //need ZLP if data transfer is not fully complete
    if (mscd->residue && (((cbw->dCBWDataTransferLength - mscd->residue) % mscd->ep_size) == 0))
    {
        mscd->control->data_size = 0;
        if (MSC_CBW_FLAG_DATA_IN(cbw->bmCBWFlags))
            usbd_usb_ep_write(usbd, mscd->ep_num, mscd->control);
        else
            usbd_usb_ep_read(usbd, mscd->ep_num, mscd->control, 0);
        mscd->state = MSCD_STATE_ZLP;
    }
    else
        mscd_write_csw(usbd, mscd);
}

static void mscd_phase_error(USBD* usbd, MSCD* mscd)
{
    scsis_reset(mscd->scsis);
    if (mscd->state == MSCD_STATE_PROCESSING)
    {
        mscd->csw_status = MSC_CSW_PHASE_ERROR;
        mscd_request_processed(usbd, mscd);
    }
    else
    {
        mscd_flush(usbd, mscd);
        mscd_read_cbw(usbd, mscd);
    }
}

static void mscd_scsi_request(USBD* usbd, MSCD* mscd)
{
    CBW* cbw = io_data(mscd->control);
    scsis_request(mscd->scsis, cbw->CBWCB);
}

void mscd_host_cb(void* param, IO* io, SCSIS_REQUEST request)
{
    MSCD* mscd = param;
    unsigned int size;

    if (mscd->state == MSCD_STATE_PROCESSING)
    {
        switch (request)
        {
        case SCSIS_REQUEST_READ:
            size = ((SCSI_STACK*)io_stack(io))->size;
            if (size > mscd->residue)
                size = mscd->residue;
            mscd->residue -= size;
            usbd_usb_ep_read(mscd->usbd, mscd->ep_num, io, size);
            break;
        case SCSIS_REQUEST_WRITE:
            if (io->data_size > mscd->residue)
                io->data_size = mscd->residue;
            mscd->residue -= io->data_size;
            usbd_usb_ep_write(mscd->usbd, mscd->ep_num, io);
            break;
        case SCSIS_REQUEST_PASS:
            mscd->csw_status = MSC_CSW_COMMAND_PASSED;
            mscd_request_processed(mscd->usbd, mscd);
            break;
        case SCSIS_REQUEST_FAIL:
            mscd->csw_status = MSC_CSW_COMMAND_FAILED;
            mscd_request_processed(mscd->usbd, mscd);
            break;
        default:
    #if (USBD_MSC_DEBUG_ERRORS)
            printf("USB MSC: phase error %d\n\r", request);
    #endif //USBD_MSC_DEBUG_ERRORS
            mscd_phase_error(mscd->usbd, mscd);
        }
    }
    else if (request == SCSIS_REQUEST_PASS)
    {
        //asynchronous write complete, ready for next
        if (mscd->state == MSCD_STATE_IDLE)
            mscd_read_cbw(mscd->usbd, mscd);
    }
    else
        mscd_phase_error(mscd->usbd, mscd);
}

void mscd_storage_cb(void* param, IO* io, SCSIS_REQUEST request)
{
    MSCD* mscd = param;
    CBW* cbw = io_data(mscd->control);
    unsigned int size = ((SCSI_STACK*)(io_stack(io)))->size;

    //just forward to user
    switch (request)
    {
    case SCSIS_REQUEST_READ:
        usbd_io_user(mscd->usbd, mscd->iface_num, cbw->bCBWLUN, HAL_CMD(HAL_USBD_IFACE, USB_MSC_READ), io, size);
        break;
    case SCSIS_REQUEST_WRITE:
        usbd_io_user(mscd->usbd, mscd->iface_num, cbw->bCBWLUN, HAL_CMD(HAL_USBD_IFACE, USB_MSC_WRITE), io, size);
        break;
    case SCSIS_REQUEST_VERIFY:
        usbd_io_user(mscd->usbd, mscd->iface_num, cbw->bCBWLUN, HAL_CMD(HAL_USBD_IFACE, USB_MSC_VERIFY), io, size);
        break;
    case SCSIS_REQUEST_GET_STORAGE_DESCRIPTOR:
        usbd_io_user(mscd->usbd, mscd->iface_num, cbw->bCBWLUN, HAL_CMD(HAL_USBD_IFACE, USB_MSC_GET_STORAGE_DESCRIPTOR), io, 0);
        break;
    case SCSIS_REQUEST_GET_MEDIA_DESCRIPTOR:
        usbd_io_user(mscd->usbd, mscd->iface_num, cbw->bCBWLUN, HAL_CMD(HAL_USBD_IFACE, USB_MSC_GET_MEDIA_DESCRIPTOR), io, 0);
        break;
    default:
#if (USBD_MSC_DEBUG_ERRORS)
        printf("USB MSC: invalid scsis storage request %d\n\r", request);
#endif //USBD_MSC_DEBUG_ERRORS
        mscd_phase_error(mscd->usbd, mscd);
    }
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
    mscd->usbd = usbd;

    mscd->control = io_create(ep_size);
    mscd->scsis = scsis_create(mscd_host_cb, mscd_storage_cb, mscd);
    if (mscd->control == NULL || mscd->scsis == NULL)
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

    mscd_read_cbw(usbd, mscd);
}

void mscd_class_reset(USBD* usbd, void* param)
{
    MSCD* mscd = (MSCD*)param;

    mscd_flush(usbd, mscd);
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
    mscd_read_cbw(usbd, mscd);
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

static inline void mscd_cbw(USBD* usbd, MSCD* mscd)
{
    CBW* cbw = io_data(mscd->control);
    if (mscd->control->data_size < sizeof(CBW) || cbw->dCBWSignature != MSC_CBW_SIGNATURE)
    {
#if (USBD_MSC_DEBUG_ERRORS)
        printf("MSCD: Fatal - invalid CBW\n\r");
#endif //USBD_CDC_DEBUG_ERRORS
        mscd_fatal(usbd, mscd);
    }
#if (USBD_MSC_DEBUG_IO)
    printf("USB_MSC: dCBWTag: %d\n\r", cbw->dCBWTag);
    printf("USB_MSC: dCBWDataTransferLength: %d\n\r", cbw->dCBWDataTransferLength);
    printf("USB_MSC: dCBWDataFlags: %02x\n\r", cbw->bmCBWFlags);
    printf("USB_MSC: dCBWLUN: %d\n\r", cbw->bCBWLUN);
#endif //USBD_MSC_DEBUG_IO
    mscd->residue = cbw->dCBWDataTransferLength;
    mscd->tag = cbw->dCBWTag;
    mscd->state = MSCD_STATE_PROCESSING;
    mscd_scsi_request(usbd, mscd);
}

static inline void mscd_usb_io_complete(USBD* usbd, MSCD* mscd)
{
    switch (mscd->state)
    {
    case MSCD_STATE_CBW:
        mscd_cbw(usbd, mscd);
        break;
    case MSCD_STATE_PROCESSING:
        scsis_host_io_complete(mscd->scsis);
        break;
    case MSCD_STATE_ZLP:
        mscd_write_csw(usbd, mscd);
        break;
    case MSCD_STATE_CSW:
        if (scsis_ready(mscd->scsis))
            mscd_read_cbw(usbd, mscd);
        else
            //wait for async write completion
            mscd->state = MSCD_STATE_IDLE;
        break;
    default:
#if (USBD_MSC_DEBUG_ERRORS)
        printf("MSCD: invalid stage on io complete - %d\n\r", mscd->state);
#endif //USBD_CDC_DEBUG_ERRORS
        mscd_phase_error(usbd, mscd);
    }
}

static inline bool mscd_driver_event(USBD* usbd, MSCD* mscd, IPC* ipc)
{
    bool need_post = false;
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_WRITE:
#if (SCSI_READ_CACHE)
        if (mscd->state == MSCD_STATE_DIRECT_WRITE)
        {
            //return IO to user
            usbd_io_user(mscd->usbd, mscd->iface_num, 0, HAL_CMD(HAL_USBD_IFACE, USB_MSC_DIRECT_WRITE), (IO*)ipc->param2, ipc->param3);
            mscd->state = MSCD_STATE_PROCESSING;
#if (USBD_MSC_DEBUG_IO)
            printf("MSC: direct write complete\n\r");
#endif //USBD_MSC_DEBUG_IO
            return false;
        }
#endif //SCSI_READ_CACHE
    case IPC_READ:
        mscd_usb_io_complete(usbd, mscd);
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
        need_post = true;
    }
    return need_post;
}

static inline void mscd_storage_response(USBD* usbd, MSCD* mscd, int param3)
{
    if (param3 < 0)
    {
#if (USBD_MSC_DEBUG_ERRORS)
        printf("MSCD: storage request failed, phase error\n\r");
#endif //USBD_MSC_DEBUG_ERRORS
        mscd_phase_error(usbd, mscd);
        return;
    }
    scsis_storage_io_complete(mscd->scsis);
}

#if (SCSI_READ_CACHE)
static inline void mscd_direct_write(USBD* usbd, MSCD* mscd, IO* io)
{
    unsigned int size = io->data_size;
    usbd_usb_ep_write(usbd, mscd->ep_num, io);
#if (USBD_MSC_DEBUG_IO)
    printf("MSC: direct write: %d\n\r", size);
#endif //USBD_MSC_DEBUG_IO
    if (size > mscd->residue)
        size = mscd->residue;
    mscd->residue -= size;
    mscd->state = MSCD_STATE_DIRECT_WRITE;
}
#endif //SCSI_READ_CACHE

bool mscd_class_request(USBD* usbd, void* param, IPC* ipc)
{
    MSCD* mscd = (MSCD*)param;
    bool need_post = false;
    if (HAL_GROUP(ipc->cmd) == HAL_USB)
        need_post = mscd_driver_event(usbd, mscd, ipc);
    else
        switch (HAL_ITEM(ipc->cmd))
        {
        case USB_MSC_GET_STORAGE_DESCRIPTOR:
        case USB_MSC_GET_MEDIA_DESCRIPTOR:
            //make another request with complete descriptor
            mscd_scsi_request(usbd, mscd);
            break;
        case USB_MSC_READ:
        case USB_MSC_WRITE:
        case USB_MSC_VERIFY:
            //just forward response to storage
            scsis_storage_io_complete(mscd->scsis);
            break;
#if (SCSI_READ_CACHE)
        case USB_MSC_DIRECT_WRITE:
            mscd_direct_write(usbd, mscd, (IO*)ipc->param2);
            break;
#endif //SCSI_READ_CACHE
        case USB_MSC_MEDIA_REMOVED:
            scsis_media_removed(mscd->scsis);
            need_post = true;
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
