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
    CCIDD_STATE_RX,
    CCIDD_STATE_CARD_REQUEST,
    CCIDD_STATE_TX,
#if (USBD_CCID_WTX_TIMEOUT_MS)
    CCIDD_STATE_WTX,
#endif // USBD_CCID_WTX_TIMEOUT_MS
    CCIDD_STATE_TX_ZLP
} CCIDD_STATE;

typedef struct {
    IO* io;
    IO* status_io;
#if(USBD_CCID_WTX_TIMEOUT_MS)
    IO* wtx_io;
    HANDLE wtx_timer;
#endif // USBD_CCID_WTX_TIMEOUT_MS
    CCIDD_STATE state;
    uint16_t data_ep_size;
    uint8_t data_ep, status_ep, iface, seq, status_busy, slot_status, aborting;
} CCIDD;

static void ccidd_destroy(CCIDD* ccidd)
{
#if (USBD_CCID_WTX_TIMEOUT_MS)
    timer_stop(ccidd->wtx_timer, USBD_IFACE(ccidd->iface, 0), HAL_USBD_IFACE);
    timer_destroy(ccidd->wtx_timer);
    io_destroy(ccidd->wtx_io);
#endif // USBD_CCID_WTX_TIMEOUT_MS
    io_destroy(ccidd->io);
    io_destroy(ccidd->status_io);
    free(ccidd);
}

static void ccidd_rx(USBD* usbd, CCIDD* ccidd)
{
    io_reset(ccidd->io);
    //first frame going with data size
    usbd_usb_ep_read(usbd, ccidd->data_ep, ccidd->io, ccidd->data_ep_size);
}

static void ccidd_notify_slot_change(USBD* usbd, CCIDD* ccidd)
{
    CCID_NOTIFY_SLOT_CHANGE* notify;
    if (ccidd->status_ep)
    {
        if (ccidd->status_busy)
            usbd_usb_ep_flush(usbd, USB_EP_IN | ccidd->status_ep);

        notify = io_data(ccidd->status_io);

        notify->bMessageType = RDR_TO_PC_NOTIFY_SLOT_CHANGE;
        notify->bmSlotICCState = CCID_SLOT_ICC_STATE_CHANGED;

        if (ccidd->slot_status != CCID_SLOT_STATUS_ICC_NOT_PRESENT)
            notify->bmSlotICCState |= CCID_SLOT_STATUS_ICC_PRESENT_AND_INACTIVE;

#if (USBD_CCID_DEBUG_REQUESTS)
    printf("CCIDD: Slot Status %02X %02X\n", notify->bMessageType, notify->bmSlotICCState);
#endif //USBD_CCID_DEBUG_REQUESTS

        ccidd->status_io->data_size = sizeof(CCID_NOTIFY_SLOT_CHANGE);
        usbd_usb_ep_write(usbd, USB_EP_IN | ccidd->status_ep, ccidd->status_io);
        ccidd->status_busy = true;
    }
}

static inline uint8_t ccidd_slot_status_register(CCIDD* ccidd, unsigned int command_status)
{
    return ((ccidd->slot_status & 3) << 0) | (command_status & 3) << 6;
}

static void ccidd_send_slot_status(USBD* usbd, CCIDD* ccidd, uint8_t seq, uint8_t error, uint8_t status)
{
    io_reset(ccidd->io);
    CCID_SLOT_STATUS* msg = io_data(ccidd->io);
    msg->bMessageType = RDR_TO_PC_SLOT_STATUS;
    msg->dwLength = 0;
    msg->bSlot = 0;
    msg->bSeq = seq;
    msg->bStatus = ccidd_slot_status_register(ccidd, status);
    msg->bError = error;
    msg->bClockStatus = CCID_CLOCK_STATUS_RUNNING;
    ccidd->io->data_size = sizeof(CCID_SLOT_STATUS);
    usbd_usb_ep_write(usbd, USB_EP_IN | ccidd->data_ep, ccidd->io);
    ccidd->state = CCIDD_STATE_TX;
}

static void ccidd_send_data_block(USBD* usbd, CCIDD* ccidd, uint8_t error, uint8_t status)
{
    CCID_DATA_BLOCK* msg = io_data(ccidd->io);
    if (error)
        ccidd->io->data_size = sizeof(CCID_MESSAGE);
    msg->bMessageType = RDR_TO_PC_DATA_BLOCK;
    msg->dwLength = ccidd->io->data_size - sizeof(CCID_MESSAGE);
    msg->bSlot = 0;
    msg->bSeq = ccidd->seq;
    msg->bStatus = ccidd_slot_status_register(ccidd, status);
    msg->bError = error;
    msg->bChainParameter = 0;
    usbd_usb_ep_write(usbd, USB_EP_IN | ccidd->data_ep, ccidd->io);
    ccidd->state = CCIDD_STATE_TX;
}

#if (USBD_CCID_WTX_TIMEOUT_MS)
static void ccidd_send_wtx(USBD* usbd, CCIDD* ccidd, uint8_t seq, uint8_t timeout)
{
    io_reset(ccidd->wtx_io);
    CCID_SLOT_STATUS* msg = io_data(ccidd->wtx_io);
    msg->bMessageType = RDR_TO_PC_SLOT_STATUS;
    msg->dwLength = 0;
    msg->bSlot = 0;
    msg->bSeq = seq;
    msg->bStatus = ccidd_slot_status_register(ccidd, CCID_SLOT_STATUS_COMMAND_TIME_EXTENSION);
    msg->bError = timeout;
    msg->bClockStatus = CCID_CLOCK_STATUS_RUNNING;
    ccidd->wtx_io->data_size = sizeof(CCID_SLOT_STATUS);
    usbd_usb_ep_write(usbd, USB_EP_IN | ccidd->data_ep, ccidd->wtx_io);
    ccidd->state = CCIDD_STATE_WTX;
}
#endif // USBD_CCID_WTX_TIMEOUT_MS

static void ccidd_send_params(USBD* usbd, CCIDD* ccidd, uint8_t error, uint8_t status, CCID_PROTOCOL protocol)
{
    CCID_PARAMS* msg = io_data(ccidd->io);
    if (error)
        ccidd->io->data_size = sizeof(CCID_MESSAGE);
    msg->bMessageType = RDR_TO_PC_PARAMETERS;
    msg->dwLength = ccidd->io->data_size - sizeof(CCID_MESSAGE);
    msg->bSlot = 0;
    msg->bSeq = ccidd->seq;
    msg->bStatus = ccidd_slot_status_register(ccidd, status);
    msg->bError = error;
    msg->bProtocolNum = protocol;
    usbd_usb_ep_write(usbd, USB_EP_IN | ccidd->data_ep, ccidd->io);
    ccidd->state = CCIDD_STATE_TX;
}

static void ccidd_user_request(USBD* usbd, CCIDD* ccidd, unsigned int req, uint8_t param)
{
    //hide ccid message to user
    io_hide(ccidd->io, sizeof(CCID_MESSAGE));
    usbd_io_user(usbd, ccidd->iface, 0, HAL_IO_REQ(HAL_USBD_IFACE, req), ccidd->io, param);
    ccidd->state = CCIDD_STATE_CARD_REQUEST;
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
            ccidd->io = ccidd->status_io = NULL;
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
                        ccidd->io = io_create(ccid_descriptor->dwMaxCCIDMessageLength + sizeof(CCID_MESSAGE));
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
            if (ccidd->data_ep == 0 || ccidd->io == NULL || (ccidd->status_ep != 0 && ccidd->status_io == NULL))
            {
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

#if (USBD_CCID_WTX_TIMEOUT_MS)
            ccidd->wtx_io = io_create(sizeof(CCID_SLOT_STATUS));
            ccidd->wtx_timer = timer_create(USBD_IFACE(ccidd->iface, 0), HAL_USBD_IFACE);
            if(ccidd->wtx_io == NULL || ccidd->wtx_timer == INVALID_HANDLE)
            {
                free(ccidd);
                continue;
            }
#endif // USBD_CCID_WTX_TIMEOUT_MS

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
        if (ccidd->state == CCIDD_STATE_CARD_REQUEST)
            usbd_post_user(usbd, ccidd->iface, 0, HAL_CMD(HAL_USBD_IFACE, IPC_CANCEL_IO), 0, 0);
        ccidd->state = CCIDD_STATE_IDLE;
        ccidd->aborting = true;
        ccidd->seq = setup->wValue >> 8;
        res = 0;
#if (USBD_CCID_DEBUG_REQUESTS)
        printf("CCIDD Abort request\n");
#endif //USBD_CCID_DEBUG_REQUESTS
        break;
    default:
        break;
    }
    return res;
}

static inline void ccidd_power_on(USBD* usbd, CCIDD* ccidd)
{
    CCID_MESSAGE* msg = io_data(ccidd->io);
#if (USBD_CCID_DEBUG_REQUESTS)
    printf("CCIDD: ICC slot%d power on\n", msg->bSlot);
#endif //USBD_CCID_DEBUG_REQUESTS
    ccidd_user_request(usbd, ccidd, USB_CCID_POWER_ON, msg->msg_specific[0]);
}

static inline void ccidd_power_off(USBD* usbd, CCIDD* ccidd)
{
#if (USBD_CCID_DEBUG_REQUESTS)
    CCID_MESSAGE* msg = io_data(ccidd->io);
    printf("CCIDD: ICC slot%d power off\n", msg->bSlot);
#endif //USBD_CCID_DEBUG_REQUESTS
    if (ccidd->slot_status == CCID_SLOT_STATUS_ICC_PRESENT_AND_ACTIVE)
        ccidd->slot_status = CCID_SLOT_STATUS_ICC_PRESENT_AND_INACTIVE;
    ccidd_send_slot_status(usbd, ccidd, ccidd->seq, 0, CCID_SLOT_STATUS_COMMAND_NO_ERROR);
    usbd_post_user(usbd, ccidd->iface, 0, HAL_CMD(HAL_USBD_IFACE, USB_CCID_POWER_OFF), 0, 0);
}

static inline void ccidd_get_slot_status(USBD* usbd, CCIDD* ccidd)
{
#if (USBD_CCID_DEBUG_REQUESTS)
    CCID_MESSAGE* msg = io_data(ccidd->io);
    printf("CCIDD: get slot%d status\n", msg->bSlot);
#endif //USBD_CCID_DEBUG_REQUESTS
    ccidd_send_slot_status(usbd, ccidd, ccidd->seq, 0, CCID_SLOT_STATUS_COMMAND_NO_ERROR);
}

static inline void ccidd_xfer_block(USBD* usbd, CCIDD* ccidd)
{
    CCID_MESSAGE* msg = io_data(ccidd->io);
#if (USBD_CCID_DEBUG_REQUESTS)
    printf("CCIDD: Xfer block to slot%d, size: %d\n", msg->bSlot, msg->dwLength);
#endif //USBD_CCID_DEBUG_REQUESTS
#if (USBD_CCID_DEBUG_IO)
    usbd_dump(io_data(ccidd->io) + sizeof(CCID_MESSAGE), msg->dwLength, "CCIDD C-APDU");
#endif

    if(ccidd->slot_status == CCID_SLOT_STATUS_ICC_PRESENT_AND_ACTIVE)
    {
#if (USBD_CCID_WTX_TIMEOUT_MS)
        timer_start_ms(ccidd->wtx_timer, USBD_CCID_WTX_TIMEOUT_MS);
#endif
        ccidd_user_request(usbd, ccidd, USB_CCID_APDU, msg->msg_specific[0]);
    }
    else
        ccidd_send_data_block(usbd, ccidd, CCID_SLOT_ERROR_XFR_OVERRUN, CCID_SLOT_STATUS_COMMAND_FAIL);
}

static inline void ccidd_get_params(USBD* usbd, CCIDD* ccidd)
{
#if (USBD_CCID_DEBUG_REQUESTS)
    printf("CCIDD: get params\n");
#endif //USBD_CCID_DEBUG_REQUESTS
    ccidd_user_request(usbd, ccidd, USB_CCID_GET_PARAMS, 0);
}

static inline void ccidd_set_params(USBD* usbd, CCIDD* ccidd)
{
    CCID_MESSAGE* msg = io_data(ccidd->io);
#if (USBD_CCID_DEBUG_REQUESTS)
    printf("CCIDD: set params - T%d\n", msg->msg_specific[0]);
#endif //USBD_CCID_DEBUG_REQUESTS
    ccidd_user_request(usbd, ccidd, USB_CCID_SET_PARAMS, msg->msg_specific[0]);
}

static inline void ccidd_reset_params(USBD* usbd, CCIDD* ccidd)
{
#if (USBD_CCID_DEBUG_REQUESTS)
        printf("CCIDD: reset params\n");
#endif //USBD_CCID_DEBUG_REQUESTS
    ccidd_user_request(usbd, ccidd, USB_CCID_RESET_PARAMS, 0);
}

static inline void ccidd_msg_process(USBD* usbd, CCIDD* ccidd)
{
    CCID_MESSAGE* msg = io_data(ccidd->io);
    ccidd->seq = msg->bSeq;
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
    default:
        ccidd_send_slot_status(usbd, ccidd, msg->bSeq, CCID_SLOT_ERROR_CMD_NOT_SUPPORTED, CCID_SLOT_STATUS_COMMAND_FAIL);
#if (USBD_CCID_DEBUG_ERRORS)
        printf("CCIDD: unsupported CMD: %#X\n", msg->bMessageType);
#endif //USBD_DEBUG_CLASS_REQUESTS
    }
}

static inline void ccidd_abort(USBD* usbd, CCIDD* ccidd)
{
    io_reset(ccidd->io);
    CCID_MESSAGE* msg = io_data(ccidd->io);
    if (msg->bSeq == ccidd->seq && msg->bMessageType == PC_TO_RDR_ABORT)
    {
        ccidd->aborting = false;
        ccidd_send_slot_status(usbd, ccidd, msg->bSeq, 0, CCID_SLOT_STATUS_COMMAND_NO_ERROR);
    }
    else
        ccidd_send_slot_status(usbd, ccidd, msg->bSeq, CCID_SLOT_ERROR_CMD_ABORTED, CCID_SLOT_STATUS_COMMAND_FAIL);
}

static inline void ccidd_rx_complete(USBD* usbd, CCIDD* ccidd)
{
    unsigned int len;
    io_show(ccidd->io);
    switch (ccidd->state)
    {
    case CCIDD_STATE_IDLE:
        if (ccidd->io->data_size >= sizeof(CCID_MESSAGE))
        {
            len = ((CCID_MESSAGE*)io_data(ccidd->io))->dwLength;
            //more than one frame
            if (len  + sizeof(CCID_MESSAGE) > ccidd->data_ep_size)
            {
                io_hide(ccidd->io, ccidd->data_ep_size);
                ccidd->state = CCIDD_STATE_RX;
                //some hardware required to be multiple of MPS
                usbd_usb_ep_read(usbd, ccidd->data_ep, ccidd->io, (len  + sizeof(CCID_MESSAGE) - 1) & ~(ccidd->data_ep_size - 1));
            }
            else
                ccidd_msg_process(usbd, ccidd);
        }
        else
            ccidd_rx(usbd, ccidd);
        break;
    case CCIDD_STATE_RX:
        ccidd_msg_process(usbd, ccidd);
        break;
    default:
        ccidd->state = CCIDD_STATE_IDLE;
        ccidd_rx(usbd, ccidd);
#if (USBD_CCID_DEBUG_ERRORS)
        printf("CCIDD: invalid state on rx: %d\n", ccidd->state);
#endif //USBD_DEBUG_CLASS_REQUESTS
    }
}

static void ccidd_tx_complete(USBD* usbd, CCIDD* ccidd)
{
    switch (ccidd->state)
    {
#if (USBD_CCID_WTX_TIMEOUT_MS)
    case CCIDD_STATE_WTX:
        ccidd->state = CCIDD_STATE_IDLE;
        return;
#endif // USBD_CCID_WTX_TIMEOUT_MS
    case CCIDD_STATE_TX:
        if ((ccidd->io->data_size % ccidd->data_ep_size) == 0)
        {
            ccidd->io->data_size = 0;
            usbd_usb_ep_write(usbd, ccidd->data_ep, ccidd->io);
            ccidd->state = CCIDD_STATE_TX_ZLP;
            return;
        }
        //follow down
    case CCIDD_STATE_TX_ZLP:
        ccidd->state = CCIDD_STATE_IDLE;
        break;
    default:
        ccidd->state = CCIDD_STATE_IDLE;
#if (USBD_CCID_DEBUG_ERRORS)
        printf("CCIDD: invalid state on tx: %d\n", ccidd->state);
#endif //USBD_DEBUG_CLASS_REQUESTS
    }
    ccidd_rx(usbd, ccidd);
}

#if (USBD_CCID_REMOVABLE_CARD)
static inline void ccidd_card_inserted(USBD* usbd, CCIDD* ccidd)
{
    if (ccidd->slot_status == CCID_SLOT_STATUS_ICC_NOT_PRESENT)
    {
        ccidd->slot_status = CCID_SLOT_STATUS_ICC_PRESENT_AND_INACTIVE;
        ccidd_notify_slot_change(usbd, ccidd);
    }
}

static inline void ccidd_card_removed(USBD* usbd, CCIDD* ccidd)
{
    if (ccidd->slot_status != CCID_SLOT_STATUS_ICC_NOT_PRESENT)
    {
        ccidd->slot_status = CCID_SLOT_STATUS_ICC_NOT_PRESENT;
        ccidd_notify_slot_change(usbd, ccidd);
    }
}
#endif //USBD_CCID_REMOVABLE_CARD

#if (USBD_CCID_WTX_TIMEOUT_MS)
static inline void ccidd_card_wtx(USBD* usbd, CCIDD* ccidd)
{
#if (USBD_CCID_DEBUG_REQUESTS)
    printf("CCIDD: WTX\n");
#endif //USBD_CCID_DEBUG_REQUESTS
    ccidd_send_wtx(usbd, ccidd, ccidd->seq, 0x10);
    timer_start_ms(ccidd->wtx_timer, USBD_CCID_WTX_TIMEOUT_MS);
}
#endif // USBD_CCID_WTX_TIMEOUT_MS

static inline void ccidd_power_on_response(USBD* usbd, CCIDD* ccidd, int param3)
{
    if (param3 < 0)
    {
        switch (param3)
        {
        case ERROR_HARDWARE:
            ccidd_send_data_block(usbd, ccidd, CCID_SLOT_ERROR_HW_ERROR, CCID_SLOT_STATUS_COMMAND_FAIL);
            break;
        default:
            ccidd_send_data_block(usbd, ccidd, CCID_SLOT_ERROR_CMD_NOT_SUPPORTED, CCID_SLOT_STATUS_COMMAND_FAIL);
        }
        return;
    }
    ccidd->slot_status = CCID_SLOT_STATUS_ICC_PRESENT_AND_ACTIVE;
    ccidd_send_data_block(usbd, ccidd, 0, CCID_SLOT_STATUS_COMMAND_NO_ERROR);
}

static inline void ccidd_data_block_response(USBD* usbd, CCIDD* ccidd, int param3)
{
    if (param3 < 0)
    {
        switch (param3)
        {
        case ERROR_HARDWARE:
            ccidd_send_data_block(usbd, ccidd, CCID_SLOT_ERROR_HW_ERROR, CCID_SLOT_STATUS_COMMAND_FAIL);
            break;
        default:
            ccidd_send_data_block(usbd, ccidd, CCID_SLOT_ERROR_CMD_NOT_SUPPORTED, CCID_SLOT_STATUS_COMMAND_FAIL);
        }
        return;
    }
#if (USBD_CCID_DEBUG_IO)
    usbd_dump(io_data(ccidd->io) + sizeof(CCID_MESSAGE), ccidd->io->data_size - sizeof(CCID_MESSAGE), "CCIDD R-APDU");
#endif
    ccidd_send_data_block(usbd, ccidd, 0, CCID_SLOT_STATUS_COMMAND_NO_ERROR);
}

static inline void ccidd_params_response(USBD* usbd, CCIDD* ccidd, int param3)
{
    if (param3 < 0)
    {
        switch (param3)
        {
        case ERROR_HARDWARE:
            ccidd_send_params(usbd, ccidd, CCID_SLOT_ERROR_HW_ERROR, CCID_SLOT_STATUS_COMMAND_FAIL, CCID_T_1);
            break;
        default:
            ccidd_send_params(usbd, ccidd, CCID_SLOT_ERROR_CMD_NOT_SUPPORTED, CCID_SLOT_STATUS_COMMAND_FAIL, CCID_T_1);
        }
        return;
    }
    ccidd_send_params(usbd, ccidd, 0, CCID_SLOT_STATUS_COMMAND_NO_ERROR, (CCID_PROTOCOL)param3);
}

static inline void ccidd_driver_event(USBD* usbd, CCIDD* ccidd, IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_READ:
        if (ccidd->aborting)
            ccidd_abort(usbd, ccidd);
        else
            ccidd_rx_complete(usbd, ccidd);
        break;
    case IPC_WRITE:
        if (USB_EP_NUM(ipc->param1) == ccidd->data_ep)
            ccidd_tx_complete(usbd, ccidd);
        //status notify complete
        else
            ccidd->status_busy = false;
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
    }
}

static inline void ccidd_user_response(USBD* usbd, CCIDD* ccidd, IPC* ipc)
{
    //ignore on abort
    if (ccidd->aborting)
        return;
    io_show(ccidd->io);
    switch (HAL_ITEM(ipc->cmd))
    {
    case USB_CCID_POWER_ON:
        ccidd_power_on_response(usbd, ccidd, ipc->param3);
        break;
    case USB_CCID_APDU:
#if (USBD_CCID_WTX_TIMEOUT_MS)
        timer_stop(ccidd->wtx_timer, USBD_IFACE(ccidd->iface, 0), HAL_USBD_IFACE);
#endif // USBD_CCID_WTX_ENABLE
        ccidd_data_block_response(usbd, ccidd, ipc->param3);
        break;
    case USB_CCID_GET_PARAMS:
    case USB_CCID_SET_PARAMS:
    case USB_CCID_RESET_PARAMS:
        ccidd_params_response(usbd, ccidd, ipc->param3);
        break;
    }
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
#if (USBD_CCID_WTX_TIMEOUT_MS)
        case IPC_TIMEOUT:
            ccidd_card_wtx(usbd, ccidd);
            break;
#endif
        case USB_CCID_POWER_ON:
        case USB_CCID_POWER_OFF:
        case USB_CCID_GET_PARAMS:
        case USB_CCID_SET_PARAMS:
        case USB_CCID_RESET_PARAMS:
        case USB_CCID_APDU:
            ccidd_user_response(usbd, ccidd, ipc);
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
