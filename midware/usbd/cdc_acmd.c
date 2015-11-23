/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "cdc_acmd.h"
#include "usbd.h"
#include "../../userspace/sys.h"
#include "../../userspace/stdio.h"
#include "../../userspace/io.h"
#include "../../userspace/uart.h"
#include "../../userspace/stdlib.h"
#include "../../userspace/cdc.h"
#include "../../userspace/usb.h"
#include "sys_config.h"

#define CDC_BLOCK_SIZE                                                          64
#define CDC_NOTIFY_SIZE                                                         64

#define LC_BAUD_STOP_BITS_SIZE                                                  3
static const int LC_BAUD_STOP_BITS[LC_BAUD_STOP_BITS_SIZE] =                    {1, 15, 2};

#define LC_BAUD_PARITY_SIZE                                                     5
static const char LC_BAUD_PARITY[LC_BAUD_PARITY_SIZE] =                         "NOEMS";

#if (USBD_CDC_ACM_DEBUG)
const char* const ON_OFF[] =                                                    {"off", "on"};
#endif


typedef struct {
    IO* rx;
    IO* tx;
    IO* notify;
    HANDLE rx_stream, tx_stream;
    HANDLE tx_stream_handle, rx_stream_handle;
    uint8_t data_ep, control_ep;
    uint16_t data_ep_size, control_ep_size, rx_free, tx_size;
    uint8_t DTR, RTS, tx_idle, control_iface, data_iface;
    uint8_t suspended;
    bool notify_ready;
    BAUD baud;
} CDC_ACMD;

void cdc_acmd_notify_serial_state(USBD* usbd, CDC_ACMD* cdc_acmd, unsigned int state)
{
    if (!cdc_acmd->notify_ready)
        usbd_usb_ep_flush(usbd, USB_EP_IN | cdc_acmd->control_ep);

    SETUP* setup = io_data(cdc_acmd->notify);
    setup->bmRequestType = BM_REQUEST_DIRECTION_DEVICE_TO_HOST | BM_REQUEST_TYPE_CLASS | BM_REQUEST_RECIPIENT_INTERFACE;
    setup->bRequest = CDC_SERIAL_STATE;
    setup->wValue = 0;
    setup->wIndex = 1;
    setup->wLength = 2;
    uint16_t* serial_state = (uint16_t*)(io_data(cdc_acmd->notify) + sizeof(SETUP));
    *serial_state = state;
    cdc_acmd->notify->data_size = sizeof(SETUP) + 2;
    usbd_usb_ep_write(usbd, cdc_acmd->control_ep, cdc_acmd->notify);
}

void cdc_acmd_destroy(CDC_ACMD* cdc_acmd)
{
    io_destroy(cdc_acmd->notify);

    io_destroy(cdc_acmd->rx);
    stream_close(cdc_acmd->rx_stream_handle);
    stream_destroy(cdc_acmd->rx_stream);

    io_destroy(cdc_acmd->tx);
    stream_close(cdc_acmd->tx_stream_handle);
    stream_destroy(cdc_acmd->tx_stream);

    free(cdc_acmd);
}

void cdc_acmd_class_configured(USBD* usbd, USB_CONFIGURATION_DESCRIPTOR_TYPE* cfg)
{

    USB_INTERFACE_DESCRIPTOR_TYPE* iface;
    USB_ENDPOINT_DESCRIPTOR_TYPE* ep;
    uint8_t data_ep, control_ep, control_iface, data_iface;
    uint16_t data_ep_size, control_ep_size;
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
    CDC_ACMD* cdc_acmd = (CDC_ACMD*)malloc(sizeof(CDC_ACMD));
    if (cdc_acmd == NULL)
        return;

    cdc_acmd->control_iface = control_iface;
    cdc_acmd->control_ep = control_ep;
    cdc_acmd->control_ep_size = control_ep_size;
    cdc_acmd->data_iface = data_iface;
    cdc_acmd->data_ep = data_ep;
    cdc_acmd->data_ep_size = data_ep_size;
    cdc_acmd->tx = cdc_acmd->rx = cdc_acmd->notify = NULL;
    cdc_acmd->tx_stream = cdc_acmd->rx_stream = cdc_acmd->tx_stream_handle = cdc_acmd->rx_stream_handle = INVALID_HANDLE;
    cdc_acmd->suspended = false;
    cdc_acmd->notify_ready = true;

#if (USBD_CDC_ACM_DEBUG)
    printf("Found USB CDCD ACM class, EP%d, size: %d, iface: %d\n", cdc_acmd->data_ep, cdc_acmd->data_ep_size, cdc_acmd->data_iface);
    if (cdc_acmd->control_ep)
        printf("Has control EP%d, size: %d, iface: %d\n", cdc_acmd->control_ep, cdc_acmd->control_ep_size, cdc_acmd->control_iface);
#endif //USBD_CDC_ACM_DEBUG

#if (USBD_CDC_ACM_TX_STREAM_SIZE)
    cdc_acmd->tx = io_create(cdc_acmd->data_ep_size);
    cdc_acmd->tx_stream = stream_create(USBD_CDC_ACM_RX_STREAM_SIZE);
    cdc_acmd->tx_stream_handle = stream_open(cdc_acmd->tx_stream);
    if (cdc_acmd->tx == NULL || cdc_acmd->tx_stream_handle == INVALID_HANDLE)
    {
        cdc_acmd_destroy(cdc_acmd);
        return;
    }
    cdc_acmd->tx_size = 0;
    cdc_acmd->tx_idle = true;
    usbd_usb_ep_open(usbd, USB_EP_IN | cdc_acmd->data_ep, USB_EP_BULK, cdc_acmd->data_ep_size);
    stream_listen(cdc_acmd->tx_stream, USBD_IFACE(cdc_acmd->data_iface, 0), HAL_USBD_IFACE);
#endif //USBD_CDC_ACM_TX_STREAM_SIZE

#if (USBD_CDC_ACM_RX_STREAM_SIZE)
    cdc_acmd->rx = io_create(cdc_acmd->data_ep_size);
    cdc_acmd->rx_stream = stream_create(USBD_CDC_ACM_RX_STREAM_SIZE);
    cdc_acmd->rx_stream_handle = stream_open(cdc_acmd->rx_stream);
    if (cdc_acmd->rx == NULL || cdc_acmd->rx_stream_handle == INVALID_HANDLE)
    {
        cdc_acmd_destroy(cdc_acmd);
        return;
    }
    cdc_acmd->rx_free = 0;
    usbd_usb_ep_open(usbd, cdc_acmd->data_ep, USB_EP_BULK, cdc_acmd->data_ep_size);
    usbd_usb_ep_read(usbd, cdc_acmd->data_ep, cdc_acmd->rx, cdc_acmd->data_ep_size);
#endif //USBD_CDC_ACM_RX_STREAM_SIZE

    if (control_ep)
    {
        cdc_acmd->notify = io_create(cdc_acmd->control_ep_size);
        if (cdc_acmd->notify == NULL)
        {
            cdc_acmd_destroy(cdc_acmd);
            return;
        }
        usbd_usb_ep_open(usbd, USB_EP_IN | cdc_acmd->control_ep, USB_EP_INTERRUPT, cdc_acmd->control_ep_size);
        cdc_acmd_notify_serial_state(usbd, cdc_acmd, CDC_SERIAL_STATE_DCD | CDC_SERIAL_STATE_DSR);
        usbd_register_interface(usbd, cdc_acmd->control_iface, &__CDC_ACMD_CLASS, cdc_acmd);
        usbd_register_endpoint(usbd, cdc_acmd->control_iface, cdc_acmd->control_ep);
    }
    usbd_register_interface(usbd, cdc_acmd->data_iface, &__CDC_ACMD_CLASS, cdc_acmd);
    usbd_register_endpoint(usbd, cdc_acmd->data_iface, cdc_acmd->data_ep);

    cdc_acmd->DTR = cdc_acmd->RTS = false;
    cdc_acmd->baud.baud = 115200;
    cdc_acmd->baud.data_bits = 8;
    cdc_acmd->baud.parity = 'N';
    cdc_acmd->baud.stop_bits = 1;
}

void cdc_acmd_class_reset(USBD* usbd, void* param)
{
    CDC_ACMD* cdc_acmd = (CDC_ACMD*)param;

#if (USBD_CDC_ACM_TX_STREAM_SIZE)
    stream_stop_listen(cdc_acmd->tx_stream);
    stream_flush(cdc_acmd->tx_stream);
    usbd_usb_ep_close(usbd, USB_EP_IN | cdc_acmd->data_ep);
#endif //USBD_CDC_ACM_TX_STREAM_SIZE
#if (USBD_CDC_ACM_RX_STREAM_SIZE)
    stream_flush(cdc_acmd->rx_stream);
    usbd_usb_ep_close(usbd, cdc_acmd->data_ep);
#endif //USBD_CDC_ACM_RX_STREAM_SIZE

    usbd_unregister_endpoint(usbd, cdc_acmd->data_iface, cdc_acmd->data_ep);
    usbd_unregister_interface(usbd, cdc_acmd->data_iface, &__CDC_ACMD_CLASS);
    if (cdc_acmd->control_ep)
    {
        usbd_usb_ep_close(usbd, USB_EP_IN | cdc_acmd->control_ep);
        usbd_unregister_endpoint(usbd, cdc_acmd->control_iface, cdc_acmd->control_ep);
        usbd_unregister_interface(usbd, cdc_acmd->control_iface, &__CDC_ACMD_CLASS);
    }
    cdc_acmd_destroy(cdc_acmd);
}

void cdc_acmd_class_suspend(USBD* usbd, void* param)
{
    CDC_ACMD* cdc_acmd = (CDC_ACMD*)param;
#if (USBD_CDC_ACM_TX_STREAM_SIZE)
    stream_stop_listen(cdc_acmd->tx_stream);
    stream_flush(cdc_acmd->tx_stream);
    usbd_usb_ep_flush(usbd, USB_EP_IN | cdc_acmd->data_ep);
    cdc_acmd->tx_idle = true;
    cdc_acmd->tx_size = 0;
#endif //USBD_CDC_ACM_TX_STREAM_SIZE
#if (USBD_CDC_ACM_RX_STREAM_SIZE)
    stream_flush(cdc_acmd->rx_stream);
    usbd_usb_ep_flush(usbd, cdc_acmd->data_ep);
    cdc_acmd->rx_free = 0;
#endif //USBD_CDC_ACM_RX_STREAM_SIZE

    if (cdc_acmd->control_ep)
        usbd_usb_ep_flush(usbd, USB_EP_IN | cdc_acmd->control_ep);
    cdc_acmd->suspended = true;
}

void cdc_acmd_class_resume(USBD* usbd, void* param)
{
    CDC_ACMD* cdc_acmd = (CDC_ACMD*)param;
    cdc_acmd->suspended = false;
#if (USBD_CDC_ACM_RX_STREAM_SIZE)
    stream_listen(cdc_acmd->tx_stream, USBD_IFACE(cdc_acmd->data_iface, 0), HAL_USBD_IFACE);
    usbd_usb_ep_read(usbd, cdc_acmd->data_ep, cdc_acmd->rx, cdc_acmd->data_ep_size);
#endif //USBD_CDC_ACM_RX_STREAM_SIZE
}

static inline void cdc_acmd_read_complete(USBD* usbd, CDC_ACMD* cdc_acmd)
{
    if (cdc_acmd->suspended)
        return;

    unsigned int to_read;
    if (cdc_acmd->rx_free < cdc_acmd->rx->data_size)
        cdc_acmd->rx_free = stream_get_free(cdc_acmd->rx_stream);
    to_read = cdc_acmd->rx->data_size;
    if (to_read > cdc_acmd->rx_free)
        to_read = cdc_acmd->rx_free;
    if (to_read < cdc_acmd->rx->data_size)
        cdc_acmd_notify_serial_state(usbd, cdc_acmd, CDC_SERIAL_STATE_DCD | CDC_SERIAL_STATE_DSR | CDC_SERIAL_STATE_OVERRUN);
#if (USBD_CDC_ACM_DEBUG_FLOW)
    int i;
    printf("CDC_ACMD rx: ");
    for (i = 0; i < cdc_acmd->rx->data_size; ++i)
        if (((uint8_t*)io_data(cdc_acmd->rx))[i] >= ' ' && ((uint8_t*)io_data(cdc_acmd->rx))[i] <= '~')
            printf("%c", ((char*)io_data(cdc_acmd->rx))[i]);
        else
            printf("\\x%d", ((uint8_t*)io_data(cdc_acmd->rx))[i]);
    printf("\n");
#endif //USBD_CDC_ACM_DEBUG_FLOW
    if (to_read && stream_write(cdc_acmd->rx_stream_handle, io_data(cdc_acmd->rx), to_read))
        cdc_acmd->rx_free -= to_read;
    usbd_usb_ep_read(usbd, cdc_acmd->data_ep, cdc_acmd->rx, cdc_acmd->data_ep_size);
}

void cdc_acmd_write(USBD* usbd, CDC_ACMD* cdc_acmd)
{
    if (!cdc_acmd->DTR || !cdc_acmd->tx_idle || cdc_acmd->suspended)
        return;

    unsigned int to_write;
    if (cdc_acmd->tx_size == 0)
        cdc_acmd->tx_size = stream_get_size(cdc_acmd->tx_stream);

    to_write = cdc_acmd->tx_size;
    if (to_write > cdc_acmd->data_ep_size)
        to_write = cdc_acmd->data_ep_size;
    if (to_write)
    {
        cdc_acmd->tx_size -= to_write;
        if (stream_read(cdc_acmd->tx_stream_handle, io_data(cdc_acmd->tx), to_write))
        {
            cdc_acmd->tx_idle = false;
            cdc_acmd->tx->data_size = to_write;
            usbd_usb_ep_write(usbd, cdc_acmd->data_ep, cdc_acmd->tx);
        }
        //just in case of driver failure
        else
            stream_listen(cdc_acmd->tx_stream, USBD_IFACE(cdc_acmd->data_iface, 0), HAL_USBD_IFACE);
    }
    else
        stream_listen(cdc_acmd->tx_stream, USBD_IFACE(cdc_acmd->data_iface, 0), HAL_USBD_IFACE);
}

#if (USBD_CDC_ACM_FLOW_CONTROL)

static inline int set_line_coding(CDC_ACMD* cdc_acmd, IO* io)
{
    LINE_CODING_STRUCT* lc = io_data(io);
    cdc_acmd->baud.baud = lc->dwDTERate;
    if (lc->bCharFormat >= LC_BAUD_STOP_BITS_SIZE)
        return -1;
    cdc_acmd->baud.stop_bits = LC_BAUD_STOP_BITS[lc->bCharFormat];
    if (lc->bParityType >= LC_BAUD_PARITY_SIZE)
        return -1;
    cdc_acmd->baud.parity = LC_BAUD_PARITY[lc->bParityType];
    cdc_acmd->baud.data_bits = lc->bDataBits;

#if (USBD_CDC_ACM_DEBUG)
    printf("CDC_ACMD set line coding: %d %d%c%d\n", cdc_acmd->baud.baud, cdc_acmd->baud.data_bits, cdc_acmd->baud.parity, cdc_acmd->baud.stop_bits);
#endif
    return 0;
}

static inline int get_line_coding(CDC_ACMD* cdc_acmd, IO* io)
{
    LINE_CODING_STRUCT* lc = io_data(io);
    lc->dwDTERate = cdc_acmd->baud.baud;
    switch (cdc_acmd->baud.stop_bits)
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
    switch (cdc_acmd->baud.parity)
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
    lc->bDataBits = cdc_acmd->baud.data_bits;

#if (USBD_CDC_ACM_DEBUG)
    printf("CDC_ACMD get line coding: %d %d%c%d\n", cdc_acmd->baud.baud, cdc_acmd->baud.data_bits, cdc_acmd->baud.parity, cdc_acmd->baud.stop_bits);
#endif
    return sizeof(LINE_CODING_STRUCT);
}

static inline int set_control_line_state(USBD* usbd, CDC_ACMD* cdc_acmd, SETUP* setup)
{
    cdc_acmd->DTR = (setup->wValue >> 0) & 1;
    cdc_acmd->RTS = (setup->wValue >> 0) & 1;
#if (USBD_CDC_ACM_DEBUG)
    printf("CDC_ACMD set control line state: DTR %s, RTS %s\n", ON_OFF[1 * cdc_acmd->DTR], ON_OFF[1 * cdc_acmd->RTS]);
#endif

    //resume write if DTR is set
    if (cdc_acmd->DTR)
        cdc_acmd_write(usbd, cdc_acmd);
    //flush if not
    else if (!cdc_acmd->tx_idle)
    {
        usbd_usb_ep_flush(usbd, USB_EP_IN | cdc_acmd->data_ep);
        cdc_acmd->tx_idle = true;
    }
    return 0;
}
#endif //USBD_CDC_ACM_FLOW_CONTROL

int cdc_acmd_class_setup(USBD* usbd, void* param, SETUP* setup, IO* io)
{
    CDC_ACMD* cdc = (CDC_ACMD*)param;
    int res = -1;
    switch (setup->bRequest)
    {
#if (USBD_CDC_ACM_FLOW_CONTROL)
    case SET_LINE_CODING:
        res = set_line_coding(cdc, io);
        break;
    case GET_LINE_CODING:
        res = get_line_coding(cdc, io);
        break;
    case SET_CONTROL_LINE_STATE:
        res = set_control_line_state(usbd, cdc, setup);
        break;
#endif //USBD_CDC_ACM_FLOW_CONTROL
    default:
        //TODO: RNDIS here
//        dump(setup, 8);
//        dump(io_data(io), 0x18);
        break;
    }
    return res;
}

static inline void cdc_acmd_driver_event(USBD* usbd, CDC_ACMD* cdc_acmd, IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_READ:
        cdc_acmd_read_complete(usbd, cdc_acmd);
        break;
    case IPC_WRITE:
        if (ipc->param1 == (cdc_acmd->data_ep | USB_EP_IN))
        {
            cdc_acmd->tx_idle = true;
            cdc_acmd_write(usbd, cdc_acmd);
        }
        else
            cdc_acmd->notify_ready = true;
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
    }
}

void cdc_acmd_class_request(USBD* usbd, void* param, IPC* ipc)
{
    CDC_ACMD* cdc_acmd = (CDC_ACMD*)param;
    if (HAL_GROUP(ipc->cmd) == HAL_USB)
        cdc_acmd_driver_event(usbd, cdc_acmd, ipc);
    else
        switch (HAL_ITEM(ipc->cmd))
        {
        case IPC_STREAM_WRITE:
            cdc_acmd->tx_size = ipc->param3;
            cdc_acmd_write(usbd, cdc_acmd);
            break;
        case IPC_GET_TX_STREAM:
            ipc->param2 = cdc_acmd->tx_stream;
            break;
        case IPC_GET_RX_STREAM:
            ipc->param2 = cdc_acmd->rx_stream;
            break;
        case USB_CDC_SEND_BREAK:
            cdc_acmd_notify_serial_state(usbd, cdc_acmd, CDC_SERIAL_STATE_DCD | CDC_SERIAL_STATE_DSR | CDC_SERIAL_STATE_BREAK);
            break;
        default:
            error(ERROR_NOT_SUPPORTED);
        }
}

const USBD_CLASS __CDC_ACMD_CLASS = {
    cdc_acmd_class_configured,
    cdc_acmd_class_reset,
    cdc_acmd_class_suspend,
    cdc_acmd_class_resume,
    cdc_acmd_class_setup,
    cdc_acmd_class_request,
};
