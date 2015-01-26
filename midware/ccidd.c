#include "ccidd.h"
#include "../userspace/ccid.h"
#include "../userspace/usb.h"
#include "../userspace/stdlib.h"
#include "../userspace/file.h"
#include "../userspace/stdio.h"
#include "../userspace/block.h"
#include "../userspace/direct.h"
#include "../userspace/sys.h"
#include <string.h>
#include "sys_config.h"

typedef enum {
    CCIDD_STATE_NO_CARD,
    CCIDD_STATE_CARD_INSERTED,
    CCIDD_STATE_CARD_POWERING_ON,
    CCIDD_STATE_CARD_POWERED,
    CCIDD_STATE_CARD_POWERING_OFF,
    CCIDD_STATE_ABORTING,
    CCIDD_STATE_READY,
    CCIDD_STATE_RX,
    CCIDD_STATE_SC_REQUEST,
    CCIDD_STATE_TX,
    CCIDD_STATE_TX_ZLP,
    CCIDD_STATE_HW_ERROR,
    CCIDD_STATE_PARAMS_REQUEST
} CCIDD_STATE;

typedef struct {
    HANDLE usb_data_block, usb_status_block, data_block;
    uint8_t* data_buf;
    unsigned int data_block_size, data_processed, rx_size;
    CCID_MESSAGE* msg;
    CCIDD_STATE state;
    uint8_t params[sizeof(CCID_T1_PARAMS)];
    uint8_t atr[USBD_CCID_MAX_ATR_SIZE];
    uint8_t atr_size, data_ep, status_ep, iface, seq, data_ep_size;
    uint8_t suspended, status_busy, protocol, rx_more;
} CCIDD;

static void ccidd_destroy(CCIDD* ccidd)
{
    block_destroy(ccidd->usb_data_block);
    block_destroy(ccidd->usb_status_block);
    free(ccidd);
}

void ccidd_class_configured(USBD* usbd, USB_CONFIGURATION_DESCRIPTOR_TYPE* cfg)
{
    USB_INTERFACE_DESCRIPTOR_TYPE* iface;
    USB_ENDPOINT_DESCRIPTOR_TYPE* ep;
    unsigned int size, status_ep_size;

    for (iface = usb_get_first_interface(cfg); iface != NULL; iface = usb_get_next_interface(cfg, iface))
    {
        if (iface->bInterfaceClass == CCID_INTERFACE_CLASS)
        {
            CCIDD* ccidd = (CCIDD*)malloc(sizeof(CCIDD));
            if (ccidd == NULL)
                return;
            ccidd->iface = iface->bInterfaceNumber;

            ccidd->data_ep = 0;
            ccidd->status_ep = 0;
            status_ep_size = 0;
            for (ep = (USB_ENDPOINT_DESCRIPTOR_TYPE*)usb_interface_get_first_descriptor(cfg, iface, USB_ENDPOINT_DESCRIPTOR_INDEX); ep != NULL;
                 ep = (USB_ENDPOINT_DESCRIPTOR_TYPE*)usb_interface_get_next_descriptor(cfg, (USB_DESCRIPTOR_TYPE*)ep, USB_ENDPOINT_DESCRIPTOR_INDEX))
            {
                switch (ep->bmAttributes & USB_EP_BM_ATTRIBUTES_TYPE_MASK)
                {
                case USB_EP_BM_ATTRIBUTES_BULK:
                    if (ccidd->data_ep == 0)
                    {
                        ccidd->data_ep = USB_EP_NUM(ep->bEndpointAddress);
                        ccidd->data_ep_size = ep->wMaxPacketSize;
                    }
                    break;
                case USB_EP_BM_ATTRIBUTES_INTERRUPT:
                    if (ccidd->status_ep == 0)
                    {
                        ccidd->status_ep = USB_EP_NUM(ep->bEndpointAddress);
                        status_ep_size = ep->wMaxPacketSize;
                    }
                    break;
                default:
                    break;
                }
            }
            //invalid configuration
            if (ccidd->data_ep == 0)
            {
                free(ccidd);
                continue;
            }

            ccidd->usb_data_block = block_create(ccidd->data_ep_size);
            if (ccidd->status_ep)
                ccidd->usb_status_block = block_create(status_ep_size);
            else
                ccidd->usb_status_block = INVALID_HANDLE;
            if (ccidd->usb_data_block == INVALID_HANDLE || ccidd->data_ep == 0 || (ccidd->status_ep && ccidd->usb_status_block == INVALID_HANDLE))
            {
                ccidd_destroy(ccidd);
                return;
            }
            ccidd->suspended = false;
            ccidd->state = CCIDD_STATE_NO_CARD;
            ccidd->seq = 0;
            ccidd->data_block = INVALID_HANDLE;
            ccidd->data_block_size = 0;
            ccidd->data_processed = 0;
            ccidd->rx_size = 0;
            ccidd->status_busy = false;
            ccidd->protocol = CCID_T_MAX;

#if (USBD_CCID_DEBUG_REQUESTS)
            printf("Found USB CCID device class, data: EP%d, iface: %d\n\r", ccidd->data_ep, ccidd->iface);
#endif //USBD_CCID_DEBUG_REQUESTS

            size = ccidd->data_ep_size;
            fopen_p(usbd_usb(usbd), HAL_HANDLE(HAL_USB, USB_EP_IN | ccidd->data_ep), USB_EP_BULK, (void*)size);
            fopen_p(usbd_usb(usbd), HAL_HANDLE(HAL_USB, ccidd->data_ep), USB_EP_BULK, (void*)size);
            usbd_register_interface(usbd, ccidd->iface, &__CCIDD_CLASS, ccidd);
            usbd_register_endpoint(usbd, ccidd->iface, ccidd->data_ep);

            if (ccidd->status_ep)
            {
                fopen_p(usbd_usb(usbd), HAL_HANDLE(HAL_USB, USB_EP_IN | ccidd->status_ep), USB_EP_INTERRUPT, (void*)status_ep_size);
                usbd_register_endpoint(usbd, ccidd->iface, ccidd->status_ep);
            }
            fread_async(usbd_usb(usbd), HAL_HANDLE(HAL_USB, ccidd->data_ep), ccidd->usb_data_block, ccidd->data_ep_size);
        }
    }
}

void ccidd_class_reset(USBD* usbd, void* param)
{
    CCIDD* ccidd = (CCIDD*)param;

    if (ccidd->data_block != INVALID_HANDLE)
        block_send(ccidd->data_block, usbd_user(usbd));

    fclose(usbd_usb(usbd), HAL_HANDLE(HAL_USB, USB_EP_IN | ccidd->data_ep));
    fclose(usbd_usb(usbd), HAL_HANDLE(HAL_USB, ccidd->data_ep));
    if (ccidd->status_ep)
    {
        fclose(usbd_usb(usbd), HAL_HANDLE(HAL_USB, USB_EP_IN | ccidd->status_ep));
        usbd_unregister_endpoint(usbd, ccidd->iface, ccidd->status_ep);
    }

    usbd_unregister_interface(usbd, ccidd->iface, &__CCIDD_CLASS);
    ccidd_destroy(ccidd);
}

static void ccidd_notify_state_change(USBD* usbd, CCIDD* ccidd)
{
    if (ccidd->status_ep && !ccidd->status_busy)
    {
        uint8_t* buf = block_open(ccidd->usb_status_block);
        //bMessageType
        buf[0] = RDR_TO_PC_NOTIFY_SLOT_CHANGE;
        //changed status
        buf[1] = 1 << 1;
        //card present
        if (ccidd->state != CCIDD_STATE_NO_CARD)
            buf[1] |= 1 << 0;
        fwrite_async(usbd_usb(usbd), HAL_HANDLE(HAL_USB, USB_EP_IN | ccidd->status_ep), ccidd->usb_status_block, 2);
        ccidd->status_busy = true;
    }
}

void ccidd_class_suspend(USBD* usbd, void* param)
{
    CCIDD* ccidd = (CCIDD*)param;
    if (ccidd->data_block != INVALID_HANDLE)
        block_send(ccidd->data_block, usbd_user(usbd));

    fflush(usbd_usb(usbd), HAL_HANDLE(HAL_USB, USB_EP_IN | ccidd->data_ep));
    fflush(usbd_usb(usbd), HAL_HANDLE(HAL_USB, ccidd->data_ep));
    if (ccidd->status_ep)
        fflush(usbd_usb(usbd), HAL_HANDLE(HAL_USB, USB_EP_IN | ccidd->status_ep));
    ccidd->suspended = true;
}

void ccidd_class_resume(USBD* usbd, void* param)
{
    CCIDD* ccidd = (CCIDD*)param;
    ccidd->suspended = false;
    if (ccidd->state != CCIDD_STATE_CARD_POWERED)
        fread_async(usbd_usb(usbd), HAL_HANDLE(HAL_USB, ccidd->data_ep), ccidd->usb_data_block, ccidd->data_ep_size);
    ccidd_notify_state_change(usbd, ccidd);
}

int ccidd_class_setup(USBD* usbd, void* param, SETUP* setup, HANDLE block)
{
    CCIDD* ccidd = (CCIDD*)param;
    unsigned int res = -1;
    switch (setup->bRequest)
    {
    case CCID_CMD_ABORT:
        switch (ccidd->state)
        {
        case CCIDD_STATE_SC_REQUEST:
        case CCIDD_STATE_CARD_POWERING_ON:
        case CCIDD_STATE_CARD_POWERING_OFF:
        case CCIDD_STATE_PARAMS_REQUEST:
            ccidd->state = CCIDD_STATE_ABORTING;
            break;
        default:
            break;
        }
        res = 0;
        break;
    default:
#if (USBD_CCID_DEBUG_REQUESTS)
        printf("CCIDD SETUP request\n\r");
#endif //USBD_CCID_DEBUG_REQUESTS
        break;
    }
    return res;
}

static void ccidd_tx(USBD* usbd, CCIDD* ccidd, unsigned int err, unsigned int size)
{
    //bStatus
    ccidd->msg->msg_specific[0] = CCID_SLOT_STATUS_NO_ERROR;
    if (err)
        ccidd->msg->msg_specific[0] |= 1 << 6;
    switch (ccidd->state)
    {
    case CCIDD_STATE_NO_CARD:
        ccidd->msg->msg_specific[0] |= CCID_SLOT_STATUS_ICC_NOT_PRESENT;
        break;
    case CCIDD_STATE_CARD_INSERTED:
    case CCIDD_STATE_CARD_POWERING_ON:
        ccidd->msg->msg_specific[0] |= CCID_SLOT_STATUS_ICC_PRESENT_AND_INACTIVE;
    default:
        break;
    }
    //bError
    ccidd->msg->msg_specific[1] = err & 0xff;
    fwrite_async(usbd_usb(usbd), HAL_HANDLE(HAL_USB, USB_EP_IN | ccidd->data_ep), ccidd->usb_data_block, size + sizeof(CCID_MESSAGE));
}

static void ccidd_data_block(USBD* usbd, CCIDD* ccidd, uint8_t* buf, unsigned int size, unsigned int err)
{
    unsigned int chunk_size;
    ccidd->msg->bMessageType = RDR_TO_PC_DATA_BLOCK;
    ccidd->msg->dwLength = size;
    ccidd->msg->bSlot = 0;
    ccidd->msg->bSeq = ccidd->seq;
    //bChainParameter
    ccidd->msg->msg_specific[2] = 0;
    chunk_size = size;
    if (chunk_size > ccidd->data_ep_size - sizeof(CCID_MESSAGE))
        chunk_size = ccidd->data_ep_size - sizeof(CCID_MESSAGE);
    memcpy((uint8_t*)ccidd->msg + sizeof(CCID_MESSAGE), buf, chunk_size);
    ccidd_tx(usbd, ccidd, err, chunk_size);
}

static void ccidd_data_block_error(USBD* usbd, CCIDD* ccidd, unsigned int err)
{
    ccidd_data_block(usbd, ccidd, NULL, 0, err);
}

static inline void ccidd_slot_status(USBD* usbd, CCIDD* ccidd)
{
    ccidd->msg->bMessageType = RDR_TO_PC_SLOT_STATUS;
    ccidd->msg->dwLength = 0;
    ccidd->msg->bSlot = 0;
    //bClockStatus
    ccidd->msg->msg_specific[2] = CCID_CLOCK_STATUS_RUNNING;
    ccidd_tx(usbd, ccidd, 0, 0);
}

static inline void ccidd_icc_power_on(USBD* usbd, CCIDD* ccidd)
{
#if (USBD_CCID_DEBUG_REQUESTS)
    printf("CCIDD: ICC slot%d power on\n\r", ccidd->msg->bSlot);
#endif //USBD_CCID_DEBUG_REQUESTS
    if (ccidd->state == CCIDD_STATE_CARD_POWERED)
        ccidd_data_block(usbd, ccidd, ccidd->atr, ccidd->atr_size, 0);
    else
    {
        ccidd->state = CCIDD_STATE_CARD_POWERING_ON;
        usbd_post_user(usbd, ccidd->iface, USB_CCID_HOST_POWER_ON, ccidd->msg->bSlot);
    }
}

static inline void ccidd_icc_power_off(USBD* usbd, CCIDD* ccidd)
{
#if (USBD_CCID_DEBUG_REQUESTS)
    printf("CCIDD: ICC slot%d power off\n\r", ccidd->msg->bSlot);
#endif //USBD_CCID_DEBUG_REQUESTS
    if (ccidd->state == CCIDD_STATE_CARD_INSERTED || ccidd->state == CCIDD_STATE_NO_CARD)
        ccidd_slot_status(usbd, ccidd);
    else
    {
        ccidd->state = CCIDD_STATE_CARD_POWERING_OFF;
        usbd_post_user(usbd, ccidd->iface, USB_CCID_HOST_POWER_OFF, ccidd->msg->bSlot);
    }
}

static inline void ccidd_get_slot_status(USBD* usbd, CCIDD* ccidd)
{
#if (USBD_CCID_DEBUG_REQUESTS)
    printf("CCIDD: get slot%d status\n\r", ccidd->msg->bSlot);
#endif //USBD_CCID_DEBUG_REQUESTS
    ccidd_slot_status(usbd, ccidd);
}

static void ccidd_rx(USBD* usbd, CCIDD* ccidd, uint8_t* buf, int size)
{
    memcpy(ccidd->data_buf + ccidd->data_processed, buf, size);
    ccidd->data_processed += size;
    if (ccidd->data_processed >= ccidd->rx_size)
    {
        if (ccidd->rx_more)
        {
            ccidd->state = CCIDD_STATE_READY;
            ccidd_tx(usbd, ccidd, 0, 0);
        }
        else
        {
            ccidd->state = CCIDD_STATE_SC_REQUEST;
#if (USBD_CCID_DEBUG_IO)
            usbd_dump(ccidd->data_buf, ccidd->data_processed, "CCIDD C-APDU");
#endif
            fread_complete(usbd_user(usbd), HAL_USBD_INTERFACE(ccidd->iface, 0), ccidd->data_block, ccidd->data_processed);
            ccidd->data_block_size = ccidd->data_processed = ccidd->rx_size = 0;
            ccidd->data_block = INVALID_HANDLE;
        }
    }
}

static inline void ccidd_xfer_block(USBD* usbd, CCIDD* ccidd)
{
    unsigned int chunk_size;
#if (USBD_CCID_DEBUG_REQUESTS)
    printf("CCIDD: Xfer block to slot%d, size: %d, chaining: %#X\n\r", ccidd->msg->bSlot, ccidd->msg->dwLength, ccidd->msg->msg_specific[1]);
#endif //USBD_CCID_DEBUG_REQUESTS
    switch (ccidd->state)
    {
    case CCIDD_STATE_READY:
        if (ccidd->msg->dwLength + ccidd->data_processed <= ccidd->data_block_size)
        {
            ccidd->rx_size += ccidd->msg->dwLength;
            ccidd->state = CCIDD_STATE_RX;
            chunk_size = ccidd->msg->dwLength;
            if (chunk_size > ccidd->data_ep_size - sizeof(CCID_MESSAGE))
                chunk_size = ccidd->data_ep_size - sizeof(CCID_MESSAGE);
            switch (ccidd->msg->msg_specific[1])
            {
            case CCID_EXT_APDU_BEGIN:
            case CCID_EXT_APDU_MORE:
                ccidd->rx_more = true;
                break;
            default:
                ccidd->rx_more = false;
            }

            ccidd_rx(usbd, ccidd, (uint8_t*)ccidd->msg + sizeof(CCID_MESSAGE), chunk_size);
        }
        else
        {
            ccidd_data_block_error(usbd, ccidd, CCID_SLOT_ERROR_XFR_OVERRUN);
#if (USBD_CCID_DEBUG_ERRORS)
            printf("CCIDD: overrun\n\r");
#endif //USBD_CCID_DEBUG_REQUESTS
        }
        break;
    default:
        ccidd_data_block_error(usbd, ccidd, CCID_SLOT_ERROR_ICC_MUTE);
        break;
    }
}

static inline void ccidd_get_params(USBD* usbd, CCIDD* ccidd)
{
    ccidd->msg->bMessageType = RDR_TO_PC_PARAMETERS;
    ccidd->msg->bSlot = 0;
    switch (ccidd->protocol)
    {
    case CCID_T_0:
        memcpy((uint8_t*)ccidd->msg + sizeof(CCID_MESSAGE), ccidd->params, sizeof(CCID_T0_PARAMS));
        ccidd->msg->msg_specific[2] = 0;
        ccidd_tx(usbd, ccidd, 0, sizeof(CCID_T0_PARAMS));
#if (USBD_CCID_DEBUG_REQUESTS)
        printf("CCIDD: get params - T0\n\r");
#endif //USBD_CCID_DEBUG_REQUESTS
        break;
    case CCID_T_1:
        memcpy((uint8_t*)ccidd->msg + sizeof(CCID_MESSAGE), ccidd->params, sizeof(CCID_T1_PARAMS));
        ccidd->msg->msg_specific[2] = 1;
        ccidd_tx(usbd, ccidd, 0, sizeof(CCID_T1_PARAMS));
#if (USBD_CCID_DEBUG_REQUESTS)
        printf("CCIDD: get params - T1\n\r");
#endif //USBD_CCID_DEBUG_REQUESTS
        break;
    default:
        ccidd_tx(usbd, ccidd, CCID_SLOT_ERROR_DEACTIVATED_PROTOCOL, 0);
    }
}

static inline void ccidd_set_params(USBD* usbd, CCIDD* ccidd)
{
    ccidd->protocol = ccidd->msg->msg_specific[2];
    memcpy(ccidd->params, (uint8_t*)ccidd->msg + sizeof(CCID_MESSAGE), ccidd->msg->dwLength);
    usbd_post_user(usbd, ccidd->iface, USB_CCID_HOST_SET_PARAMS, ccidd->protocol);
    ccidd->state = CCIDD_STATE_PARAMS_REQUEST;
#if (USBD_CCID_DEBUG_REQUESTS)
    printf("CCIDD: set params - T%d\n\r", ccidd->protocol);
#endif //USBD_CCID_DEBUG_REQUESTS
}

static inline void ccidd_reset_params(USBD* usbd, CCIDD* ccidd)
{
    usbd_post_user(usbd, ccidd->iface, USB_CCID_HOST_RESET_PARAMS, 0);
    ccidd->state = CCIDD_STATE_PARAMS_REQUEST;
#if (USBD_CCID_DEBUG_REQUESTS)
    printf("CCIDD: reset params\n\r");
#endif //USBD_CCID_DEBUG_REQUESTS
}

static inline void ccidd_abort(USBD* usbd, CCIDD* ccidd)
{
#if (USBD_CCID_DEBUG_REQUESTS)
    printf("CCIDD: Abort\n\r");
#endif //USBD_CCID_DEBUG_REQUESTS
    if (ccidd->state == CCIDD_STATE_ABORTING)
    {
        usbd_post_user(usbd, ccidd->iface, USB_CCID_HOST_ABORT, ccidd->msg->bSlot);
        ccidd->state = CCIDD_STATE_CARD_POWERED;
        ccidd_slot_status(usbd, ccidd);
    }
    else
        ccidd_data_block_error(usbd, ccidd, CCID_SLOT_ERROR_CMD_ABORTED);
}

static inline void ccidd_message_rx(USBD* usbd, CCIDD* ccidd)
{
    ccidd->msg = (CCID_MESSAGE*)block_open(ccidd->usb_data_block);
    ccidd->seq = ccidd->msg->bSeq;
    if (ccidd->state == CCIDD_STATE_HW_ERROR)
    {
        ccidd_data_block_error(usbd, ccidd, CCID_SLOT_ERROR_HW_ERROR);
        return;
    }
    switch (ccidd->msg->bMessageType)
    {
    case PC_TO_RDR_ICC_POWER_ON:
        ccidd_icc_power_on(usbd, ccidd);
        break;
    case PC_TO_RDR_ICC_POWER_OFF:
        ccidd_icc_power_off(usbd, ccidd);
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
    case PC_TO_RDR_ABORT:
        ccidd_abort(usbd, ccidd);
        break;
    default:
#if (USBD_CCID_DEBUG_ERRORS)
        printf("CCIDD: unsupported CMD: %#X\n\r", ccidd->msg->bMessageType);
#endif //USBD_DEBUG_CLASS_REQUESTS
        ccidd_data_block_error(usbd, ccidd, CCID_SLOT_ERROR_CMD_NOT_SUPPORTED | 0x100);
    }
}

static inline void ccidd_rx_more(USBD* usbd, CCIDD* ccidd, int size)
{
    ccidd->msg = (CCID_MESSAGE*)block_open(ccidd->usb_data_block);
    ccidd_rx(usbd, ccidd, (uint8_t*)ccidd->msg, size);
}

static void ccidd_tx_complete(USBD* usbd, CCIDD* ccidd)
{
#if (USBD_CCID_DEBUG_IO)
    usbd_dump(ccidd->data_buf, ccidd->data_processed, "CCIDD R-APDU");
#endif
    fwrite_complete(usbd_user(usbd), HAL_USBD_INTERFACE(ccidd->iface, 0), ccidd->data_block, ccidd->data_processed);
    ccidd->data_block = INVALID_HANDLE;
    ccidd->data_block_size = ccidd->data_processed = 0;
    ccidd->state = CCIDD_STATE_CARD_POWERED;
}

static inline void ccidd_message_tx(USBD* usbd, CCIDD* ccidd)
{
    unsigned int chunk_size;
    uint8_t* buf;
    switch (ccidd->state)
    {
    case CCIDD_STATE_TX:
        if (ccidd->data_processed >= ccidd->data_block_size)
        {
            if ((ccidd->data_block_size + sizeof(CCID_MESSAGE)) % ccidd->data_ep_size)
                ccidd_tx_complete(usbd, ccidd);
            else
            {
                fwrite_async_null(usbd_usb(usbd), HAL_HANDLE(HAL_USB, USB_EP_IN | ccidd->data_ep));
                ccidd->state = CCIDD_STATE_TX_ZLP;
            }
        }
        //tx more
        else
        {
            buf = block_open(ccidd->usb_data_block);
            chunk_size = ccidd->data_block_size - ccidd->data_processed;
            if (chunk_size > ccidd->data_ep_size)
                chunk_size = ccidd->data_ep_size;
            memcpy(buf, ccidd->data_buf + ccidd->data_processed, chunk_size);
            ccidd->data_processed += chunk_size;
            fwrite_async(usbd_usb(usbd), HAL_HANDLE(HAL_USB, USB_EP_IN | ccidd->data_ep), ccidd->usb_data_block, chunk_size);
        }
        break;
    case CCIDD_STATE_TX_ZLP:
        ccidd_tx_complete(usbd, ccidd);
        break;
    case CCIDD_STATE_CARD_POWERED:
        //NACK until next fread
        break;
    default:
        fread_async(usbd_usb(usbd), HAL_HANDLE(HAL_USB, ccidd->data_ep), ccidd->usb_data_block, ccidd->data_ep_size);
        break;
    }
}

static inline void ccidd_card_insert(USBD* usbd, CCIDD* ccidd)
{
    switch (ccidd->state)
    {
    case CCIDD_STATE_NO_CARD:
        ccidd->state = CCIDD_STATE_CARD_INSERTED;
        ccidd_notify_state_change(usbd, ccidd);
        break;
    case CCIDD_STATE_CARD_INSERTED:
        //no state change
        break;
    default:
        //wrong state, ignore
        break;
    }
}

static inline void ccidd_card_remove(USBD* usbd, CCIDD* ccidd)
{
    switch (ccidd->state)
    {
    case CCIDD_STATE_CARD_POWERED:
    case CCIDD_STATE_CARD_INSERTED:
        ccidd->state = CCIDD_STATE_NO_CARD;
        ccidd_notify_state_change(usbd, ccidd);
        break;
    case CCIDD_STATE_NO_CARD:
        //no state change
        break;
    default:
        //wrong state, ignore
        break;
    }
}

static void ccidd_card_power_on_internal(CCIDD* ccidd, HANDLE process, unsigned int size)
{
    ccidd->atr_size = size;
    if (ccidd->atr_size > USBD_CCID_MAX_ATR_SIZE)
        ccidd->atr_size = USBD_CCID_MAX_ATR_SIZE;
    direct_read(process, ccidd->atr, ccidd->atr_size);
    ccidd->state = CCIDD_STATE_CARD_POWERED;
}

static inline void ccidd_card_power_on(USBD* usbd, CCIDD* ccidd, HANDLE process, unsigned int size)
{
    switch (ccidd->state)
    {
    case CCIDD_STATE_CARD_POWERING_ON:
        ccidd_card_power_on_internal(ccidd, process, size);
        ccidd_data_block(usbd, ccidd, ccidd->atr, ccidd->atr_size, 0);
        break;
    case CCIDD_STATE_CARD_INSERTED:
    case CCIDD_STATE_NO_CARD:
        ccidd_card_power_on_internal(ccidd, process, size);
        ccidd_notify_state_change(usbd, ccidd);
        break;
    case CCIDD_STATE_CARD_POWERED:
        //no state change
        break;
    default:
        //wrong state, ignore
        break;
    }
}

static inline void ccidd_card_power_off(USBD* usbd, CCIDD* ccidd)
{
    switch (ccidd->state)
    {
    case CCIDD_STATE_CARD_POWERING_OFF:
        ccidd->state = CCIDD_STATE_CARD_INSERTED;
        ccidd_slot_status(usbd, ccidd);
        break;
    case CCIDD_STATE_CARD_POWERED:
        ccidd->state = CCIDD_STATE_CARD_INSERTED;
        ccidd_notify_state_change(usbd, ccidd);
        break;
    case CCIDD_STATE_CARD_INSERTED:
    case CCIDD_STATE_NO_CARD:
        //no state change
        break;
    default:
        //wrong state, ignore
        break;
    }
}

static inline void ccidd_card_hw_error(USBD* usbd, CCIDD* ccidd)
{
    if (ccidd->data_block != INVALID_HANDLE)
    {
        block_send(ccidd->data_block, usbd_user(usbd));
        ccidd->data_block = INVALID_HANDLE;
        ccidd->data_block_size = ccidd->data_processed = ccidd->rx_size = 0;
    }
    switch (ccidd->state)
    {
    //respond with error on user-handled requests
    case CCIDD_STATE_SC_REQUEST:
    case CCIDD_STATE_CARD_POWERING_ON:
    case CCIDD_STATE_CARD_POWERING_OFF:
    case CCIDD_STATE_PARAMS_REQUEST:
        ccidd_data_block_error(usbd, ccidd, CCID_SLOT_ERROR_HW_ERROR);
        break;
    default:
        break;
    }
    ccidd->state = CCIDD_STATE_HW_ERROR;
}

static inline void ccidd_card_reset(USBD* usbd, CCIDD* ccidd)
{
    if (ccidd->data_block)
    {
        block_send(ccidd->data_block, usbd_user(usbd));
        ccidd->data_block = INVALID_HANDLE;
        ccidd->data_block_size = ccidd->data_processed = ccidd->rx_size = 0;
    }
    switch (ccidd->state)
    {
    //respond with error on user-handled requests
    case CCIDD_STATE_SC_REQUEST:
    case CCIDD_STATE_CARD_POWERING_ON:
    case CCIDD_STATE_CARD_POWERING_OFF:
    case CCIDD_STATE_PARAMS_REQUEST:
        ccidd_data_block_error(usbd, ccidd, CCID_SLOT_ERROR_CMD_ABORTED);
        break;
    default:
        break;
    }
    ccidd->state = CCIDD_STATE_NO_CARD;
    ccidd_notify_state_change(usbd, ccidd);
}

static inline void ccidd_read(USBD* usbd, CCIDD* ccidd, HANDLE block, unsigned int max_size)
{
    if (ccidd->state != CCIDD_STATE_CARD_POWERED)
    {
        fio_cancelled(usbd_user(usbd), HAL_USBD_INTERFACE(ccidd->iface, 0), block, ERROR_INVALID_STATE);
        return;
    }
    ccidd->data_buf = block_open(block);
    if (max_size && (block == INVALID_HANDLE || ccidd->data_buf == NULL))
    {
        fio_cancelled(usbd_user(usbd), HAL_USBD_INTERFACE(ccidd->iface, 0), block, get_last_error());
        return;
    }
    ccidd->data_block = block;
    ccidd->data_block_size = max_size;
    ccidd->data_processed = ccidd->rx_size = 0;
    ccidd->state = CCIDD_STATE_READY;
    fread_async(usbd_usb(usbd), HAL_HANDLE(HAL_USB, ccidd->data_ep), ccidd->usb_data_block, ccidd->data_ep_size);
}

static inline void ccidd_card_set_params(USBD* usbd, CCIDD* ccidd, HANDLE process, CCID_PROTOCOL protocol)
{
    direct_read(process, ccidd->params, protocol == CCID_T_0 ? sizeof(CCID_T0_PARAMS) : sizeof(CCID_T1_PARAMS));
    ccidd->protocol = protocol;
}

static inline CCID_PROTOCOL ccidd_card_get_params(USBD* usbd, CCIDD* ccidd, HANDLE process)
{
    direct_write(process, ccidd->params, ccidd->protocol == CCID_T_0 ? sizeof(CCID_T0_PARAMS) : sizeof(CCID_T1_PARAMS));
    return ccidd->protocol;
}

static inline void ccidd_write(USBD* usbd, CCIDD* ccidd, HANDLE block, unsigned int size)
{
    unsigned int chunk_size;
    if (ccidd->state != CCIDD_STATE_SC_REQUEST)
    {
        fio_cancelled(usbd_user(usbd), HAL_USBD_INTERFACE(ccidd->iface, 0), block, ERROR_INVALID_STATE);
        return;
    }
    ccidd->data_buf = block_open(block);
    if (size && (block == INVALID_HANDLE || ccidd->data_buf == NULL))
    {
        fio_cancelled(usbd_user(usbd), HAL_USBD_INTERFACE(ccidd->iface, 0), block, get_last_error());
        return;
    }
    ccidd->data_block = block;
    ccidd->data_block_size = size;
    ccidd->data_processed = 0;

    chunk_size = size;
    if (chunk_size > ccidd->data_ep_size - sizeof(CCID_MESSAGE))
        chunk_size = ccidd->data_ep_size - sizeof(CCID_MESSAGE);
    ccidd_data_block(usbd, ccidd, ccidd->data_buf, ccidd->data_block_size, 0);
    ccidd->data_processed = chunk_size;
    ccidd->state = CCIDD_STATE_TX;
}

bool ccidd_class_request(USBD* usbd, void* param, IPC* ipc)
{
    CCIDD* ccidd = (CCIDD*)param;
    bool need_post = false;
    switch (ipc->cmd)
    {
    case USBD_INTERFACE_REQUEST:
        switch (ipc->param2)
        {
        case USB_CCID_CARD_INSERT:
            ccidd_card_insert(usbd, ccidd);
            break;
        case USB_CCID_CARD_REMOVE:
            ccidd_card_remove(usbd, ccidd);
            break;
        case USB_CCID_CARD_POWER_ON:
            ccidd_card_power_on(usbd, ccidd, ipc->process, ipc->param3);
            break;
        case USB_CCID_CARD_POWER_OFF:
            ccidd_card_power_off(usbd, ccidd);
            break;
        case USB_CCID_CARD_HW_ERROR:
            ccidd_card_hw_error(usbd, ccidd);
            break;
        case USB_CCID_CARD_RESET:
            ccidd_card_reset(usbd, ccidd);
            break;
        case USB_CCID_CARD_SET_PARAMS:
            ccidd_card_set_params(usbd, ccidd, ipc->process, ipc->param3);
            break;
        case USB_CCID_CARD_GET_PARAMS:
            ipc->param3 = ccidd_card_get_params(usbd, ccidd, ipc->process);
            break;
        }
        need_post = true;
        break;
    case IPC_READ:
        ccidd_read(usbd, ccidd, ipc->param2, ipc->param3);
        break;
    case IPC_WRITE:
        ccidd_write(usbd, ccidd, ipc->param2, ipc->param3);
        break;
    case IPC_READ_COMPLETE:
        if (USB_EP_NUM(HAL_ITEM(ipc->param1)) == ccidd->data_ep)
        {
            if (ccidd->state == CCIDD_STATE_RX)
                ccidd_rx_more(usbd, ccidd, ipc->param3);
            else
                ccidd_message_rx(usbd, ccidd);
        }
        break;
    case IPC_WRITE_COMPLETE:
        if (USB_EP_NUM(HAL_ITEM(ipc->param1)) == ccidd->data_ep)
            ccidd_message_tx(usbd, ccidd);
        //status notify complete
        else
            ccidd->status_busy = false;
        break;
    default:
#if (USBD_CCID_DEBUG_REQUESTS)
        printf("CCIDD class request: %#X\n\r", ipc->cmd);
#endif //USBD_CCID_DEBUG_REQUESTS
        break;
    }
    return need_post;
}

const USBD_CLASS __CCIDD_CLASS = {
    ccidd_class_configured,
    ccidd_class_reset,
    ccidd_class_suspend,
    ccidd_class_resume,
    ccidd_class_setup,
    ccidd_class_request,
};
