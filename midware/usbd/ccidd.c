/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2018, Alexey Kramarenko
    All rights reserved.
*/

#include "ccidd.h"
#include "../../userspace/ccid.h"
#include "../../userspace/usb.h"
#include "../../userspace/stdlib.h"
#include "../../userspace/io.h"
#include "../../userspace/stdio.h"
#include "../../userspace/ipc.h"
#include "../../userspace/usb.h"
#include "../../userspace/error.h"
#include <string.h>
#include "sys_config.h"

typedef enum {
    CCIDD_STATE_IDLE = 0,
    CCIDD_STATE_NO_BUFFER,
    CCIDD_STATE_RXD,
    CCIDD_STATE_REQUEST,
    CCIDD_STATE_TXD,
    CCIDD_STATE_TXD_ZLP
} CCIDD_STATE;

typedef struct {
    IO *main_io, *txd_io, *user_io;
    IO* status_io;
    CCIDD_STATE state;
    uint16_t data_ep_size;
    uint8_t data_ep, status_ep, iface, msg_type, seq, abort_seq, status_busy, status_pending, slot_status, aborting, tx_busy, tx_pending, bBWI;
} CCIDD;

static void ccidd_destroy(CCIDD* ccidd)
{

    io_destroy(ccidd->main_io);
    io_destroy(ccidd->txd_io);
    io_destroy(ccidd->status_io);
    free(ccidd);
}

static void ccidd_rx(USBD* usbd, CCIDD* ccidd)
{
    //first frame going with data size, no ZLP from host
    ccidd->main_io->data_size = 0;
    usbd_usb_ep_read(usbd, ccidd->data_ep, ccidd->main_io, ccidd->data_ep_size);
}

static inline uint8_t ccidd_gen_status(CCIDD* ccidd, unsigned int command_status)
{
    return ((ccidd->slot_status & 3) << 0) | (command_status & 3) << 6;
}

static void ccidd_tx(USBD* usbd, CCIDD* ccidd, IO* io)
{
    //pend on ongoing TXD
    if (ccidd->tx_busy)
    {
        ccidd->tx_pending = true;
        return;
    }
    usbd_usb_ep_write(usbd, USB_EP_IN | ccidd->data_ep, io);
    ccidd->tx_busy = true;
}

static void ccidd_notify_slot_change(USBD* usbd, CCIDD* ccidd)
{
    CCID_NOTIFY_SLOT_CHANGE* notify;
    if (ccidd->status_ep)
    {
        if (ccidd->status_busy)
        {
            ccidd->status_pending = true;
            return;
        }

        notify = io_data(ccidd->status_io);

        notify->bMessageType = RDR_TO_PC_NOTIFY_SLOT_CHANGE;
        notify->bmSlotICCState = CCID_SLOT_ICC_STATE_CHANGED;

        if (ccidd->slot_status != CCID_SLOT_STATUS_ICC_NOT_PRESENT)
            notify->bmSlotICCState |= CCID_SLOT_STATUS_ICC_PRESENT_AND_INACTIVE;

#if (USBD_CCID_DEBUG_REQUESTS)
        printf("CCIDD: Send slot Status %02X %02X\n", notify->bMessageType, notify->bmSlotICCState);
#endif //USBD_CCID_DEBUG_REQUESTS

        ccidd->status_io->data_size = sizeof(CCID_NOTIFY_SLOT_CHANGE);
        usbd_usb_ep_write(usbd, USB_EP_IN | ccidd->status_ep, ccidd->status_io);
        ccidd->status_busy = true;
    }

}

#if (USBD_CCID_DEBUG_REQUESTS)
static void ccidd_dbg_status(uint8_t bStatus, uint8_t bError)
{
    if (bStatus != CCID_SLOT_STATUS_COMMAND_NO_ERROR)
        printf("bStatus: %x, bError: %x\n", bStatus, bError);
}
#endif //USBD_CCID_DEBUG_REQUESTS

static void ccidd_tx_slot_status_ex(USBD* usbd, CCIDD* ccidd, uint8_t seq, uint8_t error, uint8_t status, IO* io)
{
    CCID_MSG_SLOT_STATUS* msg = io_data(io);
    msg->bMessageType = RDR_TO_PC_SLOT_STATUS;
    msg->dwLength = 0;
    msg->bSlot = 0;
    msg->bSeq = seq;
    msg->bStatus = ccidd_gen_status(ccidd, status);
    msg->bError = error;
    msg->bClockStatus = CCID_CLOCK_STATUS_RUNNING;
    io->data_size = sizeof(CCID_MSG_SLOT_STATUS);
#if (USBD_CCID_DEBUG_REQUESTS)
    printf("RDR_to_PC_SlotStatus\n");
    ccidd_dbg_status(msg->bStatus, msg->bError);
#endif //USBD_CCID_DEBUG_REQUESTS
    ccidd_tx(usbd, ccidd, io);
}

static void ccidd_tx_slot_status(USBD* usbd, CCIDD* ccidd)
{
    ccidd_tx_slot_status_ex(usbd, ccidd, ccidd->seq, 0, CCID_SLOT_STATUS_COMMAND_NO_ERROR, ccidd->txd_io);
    ccidd->state = CCIDD_STATE_TXD;
}

static void ccidd_tx_slot_status_error(USBD* usbd, CCIDD* ccidd, uint8_t error)
{
    ccidd_tx_slot_status_ex(usbd, ccidd, ccidd->seq, error, CCID_SLOT_STATUS_COMMAND_FAIL, ccidd->txd_io);
    ccidd->state = CCIDD_STATE_TXD;
}

static void ccidd_tx_data_block_ex(USBD* usbd, CCIDD* ccidd, uint8_t seq, uint8_t error, uint8_t status, IO* io, IO* data_io)
{
    CCID_MSG_DATA_BLOCK* msg = io_data(io);
    msg->bMessageType = RDR_TO_PC_DATA_BLOCK;
    if (data_io)
    {
        msg->dwLength = data_io->data_size;
        memcpy((uint8_t*)io_data(io) + sizeof(CCID_MSG), io_data(data_io), data_io->data_size);
        io->data_size = data_io->data_size + sizeof(CCID_MSG);
    }
    else
        msg->dwLength = 0;
    msg->bSlot = 0;
    msg->bSeq = seq;
    msg->bStatus = ccidd_gen_status(ccidd, status);
    msg->bError = error;
    msg->bChainParameter = 0;
#if (USBD_CCID_DEBUG_REQUESTS)
    printf("RDR_to_PC_DataBlock len: %d\n", msg->dwLength);
    ccidd_dbg_status(msg->bStatus, msg->bError);
#endif //USBD_CCID_DEBUG_REQUESTS
#if (USBD_CCID_DEBUG_IO)
    if (data_io)
        usbd_dump((uint8_t*)io_data(io) + sizeof(CCID_MSG), msg->dwLength, "CCIDD R-BLOCK");
#endif
    ccidd_tx(usbd, ccidd, io);
}

static void ccidd_tx_data_block(USBD* usbd, CCIDD* ccidd)
{
    ccidd_tx_data_block_ex(usbd, ccidd, ccidd->seq, 0, CCID_SLOT_STATUS_COMMAND_NO_ERROR, ccidd->txd_io, ccidd->user_io);
    ccidd->state = CCIDD_STATE_TXD;
}

static void ccidd_tx_params_ex(USBD* usbd, CCIDD* ccidd, uint8_t error, uint8_t seq, uint8_t status, CCID_PROTOCOL protocol, IO* io, IO* data_io)
{
    CCID_MSG_PARAMS* msg = io_data(io);
    if (data_io)
    {
        msg->dwLength = data_io->data_size;
        memcpy((uint8_t*)io_data(io) + sizeof(CCID_MSG), io_data(data_io), data_io->data_size);
        io->data_size = data_io->data_size + sizeof(CCID_MSG);
    }
    else
        msg->dwLength = 0;
    msg->bMessageType = RDR_TO_PC_PARAMETERS;
    msg->bSlot = 0;
    msg->bSeq = seq;
    msg->bStatus = ccidd_gen_status(ccidd, status);
    msg->bError = error;
    msg->bProtocolNum = protocol;


#if (USBD_CCID_DEBUG_REQUESTS)
    printf("RDR_to_PC_Parameters T=%d, len: %d\n", protocol, msg->dwLength);
    ccidd_dbg_status(msg->bStatus, msg->bError);
#endif //USBD_CCID_DEBUG_REQUESTS
#if (USBD_CCID_DEBUG_IO)
    if (data_io)
        usbd_dump((uint8_t*)io_data(io) + sizeof(CCID_MSG), msg->dwLength, "CCIDD Parameters");
#endif
    ccidd_tx(usbd, ccidd, io);
}

static void ccidd_tx_params(USBD* usbd, CCIDD* ccidd, CCID_PROTOCOL protocol)
{
    ccidd_tx_params_ex(usbd, ccidd, ccidd->seq, 0, CCID_SLOT_STATUS_COMMAND_NO_ERROR, protocol, ccidd->txd_io, ccidd->user_io);
    ccidd->state = CCIDD_STATE_TXD;
}

static void ccidd_tx_rate_clock_ex(USBD* usbd, CCIDD* ccidd, uint8_t seq, uint8_t error, uint8_t status, IO* io, unsigned int khz, unsigned int bps)
{
    CCID_MSG_RATE_CLOCK* msg = io_data(io);
    msg->bMessageType = RDR_TO_PC_RATE_AND_CLOCK;
    msg->dwLength = 8;
    msg->bSlot = 0;
    msg->bSeq = seq;
    msg->bStatus = ccidd_gen_status(ccidd, status);
    msg->bError = error;
    msg->bRFU = 0x00;
    msg->dwClockFrequency = khz;
    msg->dwDataRate = bps;
#if (USBD_CCID_DEBUG_REQUESTS)
    printf("RDR_to_PC_DataRateAndClockFrequency clock: %d KHz, rate: %d BPS\n", khz, bps);
    ccidd_dbg_status(msg->bStatus, msg->bError);
#endif //USBD_CCID_DEBUG_REQUESTS
    ccidd_tx(usbd, ccidd, io);
}

static void ccidd_tx_rate_clock(USBD* usbd, CCIDD* ccidd, unsigned int khz, unsigned int bps)
{
    ccidd_tx_rate_clock_ex(usbd, ccidd, ccidd->seq, 0, CCID_SLOT_STATUS_COMMAND_NO_ERROR, ccidd->txd_io, khz, bps);
    ccidd->state = CCIDD_STATE_TXD;
}

static inline void ccidd_tx_error(USBD* usbd, CCIDD* ccidd, uint8_t error, bool use_main, uint8_t msg_type, uint8_t seq)
{
    IO* io;

    io = use_main ? ccidd->main_io : ccidd->txd_io;
    switch (msg_type)
    {
    case PC_TO_RDR_ICC_POWER_ON:
    case PC_TO_RDR_XFER_BLOCK:
        ccidd_tx_data_block_ex(usbd, ccidd, seq, error, CCID_SLOT_STATUS_COMMAND_FAIL, io, NULL);
        break;
    case PC_TO_RDR_GET_PARAMETERS:
    case PC_TO_RDR_RESET_PARAMETERS:
    case PC_TO_RDR_SET_PARAMETERS:
        ccidd_tx_params_ex(usbd, ccidd, seq, error, CCID_SLOT_STATUS_COMMAND_FAIL, 0, io, NULL);
        break;
    case PC_TO_RDR_SET_DATA_RATE_AND_CLOCK:
        ccidd_tx_rate_clock_ex(usbd, ccidd, seq, error, CCID_SLOT_STATUS_COMMAND_FAIL, io, 0, 0);
        break;
    default:
        ccidd_tx_slot_status_ex(usbd, ccidd, seq, error, CCID_SLOT_STATUS_COMMAND_FAIL, io);
    }
    ccidd->state = CCIDD_STATE_TXD;
    if (!use_main)
        ccidd_rx(usbd, ccidd);
}

static void ccidd_user_request(USBD* usbd, CCIDD* ccidd, unsigned int req, unsigned int param)
{
    usbd_io_user(usbd, ccidd->iface, 0, HAL_IO_REQ(HAL_USBD_IFACE, req), ccidd->user_io, param);
    ccidd->user_io = NULL;
    ccidd->state = CCIDD_STATE_REQUEST;
}

void ccidd_class_configured(USBD* usbd, USB_CONFIGURATION_DESCRIPTOR* cfg)
{
    USB_INTERFACE_DESCRIPTOR* iface;
    USB_ENDPOINT_DESCRIPTOR* ep;
    CCID_DESCRIPTOR* ccid_descriptor;
    CCIDD* ccidd;
    unsigned int status_ep_size;

    for (iface = usb_get_first_interface(cfg); iface != NULL; iface = usb_get_next_interface(cfg, iface))
    {
        if (iface->bInterfaceClass == CCID_INTERFACE_CLASS)
        {
            ccid_descriptor = (CCID_DESCRIPTOR*)usb_interface_get_first_descriptor(cfg, iface, USB_FUNCTIONAL_DESCRIPTOR);
            ccidd = malloc(sizeof(CCIDD));
            if (ccidd == NULL || ccid_descriptor == NULL)
                return;

            ccidd->iface = iface->bInterfaceNumber;

            ccidd->data_ep = ccidd->status_ep = 0;
            ccidd->user_io = ccidd->status_io = ccidd->main_io = ccidd->txd_io = NULL;
            ccidd->tx_busy = ccidd->tx_pending = false;
            status_ep_size = 0;
            for (ep = (USB_ENDPOINT_DESCRIPTOR*)usb_interface_get_first_descriptor(cfg, iface, USB_ENDPOINT_DESCRIPTOR_TYPE); ep != NULL;
                 ep = (USB_ENDPOINT_DESCRIPTOR*)usb_interface_get_next_descriptor(cfg, (USB_GENERIC_DESCRIPTOR*)ep, USB_ENDPOINT_DESCRIPTOR_TYPE))
            {
                switch (ep->bmAttributes & USB_EP_BM_ATTRIBUTES_TYPE_MASK)
                {
                case USB_EP_BM_ATTRIBUTES_BULK:
                    if (ccidd->data_ep == 0)
                    {
                        ccidd->data_ep = USB_EP_NUM(ep->bEndpointAddress);
                        ccidd->data_ep_size = ep->wMaxPacketSize;
                        ccidd->main_io = io_create(ccidd->data_ep_size >= 512 ? 512 : ((ccid_descriptor->dwMaxCCIDMessageLength - 1) & ~(ccidd->data_ep_size - 1)));
                        ccidd->txd_io = io_create(ccidd->data_ep_size >= 512 ? 512 : ((ccid_descriptor->dwMaxCCIDMessageLength + ccidd->data_ep_size - 1) & ~(ccidd->data_ep_size - 1)));
                    }
                    break;
                case USB_EP_BM_ATTRIBUTES_INTERRUPT:
                    if (ccidd->status_ep == 0)
                    {
                        ccidd->status_ep = USB_EP_NUM(ep->bEndpointAddress);
                        status_ep_size = ep->wMaxPacketSize;
                        ccidd->status_io = io_create(status_ep_size);
                    }
                    break;
                default:
                    break;
                }
            }
            //invalid configuration
            if (ccidd->data_ep == 0 || ccidd->main_io == NULL || ccidd->txd_io == NULL || (ccidd->status_ep != 0 && ccidd->status_io == NULL))
            {
#if (USBD_CCID_DEBUG_ERRORS)
                if (ccidd->data_ep)
                    printf("Out of memory for CCID\n");
#endif //USBD_CCID_DEBUG_ERRORS
                ccidd_destroy(ccidd);
                continue;
            }
#if (USBD_CCID_REMOVABLE_CARD)
            ccidd->slot_status = CCID_SLOT_STATUS_ICC_NOT_PRESENT;
#else
            ccidd->slot_status = CCID_SLOT_STATUS_ICC_PRESENT_AND_INACTIVE;
#endif //USBD_CCID_REMOVABLE_CARD

            ccidd->state = CCIDD_STATE_IDLE;
            ccidd->aborting = false;

#if (USBD_CCID_DEBUG_REQUESTS)
            printf("Found USB CCID device class, data: EP%d, iface: %d\n", ccidd->data_ep, ccidd->iface);
            if (ccidd->status_ep)
                printf("Status: EP%d\n", ccidd->status_ep);
#endif //USBD_CCID_DEBUG_REQUESTS

            usbd_register_interface(usbd, ccidd->iface, &__CCIDD_CLASS, ccidd);
            usbd_register_endpoint(usbd, ccidd->iface, ccidd->data_ep);
            usbd_usb_ep_open(usbd, ccidd->data_ep, USB_EP_BULK, ccidd->data_ep_size);
            usbd_usb_ep_open(usbd, USB_EP_IN | ccidd->data_ep, USB_EP_BULK, ccidd->data_ep_size);
            ccidd_rx(usbd, ccidd);

            if (ccidd->status_ep)
            {
                usbd_register_endpoint(usbd, ccidd->iface, ccidd->status_ep);
                ccidd->status_busy = false;
                usbd_usb_ep_open(usbd, USB_EP_IN | ccidd->status_ep, USB_EP_INTERRUPT, status_ep_size);
                ccidd_notify_slot_change(usbd, ccidd);
            }
        }
    }

}

void ccidd_class_reset(USBD* usbd, void* param)
{
    CCIDD* ccidd = (CCIDD*)param;

    usbd_usb_ep_close(usbd, ccidd->data_ep);
    usbd_usb_ep_close(usbd, USB_EP_IN | ccidd->data_ep);
    usbd_unregister_endpoint(usbd, ccidd->iface, ccidd->data_ep);
    if (ccidd->status_ep)
    {
        usbd_usb_ep_close(usbd, USB_EP_IN | ccidd->status_ep);
        usbd_unregister_endpoint(usbd, ccidd->iface, ccidd->status_ep);
    }

    if (ccidd->user_io)
        usbd_io_user(usbd, ccidd->iface, 0, HAL_IO_CMD(HAL_USBD_IFACE, IPC_READ), ccidd->user_io, ERROR_IO_CANCELLED);

    usbd_unregister_interface(usbd, ccidd->iface, &__CCIDD_CLASS);
    ccidd_destroy(ccidd);
}

void ccidd_class_suspend(USBD* usbd, void* param)
{
    CCIDD* ccidd = (CCIDD*)param;
    usbd_usb_ep_flush(usbd, ccidd->data_ep);
    usbd_usb_ep_flush(usbd, USB_EP_IN | ccidd->data_ep);
    ccidd->state = CCIDD_STATE_IDLE;
    ccidd->aborting = false;
    if (ccidd->status_ep && ccidd->status_busy)
    {
        usbd_usb_ep_flush(usbd, USB_EP_IN | ccidd->status_ep);
        ccidd->status_busy = false;
        ccidd->status_pending = false;
    }
    ccidd->tx_busy = ccidd->tx_pending = false;

    if (ccidd->user_io)
    {
        usbd_io_user(usbd, ccidd->iface, 0, HAL_IO_CMD(HAL_USBD_IFACE, IPC_READ), ccidd->user_io, ERROR_IO_CANCELLED);
        ccidd->user_io = NULL;
    }
}

void ccidd_class_resume(USBD* usbd, void* param)
{
    CCIDD* ccidd = (CCIDD*)param;
    ccidd_rx(usbd, ccidd);
    ccidd_notify_slot_change(usbd, ccidd);
}

int ccidd_class_setup(USBD* usbd, void* param, SETUP* setup, IO* io)
{
    CCIDD* ccidd = (CCIDD*)param;
    unsigned int res = -1;
    switch (setup->bRequest)
    {
    case CCID_CMD_ABORT:
        if (ccidd->state == CCIDD_STATE_REQUEST)
            usbd_post_user(usbd, ccidd->iface, 0, HAL_CMD(HAL_USBD_IFACE, IPC_CANCEL_IO), 0, 0);
        ccidd->state = CCIDD_STATE_IDLE;
        ccidd->aborting = true;
        ccidd->abort_seq = setup->wValue >> 8;
        res = 0;
#if (USBD_CCID_DEBUG_REQUESTS)
        printf("CCIDD: Abort request\n");
#endif //USBD_CCID_DEBUG_REQUESTS
        break;
    default:
        break;
    }
    return res;
}

static inline void ccidd_power_on(USBD* usbd, CCIDD* ccidd)
{
    CCID_MSG_POWER_ON* msg = io_data(ccidd->main_io);
#if (USBD_CCID_DEBUG_REQUESTS)
    printf("PC_to_RDR_IccPowerOn bPowerSelect: %d\n", msg->bPowerSelect);
#endif //USBD_CCID_DEBUG_REQUESTS
    ccidd_user_request(usbd, ccidd, USB_CCID_POWER_ON, msg->bPowerSelect);
}

static inline void ccidd_power_off(USBD* usbd, CCIDD* ccidd)
{
#if (USBD_CCID_DEBUG_REQUESTS)
    printf("PC_to_RDR_IccPowerOff\n");
#endif //USBD_CCID_DEBUG_REQUESTS
    if (ccidd->slot_status == CCID_SLOT_STATUS_ICC_PRESENT_AND_ACTIVE)
        ccidd->slot_status = CCID_SLOT_STATUS_ICC_PRESENT_AND_INACTIVE;
    ccidd_tx_slot_status(usbd, ccidd);
    usbd_post_user(usbd, ccidd->iface, 0, HAL_CMD(HAL_USBD_IFACE, USB_CCID_POWER_OFF), 0, 0);
}

static inline void ccidd_get_slot_status(USBD* usbd, CCIDD* ccidd)
{
#if (USBD_CCID_DEBUG_REQUESTS)
    printf("PC_to_RDR_GetSlotStatus\n");
#endif //USBD_CCID_DEBUG_REQUESTS
    ccidd_tx_slot_status(usbd, ccidd);
}

static void ccidd_xfer_block_process(USBD* usbd, CCIDD* ccidd)
{
#if (USBD_CCID_DEBUG_REQUESTS)
    printf("PC_to_RDR_XfrBlock, BWI: %d, size: %d\n", ccidd->bBWI, ccidd->user_io->data_size);
#endif //USBD_CCID_DEBUG_REQUESTS
#if (USBD_CCID_DEBUG_IO)
    usbd_dump(io_data(ccidd->user_io), ccidd->user_io->data_size, "CCIDD C-BLOCK");
#endif

    if(ccidd->slot_status == CCID_SLOT_STATUS_ICC_PRESENT_AND_ACTIVE)
        ccidd_user_request(usbd, ccidd, IPC_WRITE, ccidd->bBWI);
    else
    {
#if (USBD_CCID_DEBUG_ERRORS)
        printf("CCIDD: Xfer failed - card removed\n");
#endif //USBD_CCID_DEBUG_ERRORS
        ccidd_tx_data_block_ex(usbd, ccidd, ccidd->seq, CCID_SLOT_ERROR_ICC_MUTE, CCID_SLOT_STATUS_COMMAND_FAIL, ccidd->txd_io, NULL);
    }
}

static inline void ccidd_xfer_block(USBD* usbd, CCIDD* ccidd)
{
    CCID_MSG_XFER_BLOCK* msg = io_data(ccidd->main_io);
    ccidd->bBWI = msg->bBWI;
    if (msg->dwLength + sizeof(CCID_MSG) > ccidd->data_ep_size)
    {
        //more than one frame
        io_data_write(ccidd->user_io, (uint8_t*)io_data(ccidd->main_io) + sizeof(CCID_MSG), ccidd->data_ep_size - sizeof(CCID_MSG));

        ccidd->state = CCIDD_STATE_RXD;
        //some hardware required to be multiple of MPS
        usbd_usb_ep_read(usbd, ccidd->data_ep, ccidd->main_io, (msg->dwLength  + sizeof(CCID_MSG) - 1) & ~(ccidd->data_ep_size - 1));
        return;
    }
    io_data_write(ccidd->user_io, (uint8_t*)io_data(ccidd->main_io) + sizeof(CCID_MSG), msg->dwLength);
    ccidd_xfer_block_process(usbd, ccidd);
}

static inline void ccidd_get_params(USBD* usbd, CCIDD* ccidd)
{
#if (USBD_CCID_DEBUG_REQUESTS)
    printf(" PC_to_RDR_GetParameters\n");
#endif //USBD_CCID_DEBUG_REQUESTS
    ccidd_user_request(usbd, ccidd, USB_CCID_GET_PARAMS, 0);
}

static inline void ccidd_set_params(USBD* usbd, CCIDD* ccidd)
{
    CCID_MSG_SET_PARAMS* msg = io_data(ccidd->main_io);
#if (USBD_CCID_DEBUG_REQUESTS)
    printf("PC_to_RDR_SetParameters T=%d\n", msg->bProtocolNum);
#endif //USBD_CCID_DEBUG_REQUESTS
#if (USBD_CCID_DEBUG_IO)
    usbd_dump((uint8_t*)io_data(ccidd->main_io) + sizeof(CCID_MSG), msg->dwLength, "CCIDD Parameters");
#endif
    io_data_write(ccidd->user_io, (uint8_t*)io_data(ccidd->main_io) + sizeof(CCID_MSG), ccidd->data_ep_size - sizeof(CCID_MSG));
    ccidd_user_request(usbd, ccidd, USB_CCID_SET_PARAMS, msg->bProtocolNum);
}

static inline void ccidd_reset_params(USBD* usbd, CCIDD* ccidd)
{
#if (USBD_CCID_DEBUG_REQUESTS)
    printf("PC_to_RDR_ResetParameters\n");
#endif //USBD_CCID_DEBUG_REQUESTS
    ccidd_user_request(usbd, ccidd, USB_CCID_RESET_PARAMS, 0);
}

static inline void ccidd_set_data_rate_and_clock(USBD* usbd, CCIDD* ccidd)
{
#if (USBD_CCID_DEBUG_REQUESTS)
    CCID_MSG_RATE_CLOCK* rate_clock = io_data(ccidd->main_io);
    printf("PC_to_RDR_SetDataRateAndClockFrequency %d KHz, %d BPS\n", rate_clock->dwClockFrequency, rate_clock->dwDataRate);
#endif //USBD_CCID_DEBUG_REQUESTS
    io_data_write(ccidd->user_io, (uint8_t*)io_data(ccidd->main_io) + sizeof(CCID_MSG), ccidd->data_ep_size - sizeof(CCID_MSG));
    ccidd_user_request(usbd, ccidd, USB_CCID_SET_RATE_CLOCK, 0);
}

static inline void ccidd_abort(USBD* usbd, CCIDD* ccidd)
{
#if (USBD_CCID_DEBUG_REQUESTS)
    printf("PC_to_RDR_Abort\n");
#endif //USBD_CCID_DEBUG_REQUESTS
    //sequence already checked
    if (ccidd->aborting)
    {
        ccidd->aborting = false;
        ccidd_tx_slot_status(usbd, ccidd);
    }
    else
        //In specification CMD_NOT_ABORTED, but no integer value given.
        ccidd_tx_slot_status_error(usbd, ccidd, CCID_SLOT_ERROR_CMD_ABORTED);
}


static void ccidd_msg_process(USBD* usbd, CCIDD* ccidd)
{
    CCID_MSG* msg = io_data(ccidd->main_io);
    switch (msg->bMessageType)
    {
    case PC_TO_RDR_ICC_POWER_ON:
        ccidd_power_on(usbd, ccidd);
        break;
    case PC_TO_RDR_ICC_POWER_OFF:
        ccidd_power_off(usbd, ccidd);
        break;
    case PC_TO_RDR_GET_SLOT_STATUS:
        ccidd_get_slot_status(usbd, ccidd);
        break;
    case PC_TO_RDR_XFER_BLOCK:
        ccidd_xfer_block(usbd, ccidd);
        break;
    case PC_TO_RDR_GET_PARAMETERS:
        ccidd_get_params(usbd, ccidd);
        break;
    case PC_TO_RDR_RESET_PARAMETERS:
        ccidd_reset_params(usbd, ccidd);
        break;
    case PC_TO_RDR_SET_PARAMETERS:
        ccidd_set_params(usbd, ccidd);
        break;
    case PC_TO_RDR_SET_DATA_RATE_AND_CLOCK:
        ccidd_set_data_rate_and_clock(usbd, ccidd);
        break;
    case PC_TO_RDR_ABORT:
        ccidd_abort(usbd, ccidd);
        break;
    default:
#if (USBD_CCID_DEBUG_ERRORS)
        printf("CCIDD: unsupported CMD: %#X\n", msg->bMessageType);
#endif //USBD_DEBUG_CLASS_REQUESTS
        ccidd_tx_slot_status_error(usbd, ccidd, CCID_SLOT_ERROR_CMD_NOT_SUPPORTED);
    }
    if (ccidd->state != CCIDD_STATE_RXD)
        ccidd_rx(usbd, ccidd);
}

static inline void ccidd_rxc(USBD* usbd, CCIDD* ccidd)
{
    CCID_MSG* msg = io_data(ccidd->main_io);

    if (ccidd->aborting && msg->bSeq != ccidd->abort_seq)
    {
#if (USBD_CCID_DEBUG_ERRORS)
        printf("CCIDD: request aborted\n");
#endif //USBD_CCID_DEBUG_ERRORS
        ccidd_tx_error(usbd, ccidd, CCID_SLOT_ERROR_CMD_ABORTED, ccidd->state != CCIDD_STATE_IDLE, msg->bMessageType, msg->bSeq);
        return;
    }

    //command can be sent in any state
    switch (ccidd->state)
    {
    case CCIDD_STATE_IDLE:
        ccidd->seq = msg->bSeq;
        ccidd->msg_type = msg->bMessageType;
        if (ccidd->user_io)
            ccidd_msg_process(usbd, ccidd);
        else
        {
#if (USBD_CCID_DEBUG_IO)
            printf("CCIDD: warning - no buffer ready\n");
#endif //USBD_CCID_DEBUG_ERRORS
            ccidd->state = CCIDD_STATE_NO_BUFFER;
        }
        break;
    case CCIDD_STATE_RXD:
        io_data_append(ccidd->user_io, io_data(ccidd->main_io), ccidd->main_io->data_size);
        ccidd_xfer_block_process(usbd, ccidd);
        break;
    default:
#if (USBD_CCID_DEBUG_ERRORS)
        printf("CCIDD: slot busy\n");
#endif //USBD_CCID_DEBUG_ERRORS
        ccidd_tx_error(usbd, ccidd, CCID_SLOT_ERROR_SLOT_BUSY, true, msg->bMessageType, msg->bSeq);
    }
}

static void ccidd_txc(USBD* usbd, CCIDD* ccidd)
{
    switch (ccidd->state)
    {
    case CCIDD_STATE_TXD:
        if ((ccidd->txd_io->data_size % ccidd->data_ep_size) == 0)
        {
            ccidd->txd_io->data_size = 0;
            ccidd_tx(usbd, ccidd, ccidd->txd_io);
            ccidd->state = CCIDD_STATE_TXD_ZLP;
            return;
        }
        //follow down
    case CCIDD_STATE_TXD_ZLP:
        ccidd->state = CCIDD_STATE_IDLE;
        break;
    default:
        ccidd->state = CCIDD_STATE_IDLE;
#if (USBD_CCID_DEBUG_ERRORS)
        printf("CCIDD: invalid state on tx: %d\n", ccidd->state);
#endif //USBD_DEBUG_CLASS_REQUESTS
    }
}

#if (USBD_CCID_REMOVABLE_CARD)
static inline void ccidd_card_inserted(USBD* usbd, CCIDD* ccidd)
{
    if (ccidd->slot_status == CCID_SLOT_STATUS_ICC_NOT_PRESENT)
    {
        ccidd->slot_status = CCID_SLOT_STATUS_ICC_PRESENT_AND_INACTIVE;
#if (USBD_CCID_DEBUG_REQUESTS)
        printf("CCIDD: Card inserted\n");
#endif //USBD_CCID_DEBUG_REQUESTS
        ccidd_notify_slot_change(usbd, ccidd);
    }
}

static inline void ccidd_card_removed(USBD* usbd, CCIDD* ccidd)app_ccid_rate_clock
{
    if (ccidd->slot_status != CCID_SLOT_STATUS_ICC_NOT_PRESENT)
    {
        ccidd->slot_status = CCID_SLOT_STATUS_ICC_NOT_PRESENT;
#if (USBD_CCID_DEBUG_REQUESTS)
        printf("CCIDD: Card removed\n");
#endif //USBD_CCID_DEBUG_REQUESTS
        ccidd_notify_slot_change(usbd, ccidd);
    }
}
#endif //USBD_CCID_REMOVABLE_CARD

static inline void ccidd_driver_event(USBD* usbd, CCIDD* ccidd, IPC* ipc)
{
    //driver is flushing handles
    if ((int)ipc->param3 < 0)
        return;
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_READ:
        ccidd_rxc(usbd, ccidd);
        break;
    case IPC_WRITE:
        if (USB_EP_NUM(ipc->param1) == ccidd->data_ep)
        {
            ccidd->tx_busy = false;
            //busy slot response, no ZLP
            if ((IO*)ipc->param2 == ccidd->main_io)
            {
                ccidd_rx(usbd, ccidd);
                if (ccidd->tx_pending)
                {
                    ccidd->tx_pending = false;
                    ccidd_tx(usbd, ccidd, ccidd->txd_io);
                }
            }
            else
            {
                if (ccidd->tx_pending)
                {
                    ccidd->tx_pending = false;
                    ccidd_tx(usbd, ccidd, ccidd->main_io);
                }
                ccidd_txc(usbd, ccidd);
            }
        }
        //status notify complete
        else
        {
            ccidd->status_busy = false;
            if (ccidd->status_pending)
            {
                ccidd->status_pending = false;
                ccidd_notify_slot_change(usbd, ccidd);
            }
        }
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
    }
}

static inline void ccidd_user_response(USBD* usbd, CCIDD* ccidd, IPC* ipc)
{
    ccidd->user_io = (IO*)ipc->param2;
    if (ccidd->aborting)
    {
#if (USBD_DEBUG_ERRORS)
        printf("CCIDD: ABORT on user response\n");
#endif //USBD_DEBUG_ERRORS
        ccidd_tx_error(usbd, ccidd, CCID_SLOT_ERROR_CMD_ABORTED, false, ccidd->msg_type, ccidd->seq);
        return;
    }
    if ((int)ipc->param3 < 0)
    {
        switch ((int)ipc->param3)
        {
        case ERROR_IO_CANCELLED:
            ccidd_tx_error(usbd, ccidd, CCID_SLOT_ERROR_CMD_ABORTED, false, ccidd->msg_type, ccidd->seq);
            break;
        case ERROR_TIMEOUT:
            ccidd_tx_error(usbd, ccidd, CCID_SLOT_ERROR_ICC_MUTE, false, ccidd->msg_type, ccidd->seq);
            break;
        default:
            ccidd_tx_error(usbd, ccidd, CCID_SLOT_ERROR_HW_ERROR, false, ccidd->msg_type, ccidd->seq);
        }
        return;
    }

    switch (HAL_ITEM(ipc->cmd))
    {
    case USB_CCID_POWER_ON:
        ccidd->slot_status = CCID_SLOT_STATUS_ICC_PRESENT_AND_ACTIVE;
        //follow down
    case IPC_WRITE:
        ccidd_tx_data_block(usbd, ccidd);
        break;
    case USB_CCID_GET_PARAMS:
    case USB_CCID_SET_PARAMS:
    case USB_CCID_RESET_PARAMS:
        ccidd_tx_params(usbd, ccidd, (CCID_PROTOCOL)ipc->param3);
        break;
    case USB_CCID_SET_RATE_CLOCK:
        ccidd_tx_rate_clock(usbd, ccidd, ((CCID_RATE_CLOCK*)io_data(ccidd->user_io))->dwClockFrequency, ((CCID_RATE_CLOCK*)io_data(ccidd->user_io))->dwDataRate);
        break;
    }
}

static inline void ccidd_user_buffer(USBD* usbd, CCIDD* ccidd, IO* io)
{
    if (ccidd->user_io || ccidd->state == CCIDD_STATE_REQUEST)
    {
        error(ERROR_ALREADY_CONFIGURED);
        return;
    }
    ccidd->user_io = io;
    if (ccidd->state == CCIDD_STATE_NO_BUFFER)
        ccidd_msg_process(usbd, ccidd);
    error(ERROR_SYNC);
}

void ccidd_class_request(USBD* usbd, void* param, IPC* ipc)
{
    CCIDD* ccidd = (CCIDD*)param;
    if (HAL_GROUP(ipc->cmd) == HAL_USB)
        ccidd_driver_event(usbd, ccidd, ipc);
    else
        switch (HAL_ITEM(ipc->cmd))
        {
#if (USBD_CCID_REMOVABLE_CARD)
        case USB_CCID_CARD_INSERTED:
            ccidd_card_inserted(usbd, ccidd);
            break;
        case USB_CCID_CARD_REMOVED:
            ccidd_card_removed(usbd, ccidd);
            break;
#endif //USBD_CCID_REMOVABLE_CARD
        case USB_CCID_POWER_ON:
        case USB_CCID_GET_PARAMS:
        case USB_CCID_SET_PARAMS:
        case USB_CCID_RESET_PARAMS:
        case USB_CCID_SET_RATE_CLOCK:
        case IPC_WRITE:
            ccidd_user_response(usbd, ccidd, ipc);
            break;
        case IPC_READ:
            ccidd_user_buffer(usbd, ccidd, (IO*)ipc->param2);
            break;
        default:
            error(ERROR_NOT_SUPPORTED);
        }
}

const USBD_CLASS __CCIDD_CLASS = {
    ccidd_class_configured,
    ccidd_class_reset,
    ccidd_class_suspend,
    ccidd_class_resume,
    ccidd_class_setup,
    ccidd_class_request,
};
