/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "cdc.h"
#include "usbd.h"
#include "../../userspace/sys.h"
#include "../../userspace/file.h"
#include "../../userspace/stdio.h"
#include "../../userspace/block.h"
#include "../../userspace/direct.h"
#include "../../userspace/uart.h"
#include "../../userspace/stdlib.h"
#include "sys_config.h"

#define CDC_BLOCK_SIZE                                                          64
#define CDC_NOTIFY_SIZE                                                         64

#define LC_BAUD_STOP_BITS_SIZE                                                  3
static const int LC_BAUD_STOP_BITS[LC_BAUD_STOP_BITS_SIZE] =                    {1, 15, 2};

#define LC_BAUD_PARITY_SIZE                                                     5
static const char LC_BAUD_PARITY[LC_BAUD_PARITY_SIZE] =                         "NOEMS";

#if (USB_DEBUG_CLASS_REQUESTS) || (SYS_INFO)
const char* const ON_OFF[] =                                                    {"off", "on"};
#endif


typedef struct {
    HANDLE usb;
    HANDLE rx, tx, notify, rx_stream, tx_stream;
    HANDLE tx_stream_handle, rx_stream_handle;
    uint8_t data_ep, control_ep;
    uint16_t data_ep_size, control_ep_size, rx_free, tx_size;
    uint8_t DTR, RTS, tx_idle, control_iface, data_iface;
    uint8_t suspended;
    BAUD baud;
} CDC;

static inline void* cdc_notify_open(CDC* cdc)
{
    void* res = block_open(cdc->notify);
    //host is not issued IN for interrupt yet, flush last
    if (res == NULL)
    {
        fflush(cdc->usb, HAL_HANDLE(HAL_USB, USB_EP_IN | cdc->control_ep));
        res = block_open(cdc->notify);
    }
    return res;
}

void cdc_notify_serial_state(CDC* cdc, unsigned int state)
{
    char* notify = cdc_notify_open(cdc);
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
        fwrite_async(cdc->usb, HAL_HANDLE(HAL_USB, USB_EP_IN | cdc->control_ep), cdc->notify, sizeof(SETUP) + 2);
    }
}

void cdc_destroy(CDC* cdc)
{
    block_destroy(cdc->notify);

    block_destroy(cdc->rx);
    stream_close(cdc->rx_stream_handle);
    stream_destroy(cdc->rx_stream);

    block_destroy(cdc->tx);
    stream_close(cdc->tx_stream_handle);
    stream_destroy(cdc->tx_stream);

    free(cdc);
}

void cdc_class_configured(USBD* usbd, USB_CONFIGURATION_DESCRIPTOR_TYPE* cfg)
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
    CDC* cdc = (CDC*)malloc(sizeof(CDC));
    if (cdc == NULL)
        return;

    cdc->usb = object_get(SYS_OBJ_USB);
    cdc->control_iface = control_iface;
    cdc->control_ep = control_ep;
    cdc->control_ep_size = control_ep_size;
    cdc->data_iface = data_iface;
    cdc->data_ep = data_ep;
    cdc->data_ep_size = data_ep_size;
    cdc->tx = cdc->rx = cdc->notify = cdc->tx_stream = cdc->rx_stream = cdc->tx_stream_handle = cdc->rx_stream_handle = INVALID_HANDLE;
    cdc->suspended = false;

#if (USB_DEBUG_CLASS_REQUESTS)
    printf("Found USB CDC ACM class, EP%d, size: %d, iface: %d\n\r", cdc->data_ep, cdc->data_ep_size, cdc->data_iface);
    if (cdc->control_ep)
        printf("Has control EP%d, size: %d, iface: %d\n\r", cdc->control_ep, cdc->control_ep_size, cdc->control_iface);
#endif //USB_DEBUG_CLASS_REQUESTS

#if (USB_CDC_TX_STREAM_SIZE)
    cdc->tx = block_create(cdc->data_ep_size);
    cdc->tx_stream = stream_create(USB_CDC_RX_STREAM_SIZE);
    cdc->tx_stream_handle = stream_open(cdc->tx_stream);
    if (cdc->tx == INVALID_HANDLE || cdc->tx_stream_handle == INVALID_HANDLE)
    {
        cdc_destroy(cdc);
        return;
    }
    cdc->tx_size = 0;
    cdc->tx_idle = true;
    size = cdc->data_ep_size;
    fopen_p(cdc->usb, HAL_HANDLE(HAL_USB, USB_EP_IN | cdc->data_ep), USB_EP_BULK, (void*)size);
    stream_listen(cdc->tx_stream, (void*)(HAL_HANDLE(HAL_USBD, USBD_HANDLE_INTERFACE + cdc->data_iface)));
#endif

#if (USB_CDC_RX_STREAM_SIZE)
    cdc->rx = block_create(cdc->data_ep_size);
    cdc->rx_stream = stream_create(USB_CDC_RX_STREAM_SIZE);
    cdc->rx_stream_handle = stream_open(cdc->rx_stream);
    if (cdc->rx == INVALID_HANDLE || cdc->rx_stream_handle == INVALID_HANDLE)
    {
        cdc_destroy(cdc);
        return;
    }
    cdc->rx_free = 0;
    size = cdc->data_ep_size;
    fopen_p(cdc->usb, HAL_HANDLE(HAL_USB, cdc->data_ep), USB_EP_BULK, (void*)size);
    fread_async(cdc->usb, HAL_HANDLE(HAL_USB, cdc->data_ep), cdc->rx, 1);
#endif

    if (control_ep)
    {
        cdc->notify = block_create(cdc->control_ep_size);
        if (cdc->notify == INVALID_HANDLE)
        {
            cdc_destroy(cdc);
            return;
        }
        size = cdc->control_ep_size;
        fopen_p(cdc->usb, HAL_HANDLE(HAL_USB, USB_EP_IN | cdc->control_ep), USB_EP_INTERRUPT, (void*)size);
        cdc_notify_serial_state(cdc, CDC_SERIAL_STATE_DCD | CDC_SERIAL_STATE_DSR);
        usbd_register_interface(usbd, cdc->control_iface, &__CDC_CLASS, cdc);
        usbd_register_endpoint(usbd, cdc->control_iface, cdc->control_ep);
    }
    usbd_register_interface(usbd, cdc->data_iface, &__CDC_CLASS, cdc);
    usbd_register_endpoint(usbd, cdc->data_iface, cdc->data_ep);

    cdc->DTR = cdc->RTS = false;
    cdc->baud.baud = 115200;
    cdc->baud.data_bits = 8;
    cdc->baud.parity = 'N';
    cdc->baud.stop_bits = 1;
}

void cdc_class_reset(USBD* usbd, void* param)
{
    CDC* cdc = (CDC*)param;

#if (USB_CDC_TX_STREAM_SIZE)
    stream_stop_listen(cdc->tx_stream);
    stream_flush(cdc->tx_stream);
    fflush(cdc->usb, HAL_HANDLE(HAL_USB, USB_EP_IN | cdc->data_ep));
    fclose(cdc->usb, HAL_HANDLE(HAL_USB, USB_EP_IN | cdc->data_ep));
#endif
#if (USB_CDC_RX_STREAM_SIZE)
    stream_flush(cdc->rx_stream);
    fflush(cdc->usb, HAL_HANDLE(HAL_USB, cdc->data_ep));
    fclose(cdc->usb, HAL_HANDLE(HAL_USB, cdc->data_ep));
#endif

    usbd_unregister_endpoint(usbd, cdc->data_iface, cdc->data_ep);
    usbd_unregister_interface(usbd, cdc->data_iface, &__CDC_CLASS);
    if (cdc->control_ep)
    {
        fflush(cdc->usb, HAL_HANDLE(HAL_USB, USB_EP_IN | cdc->control_ep));
        fclose(cdc->usb, HAL_HANDLE(HAL_USB, USB_EP_IN | cdc->control_ep));
        usbd_unregister_endpoint(usbd, cdc->control_iface, cdc->control_ep);
        usbd_unregister_interface(usbd, cdc->control_iface, &__CDC_CLASS);
    }
    cdc_destroy(cdc);
}

void cdc_class_suspend(USBD* usbd, void* param)
{
    CDC* cdc = (CDC*)param;
#if (USB_CDC_TX_STREAM_SIZE)
    stream_stop_listen(cdc->tx_stream);
    stream_flush(cdc->tx_stream);
    fflush(cdc->usb, HAL_HANDLE(HAL_USB, USB_EP_IN | cdc->data_ep));
    cdc->tx_idle = true;
    cdc->tx_size = 0;
#endif
#if (USB_CDC_RX_STREAM_SIZE)
    stream_flush(cdc->rx_stream);
    fflush(cdc->usb, HAL_HANDLE(HAL_USB, cdc->data_ep));
    cdc->rx_free = 0;
#endif

    if (cdc->control_ep)
        fflush(cdc->usb, HAL_HANDLE(HAL_USB, USB_EP_IN | cdc->data_ep));
    cdc->suspended = true;
}

void cdc_class_resume(USBD* usbd, void* param)
{
    CDC* cdc = (CDC*)param;
    cdc->suspended = false;
#if (USB_CDC_RX_STREAM_SIZE)
    stream_listen(cdc->tx_stream, (void*)(HAL_HANDLE(HAL_USBD, USBD_HANDLE_INTERFACE + cdc->data_iface)));
#endif
}

static inline void cdc_read_complete(CDC* cdc, unsigned int size)
{
    if (cdc->suspended)
        return;

    unsigned int to_read;
    void* ptr;
    if (cdc->rx_free < size)
        cdc->rx_free = stream_get_free(cdc->rx_stream);
    to_read = size;
    if (to_read > cdc->rx_free)
        to_read = cdc->rx_free;
    ptr = block_open(cdc->rx);
#if (USB_DEBUG_CLASS_IO)
    int i;
    printf("CDC rx: ");
    for (i = 0; i < size; ++i)
        if (((uint8_t*)ptr)[i] >= ' ' && ((uint8_t*)ptr)[i] <= '~')
            printf("%c", ((char*)ptr)[i]);
        else
            printf("\\x%d", ((uint8_t*)ptr)[i]);
    printf("\n\r");
#endif //USB_DEBUG_CLASS_IO
    if (ptr && to_read && stream_write(cdc->rx_stream_handle, ptr, to_read))
        cdc->rx_free -= to_read;
    fread_async(cdc->usb, HAL_HANDLE(HAL_USB, cdc->data_ep), cdc->rx, 1);

    if (to_read < size)
        cdc_notify_serial_state(cdc, CDC_SERIAL_STATE_DCD | CDC_SERIAL_STATE_DSR | CDC_SERIAL_STATE_OVERRUN);
}

void cdc_write(CDC* cdc)
{
    if (!cdc->DTR || !cdc->tx_idle || cdc->suspended)
        return;

    unsigned int to_write;
    void* ptr;
    if (cdc->tx_size == 0)
        cdc->tx_size = stream_get_size(cdc->tx_stream);

    to_write = cdc->tx_size;
    if (to_write > cdc->data_ep_size)
        to_write = cdc->data_ep_size;
    if (to_write)
    {
        cdc->tx_size -= to_write;
        ptr = block_open(cdc->tx);
        if (ptr && stream_read(cdc->tx_stream_handle, ptr, to_write))
        {
            cdc->tx_idle = false;
            fwrite_async(cdc->usb, HAL_HANDLE(HAL_USB, USB_EP_IN | cdc->data_ep), cdc->tx, to_write);
        }
        //just in case of driver failure
        else
            stream_listen(cdc->tx_stream, (void*)(HAL_HANDLE(HAL_USBD, USBD_HANDLE_INTERFACE + cdc->data_iface)));
    }
    else
        stream_listen(cdc->tx_stream, (void*)(HAL_HANDLE(HAL_USBD, USBD_HANDLE_INTERFACE + cdc->data_iface)));
}

static inline int set_line_coding(CDC* cdc, HANDLE block)
{
    LINE_CODING_STRUCT* lc = block_open(block);
    if (lc == NULL)
        return -1;
    cdc->baud.baud = lc->dwDTERate;
    if (lc->bCharFormat >= LC_BAUD_STOP_BITS_SIZE)
        return -1;
    cdc->baud.stop_bits = LC_BAUD_STOP_BITS[lc->bCharFormat];
    if (lc->bParityType >= LC_BAUD_PARITY_SIZE)
        return -1;
    cdc->baud.parity = LC_BAUD_PARITY[lc->bParityType];
    cdc->baud.data_bits = lc->bDataBits;

#if (USB_DEBUG_CLASS_REQUESTS)
    printf("CDC set line coding: %d %d%c%d\n\r", cdc->baud.baud, cdc->baud.data_bits, cdc->baud.parity, cdc->baud.stop_bits);
#endif
    return 0;
}

static inline int get_line_coding(CDC* cdc, HANDLE block)
{
    LINE_CODING_STRUCT* lc = block_open(block);
    if (lc == NULL)
        return -1;
    lc->dwDTERate = cdc->baud.baud;
    switch (cdc->baud.stop_bits)
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
    switch (cdc->baud.parity)
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
    lc->bDataBits = cdc->baud.data_bits;

#if (USB_DEBUG_CLASS_REQUESTS)
    printf("CDC get line coding: %d %d%c%d\n\r", cdc->baud.baud, cdc->baud.data_bits, cdc->baud.parity, cdc->baud.stop_bits);
#endif
    return sizeof(LINE_CODING_STRUCT);
}

static inline int set_control_line_state(CDC* cdc, SETUP* setup)
{
    cdc->DTR = (setup->wValue >> 0) & 1;
    cdc->RTS = (setup->wValue >> 0) & 1;
#if (USB_DEBUG_CLASS_REQUESTS)
    printf("CDC set control line state: DTR %s, RTS %s\n\r", ON_OFF[1 * cdc->DTR], ON_OFF[1 * cdc->RTS]);
#endif

    //resume write if DTR is set
    if (cdc->DTR)
        cdc_write(cdc);
    //flush if not
    else if (!cdc->tx_idle)
    {
        fflush(cdc->usb, HAL_HANDLE(HAL_USB, USB_EP_IN | cdc->data_ep));
        cdc->tx_idle = true;
    }
    return 0;
}

int cdc_class_setup(USBD* usbd, void* param, SETUP* setup, HANDLE block)
{
    CDC* cdc = (CDC*)param;
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
        res = set_control_line_state(cdc, setup);
        break;
    }
    return res;
}



bool cdc_class_request(USBD* usbd, void* param, IPC* ipc)
{
    CDC* cdc = (CDC*)param;
    bool need_post = false;
    switch (ipc->cmd)
    {
    case IPC_READ_COMPLETE:
        cdc_read_complete(cdc, ipc->param3);
        break;
    case IPC_WRITE_COMPLETE:
        //ignore notify complete
        if (ipc->param1 == HAL_HANDLE(HAL_USB, (cdc->data_ep | USB_EP_IN)))
        {
            cdc->tx_idle = true;
            cdc_write(cdc);
        }
        break;
    case IPC_STREAM_WRITE:
        cdc->tx_size = ipc->param2;
        cdc_write(cdc);
        break;
    case IPC_GET_TX_STREAM:
        ipc->param1 = cdc->tx_stream;
        need_post = true;
        break;
    case IPC_GET_RX_STREAM:
        ipc->param1 = cdc->rx_stream;
        need_post = true;
        break;
    case USBD_INTERFACE_REQUEST:
        if (ipc->param2 == USB_CDC_SEND_BREAK)
        {
            cdc_notify_serial_state(cdc, CDC_SERIAL_STATE_DCD | CDC_SERIAL_STATE_DSR | CDC_SERIAL_STATE_BREAK);
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

const USBD_CLASS __CDC_CLASS = {
    cdc_class_configured,
    cdc_class_reset,
    cdc_class_suspend,
    cdc_class_resume,
    cdc_class_setup,
    cdc_class_request,
};
