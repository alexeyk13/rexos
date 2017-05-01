/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "rndisd.h"
#include "usbd.h"
#include "../../userspace/rndis.h"
#include "../../userspace/sys.h"
#include "../../userspace/mac.h"
#include "../../userspace/eth.h"
#include "../../userspace/stdio.h"
#include "../../userspace/io.h"
#include "../../userspace/uart.h"
#include "../../userspace/stdlib.h"
#include "../../userspace/cdc_acm.h"
#include "../../userspace/usb.h"
#include <string.h>
#include "sys_config.h"

#define RNDIS_RESPONSE_SIZE                                                     150

#define RNDIS_GEN_SUPPORTED_LIST_COUNT                                          26
#define RNDIS_GEN_SUPPORTED_LIST_SIZE                                           (RNDIS_GEN_SUPPORTED_LIST_COUNT * sizeof(uint32_t))

#pragma pack(push, 1)
static const uint32_t __RNDIS_GEN_SUPPORTED_LIST[RNDIS_GEN_SUPPORTED_LIST_COUNT]= {
                                                                                //General OIDs
                                                                                OID_GEN_SUPPORTED_LIST,
                                                                                OID_GEN_HARDWARE_STATUS,
                                                                                OID_GEN_MEDIA_SUPPORTED,
                                                                                OID_GEN_MEDIA_IN_USE,
                                                                                OID_GEN_MAXIMUM_FRAME_SIZE,
                                                                                OID_GEN_LINK_SPEED,
                                                                                OID_GEN_TRANSMIT_BLOCK_SIZE,
                                                                                OID_GEN_RECEIVE_BLOCK_SIZE,
                                                                                OID_GEN_VENDOR_ID,
                                                                                OID_GEN_VENDOR_DESCRIPTION,
                                                                                OID_GEN_CURRENT_PACKET_FILTER,
                                                                                OID_GEN_MAXIMUM_TOTAL_SIZE,
                                                                                OID_GEN_MEDIA_CONNECT_STATUS,
                                                                                OID_GEN_PHYSICAL_MEDIUM,
                                                                                // General Statistics OIDs
                                                                                OID_GEN_XMIT_OK,
                                                                                OID_GEN_RCV_OK,
                                                                                OID_GEN_XMIT_ERROR,
                                                                                OID_GEN_RCV_ERROR,
                                                                                OID_GEN_RCV_NO_BUFFER,
                                                                                //802.3 NDIS OIDs
                                                                                OID_802_3_PERMANENT_ADDRESS,
                                                                                OID_802_3_CURRENT_ADDRESS,
                                                                                OID_802_3_MULTICAST_LIST,
                                                                                OID_802_3_MAXIMUM_LIST_SIZE,
                                                                                //802.3 Statistics NDIS OIDs
                                                                                OID_802_3_RCV_ERROR_ALIGNMENT,
                                                                                OID_802_3_XMIT_ONE_COLLISION,
                                                                                OID_802_3_XMIT_MORE_COLLISIONS,
                                                                                   };
#pragma pack(pop)

typedef struct {
    IO* usb_notify;
    IO* tx_cur;
    IO* rx_cur;
#if (ETH_DOUBLE_BUFFERING)
    IO* tx;
    IO* rx;
#else
    IO* rx;
#endif
    unsigned int transfer_size, tx_ok, rx_ok, rx_size;
    uint32_t packet_filter, vendor_id;
    char* vendor;
    ETH_CONN_TYPE conn;
    MAC eth, host;
    HANDLE tcpip;
    uint8_t response[RNDIS_RESPONSE_SIZE];
    uint8_t response_size;
    uint8_t data_ep, control_ep;
    uint16_t data_ep_size;
    uint8_t data_iface, control_iface;
    bool notify_busy, link_status_queued, connected, zlp, initialized;
} RNDISD;

static void rndisd_destroy(RNDISD* rndisd)
{
    free(rndisd);
}

static void rndisd_flush(USBD* usbd, RNDISD* rndisd)
{
    usbd_usb_ep_flush(usbd, USB_EP_IN | rndisd->control_ep);
    usbd_usb_ep_flush(usbd, USB_EP_IN | rndisd->data_ep);
    usbd_usb_ep_flush(usbd, rndisd->data_ep);
#if (ETH_DOUBLE_BUFFERING)
    if (rndisd->tx != NULL)
    {
        io_complete_ex(rndisd->tcpip, HAL_IO_CMD(HAL_ETH, IPC_WRITE), USBD_IFACE(rndisd->control_iface, 0), rndisd->tx, ERROR_IO_CANCELLED);
        rndisd->tx = NULL;
    }
    if (rndisd->rx != NULL)
    {
        io_complete_ex(rndisd->tcpip, HAL_IO_CMD(HAL_ETH, IPC_READ), USBD_IFACE(rndisd->control_iface, 0), rndisd->rx, ERROR_IO_CANCELLED);
        rndisd->rx = NULL;
    }
#endif //ETH_DOUBLE_BUFFERING
    if (rndisd->rx_cur != NULL)
    {
        io_complete_ex(rndisd->tcpip, HAL_IO_CMD(HAL_ETH, IPC_READ), USBD_IFACE(rndisd->control_iface, 0), rndisd->rx_cur, ERROR_IO_CANCELLED);
        rndisd->rx_cur = NULL;
    }
    if (rndisd->tx_cur != NULL)
    {
        io_complete_ex(rndisd->tcpip, HAL_IO_CMD(HAL_ETH, IPC_WRITE), USBD_IFACE(rndisd->control_iface, 0), rndisd->tx_cur, ERROR_IO_CANCELLED);
        rndisd->tx_cur = NULL;
    }
    rndisd->link_status_queued = rndisd->notify_busy = false;
}

static void rndisd_notify(USBD* usbd, RNDISD* rndisd)
{
    RNDIS_RESPONSE_AVAILABLE_NOTIFY* resp;
    resp = io_data(rndisd->usb_notify);
    resp->notify = RNDIS_RESPONSE_AVAILABLE;
    resp->reserved = 0;
    rndisd->usb_notify->data_size = sizeof(RNDIS_RESPONSE_AVAILABLE_NOTIFY);
    usbd_usb_ep_write(usbd, USB_EP_IN | rndisd->control_ep, rndisd->usb_notify);
}

static inline bool rndisd_link_ready(RNDISD* rndisd)
{
    return (rndisd->tcpip != INVALID_HANDLE) && rndisd->initialized && rndisd->connected;
}

static void rndisd_indicate_status(RNDISD* rndisd)
{
    RNDIS_INDICATE_STATUS_MSG* msg;
    if (rndisd->notify_busy)
    {
        rndisd->link_status_queued = true;
        return;
    }
    msg = (RNDIS_INDICATE_STATUS_MSG*)rndisd->response;
    msg->message_type = REMOTE_NDIS_INDICATE_STATUS_MSG;
    rndisd->response_size = msg->message_length = sizeof(RNDIS_INDICATE_STATUS_MSG);
    msg->status = rndisd_link_ready(rndisd) ? RNDIS_STATUS_MEDIA_CONNECT : RNDIS_STATUS_MEDIA_DISCONNECT;
    msg->information_buffer_length = msg->information_buffer_offset = 0;
}

static void rndisd_link_changed(USBD* usbd, RNDISD* rndisd)
{
    if (rndisd_link_ready(rndisd))
    {
        rndisd_indicate_status(rndisd);
        ipc_post_inline(rndisd->tcpip, HAL_CMD(HAL_ETH, ETH_NOTIFY_LINK_CHANGED), USBD_IFACE(rndisd->control_iface, 0), rndisd->conn, 0);
    }
    else
    {
        rndisd_flush(usbd, rndisd);
        if (rndisd->tcpip != INVALID_HANDLE)
            ipc_post_inline(rndisd->tcpip, HAL_CMD(HAL_ETH, ETH_NOTIFY_LINK_CHANGED), USBD_IFACE(rndisd->control_iface, 0), ETH_NO_LINK, 0);
        if (rndisd->initialized)
            rndisd_indicate_status(rndisd);
    }
}

static void rndisd_reset(USBD* usbd, RNDISD* rndisd)
{
    bool was_ready = rndisd_link_ready(rndisd);
    rndisd->initialized = false;
    rndisd->packet_filter = RNDIS_PACKET_TYPE_DIRECTED | RNDIS_PACKET_TYPE_ALL_MULTICAST | RNDIS_PACKET_TYPE_BROADCAST | RNDIS_PACKET_TYPE_PROMISCUOUS;
    rndisd->tx_ok = rndisd->rx_ok = 0;
    if (was_ready)
        rndisd_link_changed(usbd, rndisd);
}

static void rndisd_fail(RNDISD* rndisd)
{
    rndisd->response[0] = 0x00;
    rndisd->response_size = 1;
}

void rndisd_class_configured(USBD* usbd, USB_CONFIGURATION_DESCRIPTOR* cfg)
{
    USB_INTERFACE_DESCRIPTOR* iface;
    USB_INTERFACE_DESCRIPTOR* diface;
    CDC_UNION_DESCRIPTOR* u;
    USB_ENDPOINT_DESCRIPTOR* ep;
    uint8_t data_ep, control_ep, data_iface, control_iface;
    uint16_t data_ep_size, control_ep_size;

    //check control/data ep here
    for (iface = usb_get_first_interface(cfg); iface != NULL; iface = usb_get_next_interface(cfg, iface))
    {
        //RNDIS only
        if ((iface->bInterfaceClass != CDC_COMM_INTERFACE_CLASS) || (iface->bInterfaceSubClass != CDC_ACM) || (iface->bInterfaceProtocol != CDC_CP_VENDOR))
            continue;
        control_iface = iface->bInterfaceNumber;
#if (USBD_RNDIS_DEBUG)
        printf("Found RNDIS device interface: %d\n", control_iface);
#endif //USBD_RNDIS_DEBUG

        ep = (USB_ENDPOINT_DESCRIPTOR*)usb_interface_get_first_descriptor(cfg, iface, USB_ENDPOINT_DESCRIPTOR_TYPE);
        if (ep == NULL)
        {
#if (USBD_RNDIS_DEBUG)
            printf("RNDIS device: Warning no control EP, skipping interface\n");
#endif //USBD_RNDIS_DEBUG
            continue;
        }
        control_ep = USB_EP_NUM(ep->bEndpointAddress);
        control_ep_size = ep->wMaxPacketSize;

        //find union descriptor
        for (u = usb_interface_get_first_descriptor(cfg, iface, CS_INTERFACE); u != NULL; u = usb_interface_get_next_descriptor(cfg, u, CS_INTERFACE))
        {
            if ((u->bDescriptorSybType == CDC_DESCRIPTOR_UNION) && (u->bControlInterface == iface->bInterfaceNumber) &&
                (u->bFunctionLength > sizeof(CDC_UNION_DESCRIPTOR)))
                break;
        }
        if (u == NULL)
        {
#if (USBD_RNDIS_DEBUG)
            printf("RNDIS device: Warning - no UNION descriptor, skipping interface\n");
#endif //USBD_RNDIS_DEBUG
            continue;
        }
        data_iface = ((uint8_t*)u)[sizeof(CDC_UNION_DESCRIPTOR)];
        diface = usb_find_interface(cfg, data_iface);
        if (diface == NULL)
        {
#if (USBD_RNDIS_DEBUG)
            printf("RNDIS device: Warning no data interface\n");
#endif //USBD_RNDIS_DEBUG
            continue;
        }
#if (USBD_RNDIS_DEBUG)
        printf("Found RNDIS device data interface: %d\n", data_iface);
#endif //USBD_RNDIS_DEBUG

        ep = (USB_ENDPOINT_DESCRIPTOR*)usb_interface_get_first_descriptor(cfg, diface, USB_ENDPOINT_DESCRIPTOR_TYPE);
        if (ep == NULL)
        {
#if (USBD_RNDIS_DEBUG)
            printf("RNDIS device: Warning no data EP, skipping interface\n");
#endif //USBD_RNDIS_DEBUG
            continue;
        }
        data_ep = USB_EP_NUM(ep->bEndpointAddress);
        data_ep_size = ep->wMaxPacketSize;

        //configuration is ok, applying
        RNDISD* rndisd = (RNDISD*)malloc(sizeof(RNDISD));
        if (rndisd == NULL)
        {
#if (USBD_RNDIS_DEBUG)
            printf("RNDIS device: Out of memory\n");
#endif //USBD_RNDIS_DEBUG
            return;
        }

        rndisd->control_iface = control_iface;
        rndisd->control_ep = control_ep;
        rndisd->data_iface = data_iface;
        rndisd->data_ep = data_ep;
        rndisd->data_ep_size = data_ep_size;
#if (ETH_DOUBLE_BUFFERING)
        rndisd->tx = rndisd->rx = NULL;
#endif //ETH_DOUBLE_BUFFERING
        rndisd->usb_notify = io_create(control_ep_size);

        usbd_usb_ep_open(usbd, USB_EP_IN | rndisd->data_ep, USB_EP_BULK, rndisd->data_ep_size);
        usbd_usb_ep_open(usbd, rndisd->data_ep, USB_EP_BULK, rndisd->data_ep_size);
        usbd_usb_ep_open(usbd, USB_EP_IN | rndisd->control_ep, USB_EP_INTERRUPT, control_ep_size);

        usbd_register_interface(usbd, rndisd->control_iface, &__RNDISD_CLASS, rndisd);
        usbd_register_interface(usbd, rndisd->data_iface, &__RNDISD_CLASS, rndisd);
        usbd_register_endpoint(usbd, rndisd->data_iface, rndisd->data_ep);
        usbd_register_endpoint(usbd, rndisd->control_iface, rndisd->control_ep);

        rndisd->conn = ETH_NO_LINK;
        rndisd->eth.u32.hi = rndisd->eth.u32.lo = 0;
        rndisd->host.u32.hi = rndisd->host.u32.lo = 0;
        rndisd->tcpip = INVALID_HANDLE;
        rndisd->vendor_id = 0x00;
        rndisd->vendor = NULL;
        rndisd->notify_busy = false;
        rndisd->connected = false;
        rndisd->tx_cur = rndisd->rx_cur = NULL;
        rndisd->zlp = false;
        rndisd->initialized = false;
        rndisd_reset(usbd, rndisd);
    }
}

void rndisd_class_reset(USBD* usbd, void* param)
{
    RNDISD* rndisd = (RNDISD*)param;

    rndisd_reset(usbd, rndisd);
    if (rndisd->tcpip != INVALID_HANDLE)
        ipc_post_inline(rndisd->tcpip, HAL_CMD(HAL_ETH, IPC_CLOSE), USBD_IFACE(rndisd->control_iface, 0), 0, 0);

    usbd_usb_ep_close(usbd, USB_EP_IN | rndisd->control_ep);
    usbd_usb_ep_close(usbd, USB_EP_IN | rndisd->data_ep);
    usbd_usb_ep_close(usbd, rndisd->data_ep);

    usbd_unregister_endpoint(usbd, rndisd->control_iface, rndisd->control_ep);
    usbd_unregister_endpoint(usbd, rndisd->data_iface, rndisd->data_ep);
    usbd_unregister_interface(usbd, rndisd->data_iface, &__RNDISD_CLASS);
    usbd_unregister_interface(usbd, rndisd->control_iface, &__RNDISD_CLASS);
    rndisd_destroy(rndisd);
}

void rndisd_class_suspend(USBD* usbd, void* param)
{
    bool was_link;
    RNDISD* rndisd = (RNDISD*)param;
    was_link = rndisd_link_ready(rndisd);

    if (was_link)
        ipc_post_inline(rndisd->tcpip, HAL_CMD(HAL_ETH, ETH_NOTIFY_LINK_CHANGED), USBD_IFACE(rndisd->control_iface, 0), ETH_NO_LINK, 0);
    rndisd_flush(usbd, rndisd);
}

void rndisd_class_resume(USBD* usbd, void* param)
{
    RNDISD* rndisd = (RNDISD*)param;
    if (rndisd_link_ready(rndisd))
        ipc_post_inline(rndisd->tcpip, HAL_CMD(HAL_ETH, ETH_NOTIFY_LINK_CHANGED), USBD_IFACE(rndisd->control_iface, 0), rndisd->conn, 0);
}

static inline void rndisd_read_complete(USBD* usbd, RNDISD* rndisd, int size)
{
    IO* rx_cur;
    RNDIS_PACKET_MSG* msg;

    rx_cur = rndisd->rx_cur;
    rndisd->rx_cur = NULL;
#if (ETH_DOUBLE_BUFFERING)
    if (rndisd->rx != NULL)
    {
        rndisd->rx_cur = rndisd->rx;
        rndisd->rx = NULL;
        usbd_usb_ep_read(usbd, rndisd->data_ep, rndisd->rx_cur, rndisd->rx_size);
    }
#endif //ETH_DOUBLE_BUFFERING
    if (rx_cur != NULL)
    {
        do {
            if (size < 0)
            {
                io_complete_ex(rndisd->tcpip, HAL_IO_CMD(HAL_ETH, IPC_READ), USBD_IFACE(rndisd->control_iface, 0), rx_cur, size);
                break;
            }
            if (!rndisd_link_ready(rndisd))
            {
                io_complete_ex(rndisd->tcpip, HAL_IO_CMD(HAL_ETH, IPC_READ), USBD_IFACE(rndisd->control_iface, 0), rx_cur, ERROR_NOT_ACTIVE);
                break;
            }
            if (rx_cur->data_size < sizeof(RNDIS_PACKET_MSG))
            {
                io_complete_ex(rndisd->tcpip, HAL_IO_CMD(HAL_ETH, IPC_READ), USBD_IFACE(rndisd->control_iface, 0), rx_cur, ERROR_INVALID_FRAME);
                break;
            }
            msg = io_data(rx_cur);
            if ((msg->message_type != REMOTE_NDIS_PACKET_MSG) || (msg->message_length > rndisd->transfer_size) ||
                (msg->message_length > rx_cur->data_size) ||
                (msg->data_offset + msg->data_length + offsetof(RNDIS_PACKET_MSG, data_offset) > msg->message_length) || (msg->data_length == 0))
            {
                io_complete_ex(rndisd->tcpip, HAL_IO_CMD(HAL_ETH, IPC_READ), USBD_IFACE(rndisd->control_iface, 0), rx_cur, ERROR_INVALID_FRAME);
                break;
            }
            rx_cur->data_offset += msg->data_offset + offsetof(RNDIS_PACKET_MSG, data_offset);
            rx_cur->data_size = msg->data_length;
            ++rndisd->rx_ok;
#if (USBD_RNDIS_DEBUG_FLOW)
            printf("RNDIS device: RX %d\n", rx_cur->data_size);
#endif //USBD_RNDIS_DEBUG_FLOW
            io_complete(rndisd->tcpip, HAL_IO_CMD(HAL_ETH, IPC_READ), USBD_IFACE(rndisd->control_iface, 0), rx_cur);
        } while(false);
    }
}

static void rndisd_write_packet(USBD* usbd, RNDISD* rndisd)
{
    RNDIS_PACKET_MSG* msg;
    io_unhide(rndisd->tx_cur, sizeof(RNDIS_PACKET_MSG));
    msg = io_data(rndisd->tx_cur);
    msg->message_type = REMOTE_NDIS_PACKET_MSG;
    msg->message_length = rndisd->tx_cur->data_size;
    msg->data_offset = sizeof(RNDIS_PACKET_MSG) - offsetof(RNDIS_PACKET_MSG, data_offset);
    msg->data_length = msg->message_length - sizeof(RNDIS_PACKET_MSG);
    memset(&msg->out_of_band_data_offset, 0, sizeof(RNDIS_PACKET_MSG) - offsetof(RNDIS_PACKET_MSG, out_of_band_data_offset));
    usbd_usb_ep_write(usbd, USB_EP_IN | rndisd->data_ep, rndisd->tx_cur);
#if (USBD_RNDIS_DEBUG_FLOW)
    printf("RNDIS device: TX %d\n", rndisd->tx_cur->data_size - sizeof(RNDIS_PACKET_MSG));
#endif //USBD_RNDIS_DEBUG_FLOW
}

void rndisd_write_complete(USBD* usbd, RNDISD* rndisd, int size)
{
    IO* tx_cur;
    //tx ZLP
    if ((size > 0) && ((size % rndisd->data_ep_size) == 0) && rndisd->tx_cur)
    {
        io_reset(rndisd->tx_cur);
        usbd_usb_ep_write(usbd, USB_EP_IN | rndisd->data_ep, rndisd->tx_cur);
        return;
    }
    tx_cur = rndisd->tx_cur;
    rndisd->tx_cur = NULL;
#if (ETH_DOUBLE_BUFFERING)
    if (rndisd->tx != NULL)
    {
        rndisd->tx_cur = rndisd->tx;
        rndisd->tx = NULL;
        rndisd_write_packet(usbd, rndisd);
    }
#endif //ETH_DOUBLE_BUFFERING
    if (tx_cur != NULL)
    {
        do {
            if (size < 0)
            {
                io_complete_ex(rndisd->tcpip, HAL_IO_CMD(HAL_ETH, IPC_WRITE), USBD_IFACE(rndisd->control_iface, 0), tx_cur, size);
                break;
            }
            if (!rndisd_link_ready(rndisd))
            {
                io_complete_ex(rndisd->tcpip, HAL_IO_CMD(HAL_ETH, IPC_WRITE), USBD_IFACE(rndisd->control_iface, 0), tx_cur, ERROR_NOT_ACTIVE);
                break;
            }

            io_complete(rndisd->tcpip, HAL_IO_CMD(HAL_ETH, IPC_WRITE), USBD_IFACE(rndisd->control_iface, 0), tx_cur);
            ++rndisd->tx_ok;
        } while (false);
    }
}

static inline void rndisd_initialize_msg(USBD* usbd, RNDISD* rndisd, IO* io)
{
    RNDIS_INITIALIZE_MSG* msg;
    RNDIS_INITIALIZE_CMPLT* cmplt;
    if (io->data_size < sizeof(RNDIS_INITIALIZE_MSG))
    {
        rndisd_fail(rndisd);
        return;
    }
    msg = io_data(io);
    rndisd_reset(usbd, rndisd);
    rndisd->transfer_size = msg->max_transfer_size;
    if (rndisd->transfer_size > USBD_RNDIS_MAX_PACKET_SIZE)
        rndisd->transfer_size = USBD_RNDIS_MAX_PACKET_SIZE;
    cmplt = (RNDIS_INITIALIZE_CMPLT*)rndisd->response;
    rndisd->response_size = cmplt->message_length = sizeof(RNDIS_INITIALIZE_CMPLT);
    cmplt->message_type = REMOTE_NDIS_INITIALIZE_CMPLT;
    cmplt->request_id = msg->request_id;
    cmplt->status = RNDIS_STATUS_SUCCESS;
    cmplt->major_version = RNDIS_VERSION_MAJOR;
    cmplt->minor_version = RNDIS_VERSION_MINOR;
    cmplt->device_flags = RNDIS_DF_CONNECTIONLESS;
    cmplt->medium = RNDIS_MEDIUM_802_3;
    cmplt->max_packets_per_transfer = 1;
    cmplt->max_transfer_size = rndisd->transfer_size;
    //The value is specified as an exponent of 2
    cmplt->packet_alignment_factor = 2;
    //Reserved for future use and MUST be set to zero. It SHOULD be treated as an error otherwise
    cmplt->reserved[0] = cmplt->reserved[1] = 0x00000000;
    rndisd->initialized = true;
#if (USBD_RNDIS_DEBUG_REQUESTS)
    printf("RNDIS device: INITIALIZE, ver%d.%d, size: %d\n", msg->major_version, msg->minor_version, msg->max_transfer_size);
#endif //USBD_RNDIS_DEBUG_REQUESTS
    if (rndisd_link_ready(rndisd))
        rndisd_link_changed(usbd, rndisd);
}

static inline void rndisd_halt_msg(USBD* usbd, RNDISD* rndisd)
{
    rndisd_reset(usbd, rndisd);
#if (USBD_RNDIS_DEBUG_REQUESTS)
    printf("RNDIS device: HALT\n");
#endif //USBD_RNDIS_DEBUG_REQUESTS
}

static bool rndisd_query_set_check(RNDISD* rndisd, IO* io)
{
    RNDIS_QUERY_SET_MSG* msg = io_data(io);
    if (io->data_size < sizeof(RNDIS_QUERY_SET_MSG))
    {
        rndisd_fail(rndisd);
        return false;
    }
    msg = io_data(io);
    if (msg->information_buffer_length && (msg->information_buffer_offset + msg->information_buffer_length + offsetof(RNDIS_QUERY_SET_MSG, request_id) > io->data_size))
    {
        rndisd_fail(rndisd);
        return false;
    }
    return true;
}

static void* rndisd_query_set_buf(IO* io)
{
    RNDIS_QUERY_SET_MSG* msg = io_data(io);
    return msg->information_buffer_length ? ((uint8_t*)io_data(io) + msg->information_buffer_offset + offsetof(RNDIS_QUERY_SET_MSG, request_id)) : NULL;
}

static void* rndisd_query_append(RNDISD* rndisd, unsigned int size)
{
    RNDIS_QUERY_CMPLT* cmplt = (RNDIS_QUERY_CMPLT*)rndisd->response;
    if (cmplt->information_buffer_length)
        cmplt->information_buffer_offset += size;
    else
        cmplt->information_buffer_offset = sizeof(RNDIS_QUERY_CMPLT) - offsetof(RNDIS_QUERY_CMPLT, request_id);
    cmplt->information_buffer_length += size;
    cmplt->message_length += size;
    rndisd->response_size += size;
    return rndisd->response + cmplt->information_buffer_offset + offsetof(RNDIS_QUERY_CMPLT, request_id);
}

static inline void rndisd_query_gen_supported_list(RNDISD* rndisd)
{
    memcpy(rndisd_query_append(rndisd, RNDIS_GEN_SUPPORTED_LIST_SIZE), __RNDIS_GEN_SUPPORTED_LIST, RNDIS_GEN_SUPPORTED_LIST_SIZE);
#if (USBD_RNDIS_DEBUG_REQUESTS)
    printf("RNDIS device: QUERY supported list\n");
#endif //USBD_RNDIS_DEBUG_REQUESTS
}

static inline void rndisd_query_gen_hardware_status(RNDISD* rndisd)
{
    *((uint32_t*)rndisd_query_append(rndisd, sizeof(uint32_t))) = rndisd_link_ready(rndisd) ? RNDIS_HARDWARE_STATUS_READY : RNDIS_HARDWARE_STATUS_NOT_READY;
#if (USBD_RNDIS_DEBUG_REQUESTS)
    printf("RNDIS device: QUERY hardware status: %s\n", rndisd_link_ready(rndisd) ? "READY" : "NOT READY");
#endif //USBD_RNDIS_DEBUG_REQUESTS
}

static inline void rndisd_query_gen_media_supported(RNDISD* rndisd)
{
    *((uint32_t*)rndisd_query_append(rndisd, sizeof(uint32_t))) = RNDIS_MEDIUM_802_3;
#if (USBD_RNDIS_DEBUG_REQUESTS)
    printf("RNDIS device: QUERY media supported(802.3)\n");
#endif //USBD_RNDIS_DEBUG_REQUESTS
}

static inline void rndisd_query_gen_media_in_use(RNDISD* rndisd)
{
    *((uint32_t*)rndisd_query_append(rndisd, sizeof(uint32_t))) = RNDIS_MEDIUM_802_3;
#if (USBD_RNDIS_DEBUG_REQUESTS)
    printf("RNDIS device: QUERY media in use(802.3)\n");
#endif //USBD_RNDIS_DEBUG_REQUESTS
}

static inline void rndisd_query_gen_max_frame_size(RNDISD* rndisd)
{
    *((uint32_t*)rndisd_query_append(rndisd, sizeof(uint32_t))) = TCPIP_MTU;
#if (USBD_RNDIS_DEBUG_REQUESTS)
    printf("RNDIS device: QUERY maximum frame size(%d)\n", TCPIP_MTU);
#endif //USBD_RNDIS_DEBUG_REQUESTS
}

static inline void rndisd_query_gen_link_speed(RNDISD* rndisd)
{
    unsigned int value = 0;
    if (rndisd_link_ready(rndisd))
        switch (rndisd->conn)
        {
        case ETH_10_FULL:
        case ETH_10_HALF:
            value = 10000000 / 100;
            break;
        case ETH_100_FULL:
        case ETH_100_HALF:
            value = 100000000 / 100;
            break;
        default:
            break;
        }

    *((uint32_t*)rndisd_query_append(rndisd, sizeof(uint32_t))) = value;
#if (USBD_RNDIS_DEBUG_REQUESTS)
    printf("RNDIS device: QUERY link speed(%d)\n", value * 100);
#endif //USBD_RNDIS_DEBUG_REQUESTS
}

static inline void rndisd_query_gen_transmit_block_size(RNDISD* rndisd)
{
    *((uint32_t*)rndisd_query_append(rndisd, sizeof(uint32_t))) = TCPIP_MTU;
#if (USBD_RNDIS_DEBUG_REQUESTS)
    printf("RNDIS device: QUERY transmit block size(%d)\n", TCPIP_MTU);
#endif //USBD_RNDIS_DEBUG_REQUESTS
}

static inline void rndisd_query_gen_receive_block_size(RNDISD* rndisd)
{
    *((uint32_t*)rndisd_query_append(rndisd, sizeof(uint32_t))) = TCPIP_MTU;
#if (USBD_RNDIS_DEBUG_REQUESTS)
    printf("RNDIS device: QUERY receive block size(%d)\n", TCPIP_MTU);
#endif //USBD_RNDIS_DEBUG_REQUESTS
}

static inline void rndisd_query_gen_vendor_id(RNDISD* rndisd)
{
    uint8_t* id =  (uint8_t*)rndisd_query_append(rndisd, 3);
    id[0] = (rndisd->vendor_id >> 16) & 0xff;
    id[1] = (rndisd->vendor_id >> 8) & 0xff;
    id[2] = (rndisd->vendor_id >> 0) & 0xff;
#if (USBD_RNDIS_DEBUG_REQUESTS)
    printf("RNDIS device: QUERY vendor id(%02X%02X%02X)\n", (rndisd->vendor_id >> 16) & 0xff, (rndisd->vendor_id >> 8) & 0xff, (rndisd->vendor_id >> 0) & 0xff);
#endif //USBD_RNDIS_DEBUG_REQUESTS
}

static inline void rndisd_query_gen_vendor_description(RNDISD* rndisd)
{
    unsigned int len = 0;
    char* vendor;
    if (rndisd->vendor != NULL)
        len = strlen(rndisd->vendor);
    vendor = (char*)rndisd_query_append(rndisd, len + 1);
    if (len)
        memcpy(vendor, rndisd->vendor, len);
    vendor[len] = 0x0;
#if (USBD_RNDIS_DEBUG_REQUESTS)
    printf("RNDIS device: QUERY vendor description(\"%s\")\n", vendor);
#endif //USBD_RNDIS_DEBUG_REQUESTS
}

static inline void rndisd_query_gen_maximum_total_size(RNDISD* rndisd)
{
    *((uint32_t*)rndisd_query_append(rndisd, sizeof(uint32_t))) = rndisd->transfer_size;
#if (USBD_RNDIS_DEBUG_REQUESTS)
    printf("RNDIS device: QUERY maximum total size(%d)\n", rndisd->transfer_size);
#endif //USBD_RNDIS_DEBUG_REQUESTS
}

static inline void rndisd_query_gen_media_connect_status(RNDISD* rndisd)
{
    *((uint32_t*)rndisd_query_append(rndisd, sizeof(uint32_t))) = rndisd_link_ready(rndisd) ? RNDIS_MEDIA_STATE_CONNECTED : RNDIS_MEDIA_STATE_DISCONNECTED;
#if (USBD_RNDIS_DEBUG_REQUESTS)
    printf("RNDIS device: QUERY media connect status: %s\n", rndisd_link_ready(rndisd) ? "CONNECTED" : "DISCONNECTED");
#endif //USBD_RNDIS_DEBUG_REQUESTS
}

static inline void rndisd_query_gen_physical_medium(RNDISD* rndisd)
{
    *((uint32_t*)rndisd_query_append(rndisd, sizeof(uint32_t))) = RNDIS_PHYSICAL_MEDIUM_802_3;
#if (USBD_RNDIS_DEBUG_REQUESTS)
    printf("RNDIS device: QUERY physical medium(802.3)\n");
#endif //USBD_RNDIS_DEBUG_REQUESTS
}

static inline void rndisd_query_gen_xmit_ok(RNDISD* rndisd)
{
    *((uint32_t*)rndisd_query_append(rndisd, sizeof(uint32_t))) = rndisd->tx_ok;
#if (USBD_RNDIS_DEBUG_REQUESTS)
    printf("RNDIS device: QUERY xmit ok (%d)\n", rndisd->tx_ok);
#endif //USBD_RNDIS_DEBUG_REQUESTS
}

static inline void rndisd_query_gen_rcv_ok(RNDISD* rndisd)
{
    *((uint32_t*)rndisd_query_append(rndisd, sizeof(uint32_t))) = rndisd->rx_ok;
#if (USBD_RNDIS_DEBUG_REQUESTS)
    printf("RNDIS device: QUERY rcv ok (%d)\n", rndisd->rx_ok);
#endif //USBD_RNDIS_DEBUG_REQUESTS
}

static inline void rndisd_query_gen_xmit_error(RNDISD* rndisd)
{
    *((uint32_t*)rndisd_query_append(rndisd, sizeof(uint32_t))) = 0;
#if (USBD_RNDIS_DEBUG_REQUESTS)
    printf("RNDIS device: QUERY xmit error (0)\n");
#endif //USBD_RNDIS_DEBUG_REQUESTS
}

static inline void rndisd_query_gen_rcv_error(RNDISD* rndisd)
{
    *((uint32_t*)rndisd_query_append(rndisd, sizeof(uint32_t))) = 0;
#if (USBD_RNDIS_DEBUG_REQUESTS)
    printf("RNDIS device: QUERY rcv error (%d)\n", 0);
#endif //USBD_RNDIS_DEBUG_REQUESTS
}

static inline void rndisd_query_gen_rcv_no_buffer(RNDISD* rndisd)
{
    *((uint32_t*)rndisd_query_append(rndisd, sizeof(uint32_t))) = 0;
#if (USBD_RNDIS_DEBUG_REQUESTS)
    printf("RNDIS device: QUERY rcv no buffer (%d)\n", 0);
#endif //USBD_RNDIS_DEBUG_REQUESTS
}

static inline void rndisd_query_802_3_permanent_address(RNDISD* rndisd)
{
    MAC* mac;
    mac = rndisd_query_append(rndisd, sizeof(MAC));
    mac->u32.hi = rndisd->host.u32.hi;
    mac->u32.lo = rndisd->host.u32.lo;
#if (USBD_RNDIS_DEBUG_REQUESTS)
    printf("RNDIS device: QUERY 802.3 permanent address ");
    mac_print(&rndisd->host);
    printf("\n");
#endif //USBD_RNDIS_DEBUG_REQUESTS
}

static inline void rndisd_query_802_3_current_address(RNDISD* rndisd)
{
    MAC* mac;
    mac = rndisd_query_append(rndisd, sizeof(MAC));
    mac->u32.hi = rndisd->host.u32.hi;
    mac->u32.lo = rndisd->host.u32.lo;
#if (USBD_RNDIS_DEBUG_REQUESTS)
    printf("RNDIS device: QUERY 802.3 current address");
    mac_print(&rndisd->host);
    printf("\n");
#endif //USBD_RNDIS_DEBUG_REQUESTS
}

static inline void rndisd_query_802_3_multicast_list(RNDISD* rndisd)
{
    //empty list for compatibility
#if (USBD_RNDIS_DEBUG_REQUESTS)
    printf("RNDIS device: QUERY multicast list\n");
#endif //USBD_RNDIS_DEBUG_REQUESTS
}

static inline void rndisd_query_802_3_maximum_list_size(RNDISD* rndisd)
{
    //empty list for compatibility
    *((uint32_t*)rndisd_query_append(rndisd, sizeof(uint32_t))) = 0;
#if (USBD_RNDIS_DEBUG_REQUESTS)
    printf("RNDIS device: QUERY maximum list size (0)\n");
#endif //USBD_RNDIS_DEBUG_REQUESTS
}

static inline void rndisd_query_802_3_rcv_error_alignment(RNDISD* rndisd)
{
    *((uint32_t*)rndisd_query_append(rndisd, sizeof(uint32_t))) = 0;
#if (USBD_RNDIS_DEBUG_REQUESTS)
    printf("RNDIS device: QUERY rcv error alignment (0)\n");
#endif //USBD_RNDIS_DEBUG_REQUESTS
}

static inline void rndisd_query_802_3_xmit_one_collision(RNDISD* rndisd)
{
    *((uint32_t*)rndisd_query_append(rndisd, sizeof(uint32_t))) = 0;
#if (USBD_RNDIS_DEBUG_REQUESTS)
    printf("RNDIS device: QUERY xmit one collision (0)\n");
#endif //USBD_RNDIS_DEBUG_REQUESTS
}

static inline void rndisd_query_802_3_xmit_more_collisions(RNDISD* rndisd)
{
    *((uint32_t*)rndisd_query_append(rndisd, sizeof(uint32_t))) = 0;
#if (USBD_RNDIS_DEBUG_REQUESTS)
    printf("RNDIS device: QUERY xmit more collisions (0)\n");
#endif //USBD_RNDIS_DEBUG_REQUESTS
}

static inline void rndisd_query_802_3_xmit_deferred(RNDISD* rndisd)
{
    *((uint32_t*)rndisd_query_append(rndisd, sizeof(uint32_t))) = 0;
#if (USBD_RNDIS_DEBUG_REQUESTS)
    printf("RNDIS device: QUERY xmit deferred (0)\n");
#endif //USBD_RNDIS_DEBUG_REQUESTS
}

static inline void rndisd_query_msg(RNDISD* rndisd, IO* io)
{
    RNDIS_QUERY_SET_MSG* msg;
    RNDIS_QUERY_CMPLT* cmplt;
    if (!rndisd_query_set_check(rndisd, io))
        return;
    msg = io_data(io);
    cmplt = (RNDIS_QUERY_CMPLT*)rndisd->response;
    cmplt->message_type = REMOTE_NDIS_QUERY_CMPLT;
    rndisd->response_size = cmplt->message_length = sizeof(RNDIS_QUERY_CMPLT);
    cmplt->request_id = msg->request_id;
    cmplt->status = RNDIS_STATUS_SUCCESS;
    cmplt->information_buffer_length = 0;
    cmplt->information_buffer_offset = 0;

    switch (msg->oid)
    {
    case OID_GEN_SUPPORTED_LIST:
        rndisd_query_gen_supported_list(rndisd);
        break;
    case OID_GEN_HARDWARE_STATUS:
        rndisd_query_gen_hardware_status(rndisd);
        break;
    case OID_GEN_MEDIA_SUPPORTED:
        rndisd_query_gen_media_supported(rndisd);
        break;
    case OID_GEN_MEDIA_IN_USE:
        rndisd_query_gen_media_in_use(rndisd);
        break;
    case OID_GEN_MAXIMUM_FRAME_SIZE:
        rndisd_query_gen_max_frame_size(rndisd);
        break;
    case OID_GEN_LINK_SPEED:
        rndisd_query_gen_link_speed(rndisd);
        break;
    case OID_GEN_TRANSMIT_BLOCK_SIZE:
        rndisd_query_gen_transmit_block_size(rndisd);
        break;
    case OID_GEN_RECEIVE_BLOCK_SIZE:
        rndisd_query_gen_receive_block_size(rndisd);
        break;
    case OID_GEN_VENDOR_ID:
        rndisd_query_gen_vendor_id(rndisd);
        break;
    case OID_GEN_VENDOR_DESCRIPTION:
        rndisd_query_gen_vendor_description(rndisd);
        break;
    case OID_GEN_MAXIMUM_TOTAL_SIZE:
        rndisd_query_gen_maximum_total_size(rndisd);
        break;
    case OID_GEN_MEDIA_CONNECT_STATUS:
        rndisd_query_gen_media_connect_status(rndisd);
        break;
    case OID_GEN_PHYSICAL_MEDIUM:
        rndisd_query_gen_physical_medium(rndisd);
        break;
    case OID_GEN_XMIT_OK:
        rndisd_query_gen_xmit_ok(rndisd);
        break;
    case OID_GEN_RCV_OK:
        rndisd_query_gen_rcv_ok(rndisd);
        break;
    case OID_GEN_XMIT_ERROR:
        rndisd_query_gen_xmit_error(rndisd);
        break;
    case OID_GEN_RCV_ERROR:
        rndisd_query_gen_rcv_error(rndisd);
        break;
    case OID_GEN_RCV_NO_BUFFER:
        rndisd_query_gen_rcv_no_buffer(rndisd);
        break;
    case OID_802_3_PERMANENT_ADDRESS:
        rndisd_query_802_3_permanent_address(rndisd);
        break;
    case OID_802_3_CURRENT_ADDRESS:
        rndisd_query_802_3_current_address(rndisd);
        break;
    case OID_802_3_MULTICAST_LIST:
        rndisd_query_802_3_multicast_list(rndisd);
        break;
    case OID_802_3_MAXIMUM_LIST_SIZE:
        rndisd_query_802_3_maximum_list_size(rndisd);
        break;
    case OID_802_3_RCV_ERROR_ALIGNMENT:
        rndisd_query_802_3_rcv_error_alignment(rndisd);
        break;
    case OID_802_3_XMIT_ONE_COLLISION:
        rndisd_query_802_3_xmit_one_collision(rndisd);
        break;
    case OID_802_3_XMIT_MORE_COLLISIONS:
        rndisd_query_802_3_xmit_more_collisions(rndisd);
        break;
    case OID_802_3_XMIT_DEFERRED:
        rndisd_query_802_3_xmit_deferred(rndisd);
        break;
    default:
#if (USBD_RNDIS_DEBUG_REQUESTS)
        printf("RNDIS device: unsupported QUERY Oid: %#X\n", msg->oid);
#endif //USBD_RNDIS_DEBUG_REQUESTS
        cmplt->status = RNDIS_STATUS_NOT_SUPPORTED;
    }
}

static inline void rndisd_set_packet_filter(RNDISD* rndisd, void* buf, unsigned int size)
{
    if (size < sizeof(uint32_t))
    {
        rndisd_fail(rndisd);
        return;
    }
    rndisd->packet_filter = *((uint32_t*)buf);
#if (USBD_RNDIS_DEBUG_REQUESTS)
    printf("RNDIS device: SET current packet filter: %#08X\n", rndisd->packet_filter);
#endif //USBD_RNDIS_DEBUG_REQUESTS
}

static inline void rndisd_set_802_3_multicast_list(RNDISD* rndisd)
{
    RNDIS_SET_KEEP_ALIVE_CMPLT* cmplt = (RNDIS_SET_KEEP_ALIVE_CMPLT*)rndisd->response;
    cmplt->status = RNDIS_STATUS_MULTICAST_FULL;
#if (USBD_RNDIS_DEBUG_REQUESTS)
    printf("RNDIS device: SET multicast list\n");
#endif //USBD_RNDIS_DEBUG_REQUESTS
}

static inline void rndisd_set_msg(RNDISD* rndisd, IO* io)
{
    RNDIS_QUERY_SET_MSG* msg;
    RNDIS_SET_KEEP_ALIVE_CMPLT* cmplt;
    void* buf;
    if (!rndisd_query_set_check(rndisd, io))
        return;
    msg = io_data(io);
    buf = rndisd_query_set_buf(io);
    cmplt = (RNDIS_SET_KEEP_ALIVE_CMPLT*)rndisd->response;
    cmplt->message_type = REMOTE_NDIS_SET_CMPLT;
    rndisd->response_size = cmplt->message_length = sizeof(RNDIS_SET_KEEP_ALIVE_CMPLT);
    cmplt->request_id = msg->request_id;
    cmplt->status = RNDIS_STATUS_SUCCESS;

    switch (msg->oid)
    {
    case OID_GEN_CURRENT_PACKET_FILTER:
        rndisd_set_packet_filter(rndisd, buf, msg->information_buffer_length);
        break;
    case OID_802_3_MULTICAST_LIST:
        rndisd_set_802_3_multicast_list(rndisd);
    default:
#if (USBD_RNDIS_DEBUG_REQUESTS)
        printf("RNDIS device: unsupported SET Oid: %#X\n", msg->oid);
#endif //USBD_RNDIS_DEBUG_REQUESTS
        cmplt->status = RNDIS_STATUS_NOT_SUPPORTED;
    }
}

static inline void rndisd_reset_msg(USBD* usbd, RNDISD* rndisd)
{
    RNDIS_RESET_CMPLT* cmplt = (RNDIS_RESET_CMPLT*)rndisd->response;
    rndisd_reset(usbd, rndisd);
    cmplt->message_type = REMOTE_NDIS_RESET_CMPLT;
    rndisd->response_size = cmplt->message_length = sizeof(RNDIS_RESET_CMPLT);
    cmplt->status = RNDIS_STATUS_SUCCESS;
    cmplt->addressing_reset = 0x00000001;
#if (USBD_RNDIS_DEBUG_REQUESTS)
    printf("RNDIS device: RESET\n");
#endif //USBD_RNDIS_DEBUG_REQUESTS
}

static inline void rndisd_keepalive_msg(RNDISD* rndisd, IO* io)
{
    RNDIS_GENERIC_MSG* msg;
    RNDIS_SET_KEEP_ALIVE_CMPLT* cmplt = (RNDIS_SET_KEEP_ALIVE_CMPLT*)rndisd->response;
    msg = io_data(io);
    cmplt->message_type = REMOTE_NDIS_KEEPALIVE_CMPLT;
    rndisd->response_size = cmplt->message_length = sizeof(RNDIS_SET_KEEP_ALIVE_CMPLT);
    cmplt->request_id = msg->request_id;
    cmplt->status = RNDIS_STATUS_SUCCESS;
#if (USBD_RNDIS_DEBUG_REQUESTS)
    printf("RNDIS device: KEEP-ALIVE\n");
#endif //USBD_RNDIS_DEBUG_REQUESTS
}

static inline int rndisd_send_encapsulated_command(USBD* usbd, RNDISD* rndisd, IO* io)
{
    RNDIS_GENERIC_MSG* msg;
    do
    {
        if (io->data_size < sizeof(RNDIS_GENERIC_MSG))
        {
            rndisd_fail(rndisd);
            break;
        }
        msg = io_data(io);
        if (msg->message_length != io->data_size)
        {
            rndisd_fail(rndisd);
            break;
        }
        rndisd->notify_busy = true;
        //Fucking Microsoft doesn't use SETUP, they made own interface
        switch (msg->message_type)
        {
        case REMOTE_NDIS_INITIALIZE_MSG:
            rndisd_initialize_msg(usbd, rndisd, io);
            break;
        case REMOTE_NDIS_HALT_MSG:
            rndisd_halt_msg(usbd, rndisd);
            //no need to respond on HALT
            return 0;
        case REMOTE_NDIS_QUERY_MSG:
            rndisd_query_msg(rndisd, io);
            break;
        case REMOTE_NDIS_SET_MSG:
            rndisd_set_msg(rndisd, io);
            break;
        case REMOTE_NDIS_RESET_MSG:
            rndisd_reset_msg(usbd, rndisd);
            break;
        case REMOTE_NDIS_KEEPALIVE_MSG:
            rndisd_keepalive_msg(rndisd, io);
            break;
        default:
#if (USBD_RNDIS_DEBUG)
            printf("RNDIS device: unsupported MSG %#X\n", msg->message_type);
#endif //USBD_RNDIS_DEBUG
            rndisd_fail(rndisd);
        }

    } while (false);
    rndisd_notify(usbd, rndisd);
    return 0;
}

static inline int rndisd_get_encapsulated_response(RNDISD* rndisd, IO* io)
{
    memcpy(io_data(io), rndisd->response, rndisd->response_size);
    io->data_size = rndisd->response_size;
    rndisd->notify_busy = false;
    if (rndisd->link_status_queued)
    {
        rndisd->link_status_queued = false;
        rndisd_indicate_status(rndisd);
    }
    return io->data_size;
}

int rndisd_class_setup(USBD* usbd, void* param, SETUP* setup, IO* io)
{
    RNDISD* rndisd;
    int res = -1;
    rndisd = (RNDISD*)param;

    switch (setup->bRequest)
    {
    case SEND_ENCAPSULATED_COMMAND:
        res = rndisd_send_encapsulated_command(usbd, rndisd, io);
        break;
    case GET_ENCAPSULATED_RESPONSE:
        res = rndisd_get_encapsulated_response(rndisd, io);
        break;
    default:
        break;
    }
    return res;
}

static inline void rndisd_driver_event(USBD* usbd, RNDISD* rndisd, IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_READ:
        rndisd_read_complete(usbd, rndisd, (int)ipc->param3);
        break;
    case IPC_WRITE:
        //only bulk transfers
        if (((IO*)ipc->param2) != rndisd->usb_notify)
            rndisd_write_complete(usbd, rndisd, (int)ipc->param3);
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
    }
}

static inline void rndisd_eth_set_mac(RNDISD* rndisd, unsigned int param2, unsigned int param3)
{
    rndisd->eth.u32.hi = param2;
    rndisd->eth.u32.lo = (uint16_t)param3;
}

static inline void rndisd_eth_get_mac(RNDISD* rndisd, IPC* ipc)
{
    ipc->param2 = rndisd->eth.u32.hi;
    ipc->param3 = rndisd->eth.u32.lo;
}

static inline unsigned int rndisd_eth_get_header_size()
{
    return sizeof(RNDIS_PACKET_MSG);
}

static inline void rndisd_eth_open(USBD* usbd, RNDISD* rndisd, HANDLE tcpip)
{
    bool was_ready = rndisd_link_ready(rndisd);
    if (rndisd->tcpip != INVALID_HANDLE)
    {
        error(ERROR_ALREADY_CONFIGURED);
        return;
    }
    rndisd->tcpip = tcpip;
    if (was_ready != rndisd_link_ready(rndisd))
        rndisd_link_changed(usbd, rndisd);
}

static inline void rndisd_eth_close(USBD* usbd, RNDISD* rndisd, HANDLE tcpip)
{
    if (rndisd->tcpip == INVALID_HANDLE)
    {
        error(ERROR_NOT_CONFIGURED);
        return;
    }
    if (rndisd->tcpip != tcpip)
    {
        error(ERROR_ACCESS_DENIED);
        return;
    }
    rndisd_reset(usbd, rndisd);
    rndisd->tcpip = INVALID_HANDLE;
}

static inline void rndisd_eth_read(USBD* usbd, RNDISD* rndisd, IO* io, unsigned int size)
{
    if (!rndisd_link_ready(rndisd))
    {
        error(ERROR_NOT_ACTIVE);
        return;
    }
    if (rndisd->rx_cur == NULL)
    {
        rndisd->rx_cur = io;
        usbd_usb_ep_read(usbd, rndisd->data_ep, rndisd->rx_cur, size);
        error(ERROR_SYNC);
    }
#if (ETH_DOUBLE_BUFFERING)
    else if (rndisd->rx == NULL)
    {
        rndisd->rx = io;
        rndisd->rx_size = size;
        error(ERROR_SYNC);
    }
#endif //ETH_DOUBLE_BUFFERING
    else
        error(ERROR_IN_PROGRESS);
}

static inline void rndisd_eth_write(USBD* usbd, RNDISD* rndisd, IO* io)
{
    if (!rndisd_link_ready(rndisd))
    {
        error(ERROR_NOT_ACTIVE);
        return;
    }
    if (rndisd->tx_cur == NULL)
    {
        rndisd->tx_cur = io;
        rndisd_write_packet(usbd, rndisd);
        error(ERROR_SYNC);
    }
#if (ETH_DOUBLE_BUFFERING)
    else if (rndisd->tx == NULL)
    {
        rndisd->tx = io;
        error(ERROR_SYNC);
    }
#endif //ETH_DOUBLE_BUFFERING
    else
        error(ERROR_IN_PROGRESS);
}

static inline void rndisd_eth_event(USBD* usbd, RNDISD* rndisd, IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
    case ETH_SET_MAC:
        rndisd_eth_set_mac(rndisd, ipc->param2, ipc->param3);
        break;
    case ETH_GET_MAC:
        rndisd_eth_get_mac(rndisd, ipc);
        break;
    case ETH_GET_HEADER_SIZE:
        ipc->param2 = rndisd_eth_get_header_size();
        ipc->param3 = ERROR_OK;
        break;
    case IPC_OPEN:
        rndisd_eth_open(usbd, rndisd, ipc->process);
        break;
    case IPC_CLOSE:
        rndisd_eth_close(usbd, rndisd, ipc->process);
        break;
    case IPC_READ:
        rndisd_eth_read(usbd, rndisd, (IO*)ipc->param2, ipc->param3);
        break;
    case IPC_WRITE:
        rndisd_eth_write(usbd, rndisd, (IO*)ipc->param2);
        break;
    case IPC_FLUSH:
        rndisd_flush(usbd, rndisd);
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
    }
}

static inline void rndisd_set_link(USBD* usbd, RNDISD* rndisd, ETH_CONN_TYPE conn)
{
    bool was_ready = rndisd_link_ready(rndisd);
    if (rndisd->conn == conn)
        return;
    rndisd->conn = conn;
    rndisd->connected = ((rndisd->conn != ETH_NO_LINK) && (rndisd->conn != ETH_REMOTE_FAULT));
#if (USBD_RNDIS_DEBUG)
    printf("RNDIS device: ETH link changed\n");
#endif //USBD_RNDIS_DEBUG
    //link established
    if (was_ready != rndisd_link_ready(rndisd))
        rndisd_link_changed(usbd, rndisd);
}

static inline void rndisd_set_vendor_id(RNDISD* rndisd, uint32_t vendor_id)
{
    rndisd->vendor_id = vendor_id;
}

static inline void rndisd_set_vendor_description(RNDISD* rndisd, IO* io)
{
    unsigned int len;
    if (rndisd->vendor != NULL)
    {
        error(ERROR_ALREADY_CONFIGURED);
        return;
    }
    len = strlen(io_data(io));
    if (len > RNDIS_RESPONSE_SIZE - 1)
        len = RNDIS_RESPONSE_SIZE - 1;
    if ((rndisd->vendor = malloc(len + 1)) == NULL);
    memcpy(rndisd->vendor, io_data(io), len);
    rndisd->vendor[len] = 0x00;
}

static inline void rndisd_set_host_mac(RNDISD* rndisd, unsigned int param2, unsigned int param3)
{
    rndisd->host.u32.hi = param2;
    rndisd->host.u32.lo = (uint16_t)param3;
}

static inline void rndisd_get_host_mac(RNDISD* rndisd, IPC* ipc)
{
    ipc->param2 = rndisd->host.u32.hi;
    ipc->param3 = rndisd->host.u32.lo;
}


static inline void rndisd_iface_event(USBD* usbd, RNDISD* rndisd, IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
    case RNDIS_SET_LINK:
        rndisd_set_link(usbd, rndisd, (ETH_CONN_TYPE)ipc->param2);
        break;
    case RNDIS_SET_VENDOR_ID:
        rndisd_set_vendor_id(rndisd, ipc->param2);
        break;
    case RNDIS_SET_VENDOR_DESCRIPTION:
        rndisd_set_vendor_description(rndisd, (IO*)ipc->param2);
        break;
    case RNDIS_SET_HOST_MAC:
        rndisd_set_host_mac(rndisd, ipc->param2, ipc->param3);
        break;
    case RNDIS_GET_HOST_MAC:
        rndisd_get_host_mac(rndisd, ipc);
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
    }
}

void rndisd_class_request(USBD* usbd, void* param, IPC* ipc)
{
    RNDISD* rndisd = (RNDISD*)param;
    switch (HAL_GROUP(ipc->cmd))
    {
    case HAL_USB:
        rndisd_driver_event(usbd, rndisd, ipc);
        break;
    case HAL_ETH:
        rndisd_eth_event(usbd, rndisd, ipc);
        break;
    case HAL_USBD_IFACE:
        rndisd_iface_event(usbd, rndisd, ipc);
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
    }
}

const USBD_CLASS __RNDISD_CLASS = {
    rndisd_class_configured,
    rndisd_class_reset,
    rndisd_class_suspend,
    rndisd_class_resume,
    rndisd_class_setup,
    rndisd_class_request,
};
