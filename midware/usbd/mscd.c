/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
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
#include "../../userspace/storage.h"
#include "../scsis/scsis.h"
#include <stdint.h>
#include "usb.h"
#include "sys_config.h"

typedef enum  {
    MSCD_STATE_IDLE = 0,
    MSCD_STATE_CBW,
    MSCD_STATE_PROCESSING,
    MSCD_STATE_ZLP,
    MSCD_STATE_CSW,
} MSCD_STATE;

typedef struct {
    IO* control;
    IO* data;
    uint8_t ep_num, iface_num;
    uint16_t ep_size;
    MSCD_STATE state;
    unsigned int residue, tag, csw_status, lun_count;
    USBD* usbd;

    CBW* cbw;
    uint32_t io_busy_mask;
    int io_owner;
    //SCSIS* for each lun is following
} MSCD;

#define MSCD_SCSI(mscd)                            ((SCSIS**)((uint8_t*)(mscd) + sizeof(MSCD)))

static void mscd_destroy(MSCD* mscd)
{
    int i;
    for (i = 0; i < mscd->lun_count; ++i)
    {
        if (MSCD_SCSI(mscd)[i] != NULL)
            scsis_destroy(MSCD_SCSI(mscd)[i]);
    }
    io_destroy(mscd->data);
    io_destroy(mscd->control);
    free(mscd);
}

static void mscd_flush(USBD* usbd, MSCD* mscd)
{
    int i;
    usbd_usb_ep_flush(usbd, mscd->ep_num);
    usbd_usb_ep_flush(usbd, USB_EP_IN | mscd->ep_num);
    for (i = 0; i < mscd->lun_count; ++i)
        scsis_reset(MSCD_SCSI(mscd)[i]);
    mscd->io_busy_mask = 0;
    mscd->io_owner = 0xff;
    mscd->state = MSCD_STATE_IDLE;
}

static void mscd_fatal(USBD* usbd, MSCD* mscd)
{
    mscd_flush(usbd, mscd);
    usbd_usb_ep_set_stall(usbd, mscd->ep_num);
    usbd_usb_ep_set_stall(usbd, USB_EP_IN | mscd->ep_num);
}

static void mscd_read_cbw(USBD* usbd, MSCD* mscd)
{
    mscd->state = MSCD_STATE_IDLE;
    usbd_usb_ep_read(usbd, mscd->ep_num, mscd->control, mscd->ep_size);
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
    printf("USB_MSC: dCSWTag: %d\n", csw->dCSWTag);
    printf("USB_MSC: dCSWDataResidue: %d\n", csw->dCSWDataResidue);
    printf("USB_MSC: bCSWStatus: %d\n", csw->bCSWStatus);
#endif //USBD_MSC_DEBUG_IO
}

static void mscd_cbw_process(USBD* usbd, MSCD* mscd)
{
    if (mscd->cbw->bCBWLUN >= mscd->lun_count)
    {
        mscd_fatal(usbd, mscd);
        return;
    }
    if (mscd->io_owner < 0)
    {
        mscd->state = MSCD_STATE_PROCESSING;
        mscd->io_owner = mscd->cbw->bCBWLUN;
        scsis_request_cmd(MSCD_SCSI(mscd)[mscd->io_owner], mscd->data, mscd->cbw->CBWCB);
    }
}

static void mscd_request_processed(USBD* usbd, MSCD* mscd)
{
    //need ZLP if data transfer is not fully complete
    if (mscd->residue && (((mscd->cbw->dCBWDataTransferLength - mscd->residue) % mscd->ep_size) == 0))
    {
        mscd->control->data_size = 0;
        if (MSC_CBW_FLAG_DATA_IN(mscd->cbw->bmCBWFlags))
            usbd_usb_ep_write(usbd, mscd->ep_num, mscd->control);
        else
        {
            //some hardware required to be multiple of MPS
            usbd_usb_ep_read(usbd, mscd->ep_num, mscd->control, mscd->ep_size);
        }
        mscd->state = MSCD_STATE_ZLP;
    }
    else
        mscd_write_csw(usbd, mscd);
}

static void mscd_release_io(MSCD* mscd)
{
    int i;
    mscd->io_owner = -1;
    if (mscd->io_busy_mask)
    {
        for (i = 0; i < mscd->lun_count; ++i)
        {
            if (mscd->io_busy_mask & (1 << i))
            {
                mscd->io_owner = i;
                mscd->io_busy_mask &= ~(1 << i);
                scsis_host_give_io(MSCD_SCSI(mscd)[i], mscd->data);
                return;
            }
        }
    }
    if (mscd->state == MSCD_STATE_CBW)
        mscd_cbw_process(mscd->usbd, mscd);
}

void mscd_host_cb(void* param, unsigned int id, SCSIS_RESPONSE response, unsigned int size)
{
    MSCD* mscd = param;

    switch (response)
    {
    case SCSIS_RESPONSE_READ:
        if (size > mscd->residue)
            size = mscd->residue;
        mscd->residue -= size;
        //some hardware required to be multiple of MPS
        usbd_usb_ep_read(mscd->usbd, mscd->ep_num, mscd->data, (size + mscd->ep_size - 1) & ~(mscd->ep_size - 1));
        break;
    case SCSIS_RESPONSE_WRITE:
        if (mscd->data->data_size > mscd->residue)
            mscd->data->data_size = mscd->residue;
        mscd->residue -= mscd->data->data_size;
        usbd_usb_ep_write(mscd->usbd, mscd->ep_num, mscd->data);
        break;
    case SCSIS_RESPONSE_PASS:
        mscd->csw_status = MSC_CSW_COMMAND_PASSED;
        mscd_request_processed(mscd->usbd, mscd);
        break;
    case SCSIS_RESPONSE_FAIL:
        mscd->csw_status = MSC_CSW_COMMAND_FAILED;
        mscd_request_processed(mscd->usbd, mscd);
        break;
    case SCSIS_RESPONSE_NEED_IO:
        if (mscd->io_owner < 0)
        {
            mscd->io_owner = id;
            scsis_host_give_io(MSCD_SCSI(mscd)[id], mscd->data);
        }
        else
            mscd->io_busy_mask |= 1 << id;
        break;
    case SCSIS_RESPONSE_RELEASE_IO:
        mscd_release_io(mscd);
        break;
    default:
        break;
    }
}

void mscd_class_configured(USBD* usbd, USB_CONFIGURATION_DESCRIPTOR* cfg)
{
    USB_INTERFACE_DESCRIPTOR* iface;
    USB_ENDPOINT_DESCRIPTOR* ep;
    void* config;
    int i;
    unsigned int ep_num, ep_size, iface_num, lun_count;
    ep_num = 0;

    //check control/data ep here
    for (iface = usb_get_first_interface(cfg); iface != NULL; iface = usb_get_next_interface(cfg, iface))
    {
        ep = (USB_ENDPOINT_DESCRIPTOR*)usb_interface_get_first_descriptor(cfg, iface, USB_ENDPOINT_DESCRIPTOR_TYPE);
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
    i = usbd_get_cfg(usbd, iface_num);
    config = usbd_get_cfg_data(usbd, i);
    lun_count = 0;
    if (usbd_get_cfg_data_size(usbd, i) > (int)sizeof(uint32_t))
        lun_count = MSC_LUN_COUNT(config);
    if ((config == NULL) || (usbd_get_cfg_data_size(usbd, i) < (int)((sizeof(SCSI_STORAGE_DESCRIPTOR)) * lun_count + sizeof(uint32_t))) || (lun_count == 0))
    {
#if (USBD_MSC_DEBUG_ERRORS)
        printf("MSCD: Failed to read user configuration\n");
#endif //USBD_MSC_DEBUG_ERRORS
        return;
    }
    MSCD* mscd = (MSCD*)malloc(sizeof(MSCD) + sizeof(void*) * lun_count);
    if (mscd == NULL)
        return;

    mscd->iface_num = iface_num;
    mscd->ep_num = ep_num;
    mscd->ep_size = ep_size;
    mscd->usbd = usbd;

    mscd->control = io_create(ep_size);
    mscd->data = io_create(USBD_MSC_IO_SIZE + sizeof(STORAGE_STACK));
    mscd->lun_count = lun_count;
    mscd->io_busy_mask = 0x00000000;
    mscd->io_owner = -1;
    mscd->cbw = io_data(mscd->control);
    for (i = 0; i < mscd->lun_count; ++i)
        MSCD_SCSI(mscd)[i] = scsis_create(mscd_host_cb, mscd, i, MSC_LUN_CONFIGURATION(config, i));
    if (mscd->control == NULL || mscd->data == NULL || MSCD_SCSI(mscd)[mscd->lun_count - 1] == NULL)
    {
#if (USBD_MSC_DEBUG_ERRORS)
        printf("MSCD: Out of memory\n");
#endif //USBD_MSC_DEBUG_ERRORS
        mscd_destroy(mscd);
        return;
    }

    for (i = 0; i < mscd->lun_count; ++i)
        scsis_init(MSCD_SCSI(mscd)[i]);

#if (USBD_MSC_DEBUG_REQUESTS)
    printf("Found USB MSCD class, EP%d, size: %d, iface: %d\n", ep_num, ep_size, iface_num);
#endif //USBD_MSC_DEBUG_REQUESTS

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
    *((uint8_t*)io_data(io)) = mscd->lun_count - 1;
    io->data_size = sizeof(uint8_t);
#if (USBD_MSC_DEBUG_REQUESTS)
    printf("MSCD: Get Max Lun - %d\n", mscd->lun_count - 1);
#endif //USBD_MSC_DEBUG_REQUESTS
    return sizeof(uint8_t);
}

static int mscd_mass_storage_reset(USBD* usbd, MSCD* mscd)
{
    mscd_flush(usbd, mscd);
    mscd_read_cbw(usbd, mscd);
#if (USBD_MSC_DEBUG_REQUESTS)
    printf("MSCD: Mass Storage Reset\n");
#endif //USBD_MSC_DEBUG_REQUESTS
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

static inline void mscd_cbw_rx(USBD* usbd, MSCD* mscd)
{
    if (mscd->control->data_size < sizeof(CBW) || mscd->cbw->dCBWSignature != MSC_CBW_SIGNATURE)
    {
#if (USBD_MSC_DEBUG_ERRORS)
        printf("MSCD: Fatal - invalid CBW\n");
#endif //USBD_MSC_DEBUG_ERRORS
        mscd_fatal(usbd, mscd);
    }
#if (USBD_MSC_DEBUG_IO)
    printf("USB_MSC: dCBWTag: %d\n", mscd->cbw->dCBWTag);
    printf("USB_MSC: dCBWDataTransferLength: %d\n", mscd->cbw->dCBWDataTransferLength);
    printf("USB_MSC: dCBWDataFlags: %02x\n", mscd->cbw->bmCBWFlags);
    printf("USB_MSC: dCBWLUN: %d\n", mscd->cbw->bCBWLUN);
#endif //USBD_MSC_DEBUG_IO
    mscd->state = MSCD_STATE_CBW;
    mscd->residue = mscd->cbw->dCBWDataTransferLength;
    mscd->tag = mscd->cbw->dCBWTag;
    mscd_cbw_process(usbd, mscd);
}

static inline void mscd_usb_io_complete(USBD* usbd, MSCD* mscd, int resp_size)
{
    switch (mscd->state)
    {
    case MSCD_STATE_IDLE:
        mscd_cbw_rx(usbd, mscd);
        break;
    case MSCD_STATE_PROCESSING:
        scsis_host_io_complete(MSCD_SCSI(mscd)[mscd->io_owner], resp_size);
        break;
    case MSCD_STATE_ZLP:
        mscd_write_csw(usbd, mscd);
        break;
    case MSCD_STATE_CSW:
        mscd_read_cbw(usbd, mscd);
        break;
    default:
        mscd_release_io(mscd);
        break;
    }
}

static inline void mscd_driver_event(USBD* usbd, MSCD* mscd, IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_WRITE:
    case IPC_READ:
        mscd_usb_io_complete(usbd, mscd, (int)ipc->param3);
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
    }
}

void mscd_class_request(USBD* usbd, void* param, IPC* ipc)
{
    MSCD* mscd = (MSCD*)param;
    switch (HAL_GROUP(ipc->cmd))
    {
    case HAL_USB:
        mscd_driver_event(usbd, mscd, ipc);
        break;
    default:
        if (USBD_IFACE_ITEM(ipc->param1) == mscd->io_owner)
            scsis_request(MSCD_SCSI(mscd)[mscd->io_owner], ipc);
        else
            mscd_release_io(mscd);
    }
}

const USBD_CLASS __MSCD_CLASS = {
    mscd_class_configured,
    mscd_class_reset,
    mscd_class_suspend,
    mscd_class_resume,
    mscd_class_setup,
    mscd_class_request,
};
