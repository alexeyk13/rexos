/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "cdc.h"
#include "../sys.h"
#include "../file.h"
#include "usbd.h"
#include "../../userspace/lib/stdio.h"
#include "../../userspace/block.h"
#include "../../userspace/direct.h"
#include "../uart.h"
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
    SETUP setup;
    HANDLE usbd, usb;
    HANDLE rx, tx, notify, rx_stream, tx_stream;
    HANDLE tx_stream_handle, rx_stream_handle;
    uint8_t data_ep, control_ep;
    uint16_t data_ep_size, control_ep_size, rx_free, tx_size;
    uint8_t DTR, RTS, tx_idle;
    uint8_t suspended, configured;
    BAUD baud;
} CDC;

void cdc();

const REX __CDC = {
    //name
    "CDC USB class",
    //size
    USB_CDC_PROCESS_SIZE,
    //priority - midware priority
    150,
    //flags
    PROCESS_FLAGS_ACTIVE | REX_HEAP_FLAGS(HEAP_PERSISTENT_NAME),
    //ipc size
    10,
    //function
    cdc
};

static inline void cdc_open(CDC* cdc, HANDLE usbd, CDC_OPEN_STRUCT* cos)
{
    if (cdc->usbd != INVALID_HANDLE)
    {
        error(ERROR_ALREADY_CONFIGURED);
        return;
    }
    cdc->tx = block_create(cos->data_ep_size);
    cdc->rx = block_create(cos->data_ep_size);
    cdc->notify = block_create(cos->control_ep_size);
    cdc->rx_stream = stream_create(cos->rx_stream_size);
    if (cdc->rx_stream != INVALID_HANDLE)
        cdc->rx_stream_handle = stream_open(cdc->rx_stream);
    cdc->tx_stream = stream_create(cos->tx_stream_size);
    if (cdc->tx_stream != INVALID_HANDLE)
        cdc->tx_stream_handle = stream_open(cdc->tx_stream);
    if (cdc->tx == INVALID_HANDLE || cdc->rx == INVALID_HANDLE || cdc->notify == INVALID_HANDLE ||
        cdc->rx_stream == INVALID_HANDLE || cdc->tx_stream == INVALID_HANDLE ||
        cdc->rx_stream_handle == INVALID_HANDLE || cdc->tx_stream_handle == INVALID_HANDLE)
    {
        block_destroy(cdc->tx);
        cdc->tx = INVALID_HANDLE;
        block_destroy(cdc->rx);
        cdc->rx = INVALID_HANDLE;
        block_destroy(cdc->notify);
        cdc->notify = INVALID_HANDLE;
        stream_close(cdc->rx_stream_handle);
        cdc->rx_stream_handle = INVALID_HANDLE;
        stream_destroy(cdc->rx_stream);
        cdc->rx_stream = INVALID_HANDLE;
        stream_close(cdc->tx_stream_handle);
        cdc->tx_stream_handle = INVALID_HANDLE;
        stream_destroy(cdc->tx_stream);
        cdc->tx_stream = INVALID_HANDLE;
        error(ERROR_OUT_OF_PAGED_MEMORY);
        return;
    }
    cdc->usbd = usbd;
    ack(cdc->usbd, USBD_REGISTER_CLASS, 0, 0, 0);
    cdc->usb = get(cdc->usbd, USBD_GET_DRIVER, 0, 0, 0);
    cdc->data_ep = cos->data_ep;
    cdc->control_ep = cos->control_ep;
    cdc->data_ep_size = cos->data_ep_size;
    cdc->control_ep_size = cos->control_ep_size;

}

static inline void cdc_close(CDC* cdc)
{
    if (cdc->usbd == INVALID_HANDLE)
    {
        error(ERROR_NOT_CONFIGURED);
        return;
    }

    if (cdc->configured)
    {
        fclose(cdc->usb, HAL_HANDLE(HAL_USB, cdc->data_ep));
        fclose(cdc->usb, HAL_HANDLE(HAL_USB, USB_EP_IN | cdc->data_ep));
        fclose(cdc->usb, HAL_HANDLE(HAL_USB, USB_EP_IN | cdc->control_ep));
        cdc->configured = false;
    }

    block_destroy(cdc->tx);
    cdc->tx = INVALID_HANDLE;
    block_destroy(cdc->rx);
    cdc->rx = INVALID_HANDLE;
    block_destroy(cdc->notify);
    cdc->notify = INVALID_HANDLE;

    stream_close(cdc->rx_stream_handle);
    cdc->rx_stream_handle = INVALID_HANDLE;
    stream_destroy(cdc->rx_stream);
    cdc->rx_stream = INVALID_HANDLE;
    stream_close(cdc->tx_stream_handle);
    cdc->tx_stream_handle = INVALID_HANDLE;
    stream_destroy(cdc->tx_stream);
    cdc->tx_stream = INVALID_HANDLE;

    ack(cdc->usbd, USBD_UNREGISTER_CLASS, 0, 0, 0);
    cdc->usbd = INVALID_HANDLE;
    cdc->usb = INVALID_HANDLE;
}

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
            stream_listen(cdc->tx_stream, (void*)0);
    }
    else
        stream_listen(cdc->tx_stream, (void*)0);
}

static inline void cdc_reset(CDC* cdc)
{
    cdc->suspended = false;
    if (cdc->configured)
    {
        fclose(cdc->usb, HAL_HANDLE(HAL_USB, cdc->data_ep));
        fclose(cdc->usb, HAL_HANDLE(HAL_USB, USB_EP_IN | cdc->data_ep));
        fclose(cdc->usb, HAL_HANDLE(HAL_USB, USB_EP_IN | cdc->control_ep));
        cdc->configured = false;
    }
    cdc->DTR = cdc->RTS = false;
    cdc->baud.baud = 115200;
    cdc->baud.data_bits = 8;
    cdc->baud.parity = 'N';
    cdc->baud.stop_bits = 1;

    stream_flush(cdc->rx_stream);
    stream_flush(cdc->tx_stream);
    cdc->rx_free = stream_get_free(cdc->rx_stream);
    cdc->tx_size = 0;
}

static inline void cdc_configured(CDC* cdc)
{
    USB_EP_OPEN ep_open;
    //data
    ep_open.size = cdc->data_ep_size;
    ep_open.type = USB_EP_BULK;
    fopen_ex(cdc->usb, HAL_HANDLE(HAL_USB, cdc->data_ep), 0, (void*)&ep_open, sizeof(USB_EP_OPEN));
    fopen_ex(cdc->usb, HAL_HANDLE(HAL_USB, USB_EP_IN | cdc->data_ep), 0, (void*)&ep_open, sizeof(USB_EP_OPEN));

    //control
    ep_open.size = cdc->control_ep_size;
    ep_open.type = USB_EP_INTERRUPT;
    fopen_ex(cdc->usb, HAL_HANDLE(HAL_USB, USB_EP_IN | cdc->control_ep), 0, (void*)&ep_open, sizeof(USB_EP_OPEN));

    fread_async(cdc->usb, HAL_HANDLE(HAL_USB, cdc->data_ep), cdc->rx, 1);
    cdc_notify_serial_state(cdc, CDC_SERIAL_STATE_DCD | CDC_SERIAL_STATE_DSR);
    cdc->configured = true;
}

static inline void cdc_suspend(CDC* cdc)
{
    cdc->suspended = true;
    stream_stop_listen(cdc->rx_stream);
    if (cdc->configured)
    {
        fflush(cdc->usb, HAL_HANDLE(HAL_USB, USB_EP_IN | cdc->control_ep));
        fflush(cdc->usb, HAL_HANDLE(HAL_USB, cdc->data_ep));
        fflush(cdc->usb, HAL_HANDLE(HAL_USB, USB_EP_IN | cdc->data_ep));
    }
    cdc->tx_idle = true;
    cdc->rx_free = cdc->tx_size = 0;
}

static inline void cdc_resume(CDC* cdc)
{
    cdc->suspended = false;
    stream_listen(cdc->rx_stream, (void*)0);
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

static inline int set_control_line_state(CDC* cdc)
{
    cdc->DTR = (cdc->setup.wValue >> 0) & 1;
    cdc->RTS = (cdc->setup.wValue >> 0) & 1;
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

static inline void cdc_setup(CDC* cdc, HANDLE block)
{
    int res = -1;
    switch (cdc->setup.bRequest)
    {
    case SET_LINE_CODING:
        res = set_line_coding(cdc, block);
        break;
    case GET_LINE_CODING:
        res = get_line_coding(cdc, block);
        break;
    case SET_CONTROL_LINE_STATE:
        res = set_control_line_state(cdc);
        break;
    }
#if (USB_DEBUG_ERRORS)
    if (res < 0)
        printf("Unhandled CDC request: %#X\n\r", cdc->setup.bRequest);
#endif
    block_send_ipc_inline(block, cdc->usbd, USB_SETUP, (unsigned int)res, 0, 0);
}

static inline void usbd_alert(CDC* cdc, unsigned int cmd, unsigned int param1, unsigned int param2)
{
    switch (cmd)
    {
    case USBD_ALERT_RESET:
        cdc_reset(cdc);
        break;
    case USBD_ALERT_SUSPEND:
        cdc_suspend(cdc);
        break;
    case USBD_ALERT_WAKEUP:
        cdc_resume(cdc);
        break;
    case USBD_ALERT_CONFIGURATION_SET:
        cdc_configured(cdc);
        break;
#if (USB_DEBUG_ERRORS)
    default:
        printf("Unknown USB device alert: %d\n\r", cmd);
#endif
    }
}

#if (SYS_INFO)
static inline void cdc_info(CDC* cdc)
{
    printf("USB CDC info\n\r\n\r");
    printf("DTR %s, RTS %s\n\r", ON_OFF[1 * cdc->DTR], ON_OFF[1 * cdc->RTS]);
    printf("line coding: %d %d%c%d\n\r", cdc->baud.baud, cdc->baud.data_bits, cdc->baud.parity, cdc->baud.stop_bits);
}
#endif

void cdc()
{
    IPC ipc;
    CDC cdc;
    open_stdout();
    CDC_OPEN_STRUCT cdc_open_struct;

    cdc.usbd = cdc.usb = cdc.rx = cdc.tx = cdc.notify = INVALID_HANDLE;
    cdc.rx_stream = cdc.tx_stream = INVALID_HANDLE;
    cdc.rx_stream_handle = cdc.tx_stream_handle = INVALID_HANDLE;
    cdc.data_ep = cdc.control_ep = 0;
    cdc.data_ep_size = cdc.control_ep_size = 0;
    cdc.rx_free = cdc.tx_size = 0;
    cdc.tx_idle = true;
    cdc.suspended = false;
    cdc.configured = false;

    for (;;)
    {
        ipc_read_ms(&ipc, 0, 0);
        error(ERROR_OK);
        switch (ipc.cmd)
        {
        case IPC_PING:
            ipc_post(&ipc);
            break;
#if (SYS_INFO)
        case IPC_GET_INFO:
            cdc_info(&cdc);
            ipc_post(&ipc);
            break;
#endif
        case IPC_OPEN:
            if (direct_read(ipc.process, (void*)&cdc_open_struct, sizeof(CDC_OPEN_STRUCT)))
                cdc_open(&cdc, (HANDLE)ipc.param1, &cdc_open_struct);
            ipc_post_or_error(&ipc);
            break;
        case IPC_CLOSE:
            cdc_close(&cdc);
            ipc_post_or_error(&ipc);
            break;
        case USBD_ALERT:
            usbd_alert(&cdc, ipc.param1, ipc.param2, ipc.param3);
            //no response required
            break;
        case USB_SETUP:
            ((uint32_t*)(&cdc.setup))[0] = ipc.param1;
            ((uint32_t*)(&cdc.setup))[1] = ipc.param2;
            cdc_setup(&cdc, ipc.param3);
            break;
        case IPC_READ_COMPLETE:
            cdc_read_complete(&cdc, ipc.param3);
            break;
        case IPC_STREAM_WRITE:
            cdc.tx_size = ipc.param2;
            cdc_write(&cdc);
            break;
        case IPC_WRITE_COMPLETE:
            //ignore notify complete
            if (ipc.param1 == HAL_HANDLE(HAL_USB, (cdc.data_ep | USB_EP_IN)))
            {
                cdc.tx_idle = true;
                cdc_write(&cdc);
            }
            break;
        case IPC_GET_TX_STREAM:
            ipc.param1 = cdc.tx_stream;
            ipc_post(&ipc);
            break;
        case IPC_GET_RX_STREAM:
            ipc.param1 = cdc.rx_stream;
            ipc_post(&ipc);
            break;
        case CDC_SEND_BREAK:
            cdc_notify_serial_state(&cdc, CDC_SERIAL_STATE_DCD | CDC_SERIAL_STATE_DSR | CDC_SERIAL_STATE_BREAK);
            ipc_post_or_error(&ipc);
            break;
        default:
            ipc_post_error(ipc.process, ERROR_NOT_SUPPORTED);
            break;
        }
    }
}

