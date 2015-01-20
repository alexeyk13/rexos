#include "ccidd.h"
#include "../userspace/ccid.h"
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
    CCIDD_STATE_ABORTING
} CCIDD_STATE;

typedef struct {
    HANDLE usb;
    HANDLE usb_data_block, usb_status_block;
    CCIDD_STATE state;
    uint8_t atr[USB_CCID_MAX_ATR_SIZE];
    uint8_t atr_size, data_ep, iface, seq;
    uint8_t suspended;
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
    uint8_t iface_num, data_ep;
    data_ep = iface_num = 0;

    for (iface = usb_get_first_interface(cfg); iface != NULL; iface = usb_get_next_interface(cfg, iface))
    {
        if (iface->bInterfaceClass == CCID_INTERFACE_CLASS)
        {
            //TODO: decode EPs (include interrupt)
            iface_num = iface->bInterfaceNumber;
            data_ep = 2;
            break;
/*            ep = (USB_ENDPOINT_DESCRIPTOR_TYPE*)usb_interface_get_first_descriptor(cfg, iface, USB_ENDPOINT_DESCRIPTOR_INDEX);
            if (ep != NULL)
            {
                in_ep = USB_EP_NUM(ep->bEndpointAddress);
                in_ep_size = ep->wMaxPacketSize;
                hid_iface = iface->bInterfaceNumber;
                break;
            }*/
        }
    }

    //No CCID descriptors in interface
    if (data_ep == 0)
        return;
    CCIDD* ccidd = (CCIDD*)malloc(sizeof(CCIDD));
    if (ccidd == NULL)
        return;
    ccidd->usb_data_block = block_create(64);
    ccidd->usb_status_block = INVALID_HANDLE;
    //TODO: only if configured, block size calculate
    ccidd->usb_status_block = block_create(8);

    if (ccidd->usb_data_block == INVALID_HANDLE)
    {
        ccidd_destroy(ccidd);
        return;
    }
    ccidd->usb = object_get(SYS_OBJ_USB);
    ccidd->iface = iface_num;
    ccidd->data_ep = data_ep;
    ccidd->suspended = false;
    ccidd->state = CCIDD_STATE_NO_CARD;
    ccidd->atr_size = 0;
    ccidd->seq = 0;

#if (USB_CCID_DEBUG_REQUESTS)
    printf("Found USB CCID device class, data: EP%d, iface: %d\n\r", ccidd->data_ep, ccidd->iface);
#endif //USB_CCID_DEBUG_REQUESTS

    //TODO:
    unsigned int size;
    size = 64;
    fopen_p(ccidd->usb, HAL_HANDLE(HAL_USB, USB_EP_IN | ccidd->data_ep), USB_EP_BULK, (void*)size);
    fopen_p(ccidd->usb, HAL_HANDLE(HAL_USB, ccidd->data_ep), USB_EP_BULK, (void*)size);
    usbd_register_interface(usbd, ccidd->iface, &__CCIDD_CLASS, ccidd);
    usbd_register_endpoint(usbd, ccidd->iface, ccidd->data_ep);

    //TODO: only if configured
    size = 8;
    fopen_p(ccidd->usb, HAL_HANDLE(HAL_USB, USB_EP_IN | 1), USB_EP_INTERRUPT, (void*)size);
    usbd_register_endpoint(usbd, ccidd->iface, 1);

    //TODO:
    uint8_t* ptr = block_open(ccidd->usb_status_block);
    ptr[0] = 0x50;
    ptr[1] = 0x03;
//    fwrite_async(ccidd->usb, HAL_HANDLE(HAL_USB, USB_EP_IN | 1), ccidd->usb_status_block, 2);

    fread_async(ccidd->usb, HAL_HANDLE(HAL_USB, ccidd->data_ep), ccidd->usb_data_block, sizeof(CCID_MESSAGE));
}

void ccidd_class_reset(USBD* usbd, void* param)
{
    CCIDD* ccidd = (CCIDD*)param;

    fclose(ccidd->usb, HAL_HANDLE(HAL_USB, USB_EP_IN | ccidd->data_ep));
    fclose(ccidd->usb, HAL_HANDLE(HAL_USB, ccidd->data_ep));
    //TODO: only if configured
    fclose(ccidd->usb, HAL_HANDLE(HAL_USB, USB_EP_IN | 1));
    usbd_unregister_endpoint(usbd, ccidd->iface, 1);

    usbd_unregister_interface(usbd, ccidd->iface, &__CCIDD_CLASS);
    ccidd_destroy(ccidd);
}

void ccidd_class_suspend(USBD* usbd, void* param)
{
    CCIDD* ccidd = (CCIDD*)param;
    fflush(ccidd->usb, HAL_HANDLE(HAL_USB, USB_EP_IN | ccidd->data_ep));
    fflush(ccidd->usb, HAL_HANDLE(HAL_USB, ccidd->data_ep));
    //TODO: only if configured
    fflush(ccidd->usb, HAL_HANDLE(HAL_USB, USB_EP_IN | 1));
    ccidd->suspended = true;
}

void ccidd_class_resume(USBD* usbd, void* param)
{
    CCIDD* ccidd = (CCIDD*)param;
    ccidd->suspended = false;
    fread_async(ccidd->usb, HAL_HANDLE(HAL_USB, ccidd->data_ep), ccidd->usb_data_block, sizeof(CCID_MESSAGE));
}

int ccidd_class_setup(USBD* usbd, void* param, SETUP* setup, HANDLE block)
{
    CCIDD* ccidd = (CCIDD*)param;
    unsigned int res = -1;
    switch (setup->bRequest)
    {
    default:
#if (USB_CCID_DEBUG_REQUESTS)
        printf("CCIDD SETUP request\n\r");
#endif //USB_CCID_DEBUG_REQUESTS
    }
    return res;
}

static void ccidd_data_block(CCIDD* ccidd, CCID_MESSAGE* msg, uint8_t* buf, unsigned int size, uint8_t chain)
{
    msg->bMessageType = RDR_TO_PC_DATA_BLOCK;
    msg->dwLength = size;
    msg->bSlot = 0;
    msg->bSeq = ccidd->seq;
    //bStatus
    msg->msg_specific[0] = 0;
    //bError
    msg->msg_specific[1] = 0;
    //bChainParameter
    msg->msg_specific[2] = chain;
    memcpy((uint8_t*)msg + sizeof(CCID_MESSAGE), buf, size);
    fwrite_async(ccidd->usb, HAL_HANDLE(HAL_USB, USB_EP_IN | ccidd->data_ep), ccidd->usb_data_block, sizeof(CCID_MESSAGE) + size);
}

static inline void ccidd_slot_status(CCIDD* ccidd, CCID_MESSAGE* msg)
{
    msg->bMessageType = RDR_TO_PC_SLOT_STATUS;
    msg->dwLength = 0;
    msg->bSlot = 0;
    //bStatus
    msg->msg_specific[0] = CCID_SLOT_STATUS_NO_ERROR;
    switch (ccidd->state)
    {
    case CCIDD_STATE_NO_CARD:
        msg->msg_specific[0] |= CCID_SLOT_STATUS_ICC_NOT_PRESENT;
        break;
    case CCIDD_STATE_CARD_INSERTED:
    case CCIDD_STATE_CARD_POWERING_ON:
        msg->msg_specific[0] |= CCID_SLOT_STATUS_ICC_PRESENT_AND_INACTIVE;
    default:
        break;
    }
    //bError
    msg->msg_specific[1] = 0x00;
    //bClockStatus
    msg->msg_specific[2] = CCID_CLOCK_STATUS_RUNNING;
    fwrite_async(ccidd->usb, HAL_HANDLE(HAL_USB, USB_EP_IN | ccidd->data_ep), ccidd->usb_data_block, sizeof(CCID_MESSAGE));
}

static inline void ccidd_icc_power_on(CCIDD* ccidd, CCID_MESSAGE* msg)
{
#if (USB_CCID_DEBUG_REQUESTS)
    printf("CCIDD: ICC slot%d power on\n\r", msg->bSlot);
#endif //USB_CCID_DEBUG_REQUESTS
    if (ccidd->state == CCIDD_STATE_CARD_POWERED)
    {
        msg->bMessageType = RDR_TO_PC_SLOT_STATUS;
        ccidd_data_block(ccidd, msg, ccidd->atr, ccidd->atr_size, 0);
    }
    else
    {
        ccidd->state = CCIDD_STATE_CARD_POWERING_ON;
        //TODO: notify application to power up card
    }
}

static inline void ccidd_icc_power_off(CCIDD* ccidd, CCID_MESSAGE* msg)
{
#if (USB_CCID_DEBUG_REQUESTS)
    printf("CCIDD: ICC slot%d power off\n\r", msg->bSlot);
#endif //USB_CCID_DEBUG_REQUESTS
    if (ccidd->state == CCIDD_STATE_CARD_INSERTED || ccidd->state == CCIDD_STATE_NO_CARD)
        ccidd_slot_status(ccidd, msg);
    else
    {
        ccidd->state = CCIDD_STATE_CARD_POWERING_OFF;
        //TODO: notify application to power up card
        ccidd->state = CCIDD_STATE_CARD_INSERTED;
        ccidd_slot_status(ccidd, msg);
    }
}

static inline void ccidd_get_slot_status(CCIDD* ccidd, CCID_MESSAGE* msg)
{
#if (USB_CCID_DEBUG_REQUESTS)
    printf("CCIDD: get slot%d status\n\r", msg->bSlot);
#endif //USB_CCID_DEBUG_REQUESTS
    ccidd_slot_status(ccidd, msg);
}

static inline void ccidd_message(CCIDD* ccidd)
{
    CCID_MESSAGE* msg;
    msg = (CCID_MESSAGE*)block_open(ccidd->usb_data_block);
    ccidd->seq = msg->bSeq;
    switch (msg->bMessageType)
    {
    case PC_TO_RDR_ICC_POWER_ON:
        ccidd_icc_power_on(ccidd, msg);
        break;
    case PC_TO_RDR_ICC_POWER_OFF:
        ccidd_icc_power_off(ccidd, msg);
        break;
    case PC_TO_RDR_GET_SLOT_STATUS:
        ccidd_get_slot_status(ccidd, msg);
        break;
//    case PC_TO_RDR_XFER_BLOCK:
//    case PC_TO_RDR_GET_PARAMETERS:
//    case PC_TO_RDR_RESET_PARAMETERS:
//    case PC_TO_RDR_SET_PARAMETERS:
//    case PC_TO_RDR_ESCAPE:
//    case PC_TO_RDR_ICC_CLOCK:
//    case PC_TO_RDR_T0APDU:
//    case PC_TO_RDR_SECURE:
//    case PC_TO_RDR_MECHANICAL:
//    case PC_TO_RDR_ABORT:
//    case PC_TO_RDR_SET_DATA_RATE_AND_CLOCK:
    default:
#if (USB_CCID_DEBUG_REQUESTS)
        printf("CCIDD: unsupported CMD: %#X\n\r", msg->bMessageType);
#endif //USBD_DEBUG_CLASS_REQUESTS
        //TODO: response with status
    }

}

static inline void ccidd_card_insert(CCIDD* ccidd)
{
    switch (ccidd->state)
    {
    case CCIDD_STATE_NO_CARD:
        ccidd->state = CCIDD_STATE_CARD_INSERTED;
        //TODO: notify
        break;
    case CCIDD_STATE_CARD_INSERTED:
        //no state change
        break;
    default:
        //wrong state, ignore
        break;
    }
}

static inline void ccidd_card_remove(CCIDD* ccidd)
{
    switch (ccidd->state)
    {
    case CCIDD_STATE_CARD_POWERED:
        ccidd->atr_size = 0;
        //follow down
    case CCIDD_STATE_CARD_INSERTED:
        ccidd->state = CCIDD_STATE_NO_CARD;
        //TODO: notify
        break;
    case CCIDD_STATE_CARD_POWERING_ON:
    case CCIDD_STATE_CARD_POWERING_OFF:
        ccidd->state = CCIDD_STATE_NO_CARD;
        //TODO: response with error
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
    if (ccidd->atr_size > USB_CCID_MAX_ATR_SIZE)
        ccidd->atr_size = USB_CCID_MAX_ATR_SIZE;
    direct_read(process, ccidd->atr, ccidd->atr_size);
    ccidd->state = CCIDD_STATE_CARD_POWERED;
}

static inline void ccidd_card_power_on(CCIDD* ccidd, HANDLE process, unsigned int size)
{
    switch (ccidd->state)
    {
    case CCIDD_STATE_CARD_POWERING_ON:
        ccidd_card_power_on_internal(ccidd, process, size);
        //TODO: send ATR
        break;
    case CCIDD_STATE_CARD_INSERTED:
    case CCIDD_STATE_NO_CARD:
        ccidd_card_power_on_internal(ccidd, process, size);
        //TODO: notify
        break;
    case CCIDD_STATE_CARD_POWERED:
        //no state change
        break;
    default:
        //wrong state, ignore
        break;
    }
}

static inline void ccidd_card_power_off(CCIDD* ccidd)
{
    switch (ccidd->state)
    {
    case CCIDD_STATE_CARD_POWERING_OFF:
        ccidd->state = CCIDD_STATE_CARD_INSERTED;
        ccidd->atr_size = 0;
        //TODO: send bulk in
        break;
    case CCIDD_STATE_CARD_POWERED:
        ccidd->state = CCIDD_STATE_CARD_INSERTED;
        ccidd->atr_size = 0;
        //TODO: notify
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
            ccidd_card_insert(ccidd);
            break;
        case USB_CCID_CARD_REMOVE:
            ccidd_card_remove(ccidd);
            break;
        case USB_CCID_CARD_POWER_ON:
            ccidd_card_power_on(ccidd, ipc->process, ipc->param3);
            break;
        case USB_CCID_CARD_POWER_OFF:
            ccidd_card_power_off(ccidd);
            break;
        }
        need_post = true;
        break;
    case IPC_READ_COMPLETE:
        //TODO: not in data phase
        if (USB_EP_NUM(HAL_ITEM(ipc->param1)) == ccidd->data_ep)
            ccidd_message(ccidd);
        break;
    case IPC_WRITE_COMPLETE:
        //TODO: not in data phase
        if (USB_EP_NUM(HAL_ITEM(ipc->param1)) == ccidd->data_ep)
            fread_async(ccidd->usb, HAL_HANDLE(HAL_USB, ccidd->data_ep), ccidd->usb_data_block, sizeof(CCID_MESSAGE));
        break;
    default:
#if (USB_CCID_DEBUG_REQUESTS)
        printf("CCIDD class request: %#X\n\r", ipc->cmd);
#endif //USB_CCID_DEBUG_REQUESTS
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
