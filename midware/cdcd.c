/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "cdcd.h"
#include "usbd.h"
#include "../userspace/sys.h"
#include "../userspace/file.h"
#include "../userspace/stdio.h"
#include "../userspace/block.h"
#include "../userspace/direct.h"
#include "../userspace/uart.h"
#include "../userspace/stdlib.h"
#include "sys_config.h"

#define CDC_BLOCK_SIZE                                                          64
#define CDC_NOTIFY_SIZE                                                         64

#define LC_BAUD_STOP_BITS_SIZE                                                  3
static const int LC_BAUD_STOP_BITS[LC_BAUD_STOP_BITS_SIZE] =                    {1, 15, 2};

#define LC_BAUD_PARITY_SIZE                                                     5
static const char LC_BAUD_PARITY[LC_BAUD_PARITY_SIZE] =                         "NOEMS";

#if (USBD_CDC_DEBUG_REQUESTS) || (SYS_INFO)
const char* const ON_OFF[] =                                                    {"off", "on"};
#endif


typedef struct {
    HANDLE rx, tx, notify, rx_stream, tx_stream;
    HANDLE tx_stream_handle, rx_stream_handle;
    uint8_t data_ep, control_ep;
    uint16_t data_ep_size, control_ep_size, rx_free, tx_size;
    uint8_t DTR, RTS, tx_idle, control_iface, data_iface;
    uint8_t suspended;
    BAUD baud;
} CDCD;

static inline void* cdcd_notify_open(USBD* usbd, CDCD* cdcd)
{
    void* res = block_open(cdcd->notify);
    //host is not issued IN for interrupt yet, flush last
    if (res == NULL)
    {
        fflush(usbd_usb(usbd), HAL_HANDLE(HAL_USB, USB_EP_IN | cdcd->control_ep));
        res = block_open(cdcd->notify);
    }
    return res;
}

void cdcd_notify_serial_state(USBD* usbd, CDCD* cdcd, unsigned int state)
{
    char* notify = cdcd_notify_open(usbd, cdcd);
    if (notify)
    {
        SETUP* setup = (SETUP*)notify;
        setup->bmRequestType = BM_REQUEST_DIRECTION_DEVICE_TO_HOST | BM_REQUEST_TYPE_CLASS | BM_REQUEST_RECIPIENT_INTERFACE;
        setup->bRequest = CDC_SERIAL_STATE;
        setup->wValue = 0;
        setup->wIndex = 1;
        setup->wLength = 2;
        uint16_t* serial_state = (uint16_t*)(notify + sizeof(SETUP));
        *serial_state = state;
        fwrite_async(usbd_usb(usbd), HAL_HANDLE(HAL_USB, USB_EP_IN | cdcd->control_ep), cdcd->notify, sizeof(SETUP) + 2);
    }
}

void cdcd_destroy(CDCD* cdcd)
{
    block_destroy(cdcd->notify);

    block_destroy(cdcd->rx);
    stream_close(cdcd->rx_stream_handle);
    stream_destroy(cdcd->rx_stream);

    block_destroy(cdcd->tx);
    stream_close(cdcd->tx_stream_handle);
    stream_destroy(cdcd->tx_stream);

    free(cdcd);
}

void cdcd_class_configured(USBD* usbd, USB_CONFIGURATION_DESCRIPTOR_TYPE* cfg)
{

    USB_INTERFACE_DESCRIPTOR_TYPE* iface;
    USB_ENDPOINT_DESCRIPTOR_TYPE* ep;
    uint8_t data_ep, control_ep, data_ep_size, control_ep_size, control_iface, data_iface;
    unsigned int size;
    data_ep = control_ep = data_ep_size = control_ep_size = data_iface = control_iface = 0;

    //check control/data ep here
    for (iface = usb_get_first_interface(cfg); iface != NULL; iface = usb_get_next_interface(cfg, iface))
    {
        ep = (USB_ENDPOINT_DESCRIPTOR_TYPE*)usb_interface_get_first_descriptor(cfg, iface, USB_ENDPOINT_DESCRIPTOR_INDEX);
        if (ep != NULL)
        {
            switch (iface->bInterfaceClass)
            {
            case CDC_DATA_INTERFACE_CLASS:
                data_ep = USB_EP_NUM(ep->bEndpointAddress);
                data_ep_size = ep->wMaxPacketSize;
                data_iface = iface->bInterfaceNumber;
                break;
            case CDC_COMM_INTERFACE_CLASS:
                control_ep = USB_EP_NUM(ep->bEndpointAddress);
                control_ep_size = ep->wMaxPacketSize;
                control_iface = iface->bInterfaceNumber;
                break;
            default:
                break;
            }
        }
    }

    //No CDC descriptors in interface
    if (control_ep == 0)
        return;
    CDCD* cdcd = (CDCD*)malloc(sizeof(CDCD));
    if (cdcd == NULL)
        return;

    cdcd->control_iface = control_iface;
    cdcd->control_ep = control_ep;
    cdcd->control_ep_size = control_ep_size;
    cdcd->data_iface = data_iface;
    cdcd->data_ep = data_ep;
    cdcd->data_ep_size = data_ep_size;
    cdcd->tx = cdcd->rx = cdcd->notify = cdcd->tx_stream = cdcd->rx_stream = cdcd->tx_stream_handle = cdcd->rx_stream_handle = INVALID_HANDLE;
    cdcd->suspended = false;

#if (USBD_CDC_DEBUG_REQUESTS)
    printf("Found USB CDCD ACM class, EP%d, size: %d, iface: %d\n\r", cdcd->data_ep, cdcd->data_ep_size, cdcd->data_iface);
    if (cdcd->control_ep)
        printf("Has control EP%d, size: %d, iface: %d\n\r", cdcd->control_ep, cdcd->control_ep_size, cdcd->control_iface);
#endif //USBD_CDC_DEBUG_REQUESTS

#if (USB_CDCD_TX_STREAM_SIZE)
    cdcd->tx = block_create(cdcd->data_ep_size);
    cdcd->tx_stream = stream_create(USB_CDCD_RX_STREAM_SIZE);
    cdcd->tx_stream_handle = stream_open(cdcd->tx_stream);
    if (cdcd->tx == INVALID_HANDLE || cdcd->tx_stream_handle == INVALID_HANDLE)
    {
        cdcd_destroy(cdcd);
        return;
    }
    cdcd->tx_size = 0;
    cdcd->tx_idle = true;
    size = cdcd->data_ep_size;
    fopen_p(usbd_usb(usbd), HAL_HANDLE(HAL_USB, USB_EP_IN | cdcd->data_ep), USB_EP_BULK, (void*)size);
    stream_listen(cdcd->tx_stream, (void*)(HAL_HANDLE(HAL_USBD, HAL_USBD_INTERFACE(cdcd->data_iface, 0))));
#endif

#if (USB_CDCD_RX_STREAM_SIZE)
    cdcd->rx = block_create(cdcd->data_ep_size);
    cdcd->rx_stream = stream_create(USB_CDCD_RX_STREAM_SIZE);
    cdcd->rx_stream_handle = stream_open(cdcd->rx_stream);
    if (cdcd->rx == INVALID_HANDLE || cdcd->rx_stream_handle == INVALID_HANDLE)
    {
        cdcd_destroy(cdcd);
        return;
    }
    cdcd->rx_free = 0;
    size = cdcd->data_ep_size;
    fopen_p(usbd_usb(usbd), HAL_HANDLE(HAL_USB, cdcd->data_ep), USB_EP_BULK, (void*)size);
    fread_async(usbd_usb(usbd), HAL_HANDLE(HAL_USB, cdcd->data_ep), cdcd->rx, 1);
#endif

    if (control_ep)
    {
        cdcd->notify = block_create(cdcd->control_ep_size);
        if (cdcd->notify == INVALID_HANDLE)
        {
            cdcd_destroy(cdcd);
            return;
        }
        size = cdcd->control_ep_size;
        fopen_p(usbd_usb(usbd), HAL_HANDLE(HAL_USB, USB_EP_IN | cdcd->control_ep), USB_EP_INTERRUPT, (void*)size);
        cdcd_notify_serial_state(usbd, cdcd, CDC_SERIAL_STATE_DCD | CDC_SERIAL_STATE_DSR);
        usbd_register_interface(usbd, cdcd->control_iface, &__CDCD_CLASS, cdcd);
        usbd_register_endpoint(usbd, cdcd->control_iface, cdcd->control_ep);
    }
    usbd_register_interface(usbd, cdcd->data_iface, &__CDCD_CLASS, cdcd);
    usbd_register_endpoint(usbd, cdcd->data_iface, cdcd->data_ep);

    cdcd->DTR = cdcd->RTS = false;
    cdcd->baud.baud = 115200;
    cdcd->baud.data_bits = 8;
    cdcd->baud.parity = 'N';
    cdcd->baud.stop_bits = 1;
}

void cdcd_class_reset(USBD* usbd, void* param)
{
    CDCD* cdcd = (CDCD*)param;

#if (USB_CDCD_TX_STREAM_SIZE)
    stream_stop_listen(cdcd->tx_stream);
    stream_flush(cdcd->tx_stream);
    fflush(usbd_usb(usbd), HAL_HANDLE(HAL_USB, USB_EP_IN | cdcd->data_ep));
    fclose(usbd_usb(usbd), HAL_HANDLE(HAL_USB, USB_EP_IN | cdcd->data_ep));
#endif
#if (USB_CDCD_RX_STREAM_SIZE)
    stream_flush(cdcd->rx_stream);
    fflush(usbd_usb(usbd), HAL_HANDLE(HAL_USB, cdcd->data_ep));
    fclose(usbd_usb(usbd), HAL_HANDLE(HAL_USB, cdcd->data_ep));
#endif

    usbd_unregister_endpoint(usbd, cdcd->data_iface, cdcd->data_ep);
    usbd_unregister_interface(usbd, cdcd->data_iface, &__CDCD_CLASS);
    if (cdcd->control_ep)
    {
        fflush(usbd_usb(usbd), HAL_HANDLE(HAL_USB, USB_EP_IN | cdcd->control_ep));
        fclose(usbd_usb(usbd), HAL_HANDLE(HAL_USB, USB_EP_IN | cdcd->control_ep));
        usbd_unregister_endpoint(usbd, cdcd->control_iface, cdcd->control_ep);
        usbd_unregister_interface(usbd, cdcd->control_iface, &__CDCD_CLASS);
    }
    cdcd_destroy(cdcd);
}

void cdcd_class_suspend(USBD* usbd, void* param)
{
    CDCD* cdcd = (CDCD*)param;
#if (USB_CDCD_TX_STREAM_SIZE)
    stream_stop_listen(cdcd->tx_stream);
    stream_flush(cdcd->tx_stream);
    fflush(usbd_usb(usbd), HAL_HANDLE(HAL_USB, USB_EP_IN | cdcd->data_ep));
    cdcd->tx_idle = true;
    cdcd->tx_size = 0;
#endif
#if (USB_CDCD_RX_STREAM_SIZE)
    stream_flush(cdcd->rx_stream);
    fflush(usbd_usb(usbd), HAL_HANDLE(HAL_USB, cdcd->data_ep));
    cdcd->rx_free = 0;
#endif

    if (cdcd->control_ep)
        fflush(usbd_usb(usbd), HAL_HANDLE(HAL_USB, USB_EP_IN | cdcd->data_ep));
    cdcd->suspended = true;
}

void cdcd_class_resume(USBD* usbd, void* param)
{
    CDCD* cdcd = (CDCD*)param;
    cdcd->suspended = false;
#if (USB_CDCD_RX_STREAM_SIZE)
    stream_listen(cdcd->tx_stream, (void*)(HAL_HANDLE(HAL_USBD, HAL_USBD_INTERFACE(cdcd->data_iface, 0)));
#endif
}

static inline void cdcd_read_complete(USBD* usbd, CDCD* cdcd, unsigned int size)
{
    if (cdcd->suspended)
        return;

    unsigned int to_read;
    void* ptr;
    if (cdcd->rx_free < size)
        cdcd->rx_free = stream_get_free(cdcd->rx_stream);
    to_read = size;
    if (to_read > cdcd->rx_free)
        to_read = cdcd->rx_free;
    ptr = block_open(cdcd->rx);
#if (USBD_CDC_DEBUG_IO)
    int i;
    printf("CDCD rx: ");
    for (i = 0; i < size; ++i)
        if (((uint8_t*)ptr)[i] >= ' ' && ((uint8_t*)ptr)[i] <= '~')
            printf("%c", ((char*)ptr)[i]);
        else
            printf("\\x%d", ((uint8_t*)ptr)[i]);
    printf("\n\r");
#endif //USBD_CDC_DEBUG_IO
    if (ptr && to_read && stream_write(cdcd->rx_stream_handle, ptr, to_read))
        cdcd->rx_free -= to_read;
    fread_async(usbd_usb(usbd), HAL_HANDLE(HAL_USB, cdcd->data_ep), cdcd->rx, 1);

    if (to_read < size)
        cdcd_notify_serial_state(usbd, cdcd, CDC_SERIAL_STATE_DCD | CDC_SERIAL_STATE_DSR | CDC_SERIAL_STATE_OVERRUN);
}

void cdcd_write(USBD* usbd, CDCD* cdcd)
{
    if (!cdcd->DTR || !cdcd->tx_idle || cdcd->suspended)
        return;

    unsigned int to_write;
    void* ptr;
    if (cdcd->tx_size == 0)
        cdcd->tx_size = stream_get_size(cdcd->tx_stream);

    to_write = cdcd->tx_size;
    if (to_write > cdcd->data_ep_size)
        to_write = cdcd->data_ep_size;
    if (to_write)
    {
        cdcd->tx_size -= to_write;
        ptr = block_open(cdcd->tx);
        if (ptr && stream_read(cdcd->tx_stream_handle, ptr, to_write))
        {
            cdcd->tx_idle = false;
            fwrite_async(usbd_usb(usbd), HAL_HANDLE(HAL_USB, USB_EP_IN | cdcd->data_ep), cdcd->tx, to_write);
        }
        //just in case of driver failure
        else
            stream_listen(cdcd->tx_stream, (void*)(HAL_HANDLE(HAL_USBD, HAL_USBD_INTERFACE(cdcd->data_iface, 0))));
    }
    else
        stream_listen(cdcd->tx_stream, (void*)(HAL_HANDLE(HAL_USBD, HAL_USBD_INTERFACE(cdcd->data_iface, 0))));
}

static inline int set_line_coding(CDCD* cdcd, HANDLE block)
{
    LINE_CODING_STRUCT* lc = block_open(block);
    if (lc == NULL)
        return -1;
    cdcd->baud.baud = lc->dwDTERate;
    if (lc->bCharFormat >= LC_BAUD_STOP_BITS_SIZE)
        return -1;
    cdcd->baud.stop_bits = LC_BAUD_STOP_BITS[lc->bCharFormat];
    if (lc->bParityType >= LC_BAUD_PARITY_SIZE)
        return -1;
    cdcd->baud.parity = LC_BAUD_PARITY[lc->bParityType];
    cdcd->baud.data_bits = lc->bDataBits;

#if (USBD_CDC_DEBUG_REQUESTS)
    printf("CDCD set line coding: %d %d%c%d\n\r", cdcd->baud.baud, cdcd->baud.data_bits, cdcd->baud.parity, cdcd->baud.stop_bits);
#endif
    return 0;
}

static inline int get_line_coding(CDCD* cdcd, HANDLE block)
{
    LINE_CODING_STRUCT* lc = block_open(block);
    if (lc == NULL)
        return -1;
    lc->dwDTERate = cdcd->baud.baud;
    switch (cdcd->baud.stop_bits)
    {
    case 1:
        lc->bCharFormat = 0;
        break;
    case 15:
        lc->bCharFormat = 1;
        break;
    case 2:
        lc->bCharFormat = 2;
        break;
    }
    switch (cdcd->baud.parity)
    {
    case 'N':
        lc->bParityType = 0;
        break;
    case 'O':
        lc->bParityType = 1;
        break;
    case 'E':
        lc->bParityType = 2;
        break;
    case 'M':
        lc->bParityType = 3;
        break;
    case 'S':
        lc->bParityType = 4;
        break;
    }
    lc->bDataBits = cdcd->baud.data_bits;

#if (USBD_CDC_DEBUG_REQUESTS)
    printf("CDCD get line coding: %d %d%c%d\n\r", cdcd->baud.baud, cdcd->baud.data_bits, cdcd->baud.parity, cdcd->baud.stop_bits);
#endif
    return sizeof(LINE_CODING_STRUCT);
}

static inline int set_control_line_state(USBD* usbd, CDCD* cdcd, SETUP* setup)
{
    cdcd->DTR = (setup->wValue >> 0) & 1;
    cdcd->RTS = (setup->wValue >> 0) & 1;
#if (USBD_CDC_DEBUG_REQUESTS)
    printf("CDCD set control line state: DTR %s, RTS %s\n\r", ON_OFF[1 * cdcd->DTR], ON_OFF[1 * cdcd->RTS]);
#endif

    //resume write if DTR is set
    if (cdcd->DTR)
        cdcd_write(usbd, cdcd);
    //flush if not
    else if (!cdcd->tx_idle)
    {
        fflush(usbd_usb(usbd), HAL_HANDLE(HAL_USB, USB_EP_IN | cdcd->data_ep));
        cdcd->tx_idle = true;
    }
    return 0;
}

int cdcd_class_setup(USBD* usbd, void* param, SETUP* setup, HANDLE block)
{
    CDCD* cdc = (CDCD*)param;
    int res = -1;
    switch (setup->bRequest)
    {
    case SET_LINE_CODING:
        res = set_line_coding(cdc, block);
        break;
    case GET_LINE_CODING:
        res = get_line_coding(cdc, block);
        break;
    case SET_CONTROL_LINE_STATE:
        res = set_control_line_state(usbd, cdc, setup);
        break;
    }
    return res;
}



bool cdcd_class_request(USBD* usbd, void* param, IPC* ipc)
{
    CDCD* cdcd = (CDCD*)param;
    bool need_post = false;
    switch (ipc->cmd)
    {
    case IPC_READ_COMPLETE:
        cdcd_read_complete(usbd, cdcd, ipc->param3);
        break;
    case IPC_WRITE_COMPLETE:
        //ignore notify complete
        if (ipc->param1 == HAL_HANDLE(HAL_USB, (cdcd->data_ep | USB_EP_IN)))
        {
            cdcd->tx_idle = true;
            cdcd_write(usbd, cdcd);
        }
        break;
    case IPC_STREAM_WRITE:
        cdcd->tx_size = ipc->param2;
        cdcd_write(usbd, cdcd);
        break;
    case IPC_GET_TX_STREAM:
        ipc->param1 = cdcd->tx_stream;
        need_post = true;
        break;
    case IPC_GET_RX_STREAM:
        ipc->param1 = cdcd->rx_stream;
        need_post = true;
        break;
    case USBD_INTERFACE_REQUEST:
        if (ipc->param2 == USB_CDC_SEND_BREAK)
        {
            cdcd_notify_serial_state(usbd, cdcd, CDC_SERIAL_STATE_DCD | CDC_SERIAL_STATE_DSR | CDC_SERIAL_STATE_BREAK);
            need_post = true;
            break;
        }
    default:
        error(ERROR_NOT_SUPPORTED);
        need_post = true;
        break;
    }
    return need_post;
}

const USBD_CLASS __CDCD_CLASS = {
    cdcd_class_configured,
    cdcd_class_reset,
    cdcd_class_suspend,
    cdcd_class_resume,
    cdcd_class_setup,
    cdcd_class_request,
};
