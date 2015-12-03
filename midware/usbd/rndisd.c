/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
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

#define RNDIS_RESPONSE_SIZE                                                     64

typedef struct {
    IO* usb_rx;
    IO* usb_tx;
    IO* usb_notify;
#if (ETH_DOUBLE_BUFFERING)
    IO* tx[2];
    IO* rx[2];
#else
    IO* tx;
    IO* rx;
#endif
    unsigned int transfer_size;
    uint32_t packet_filter;
    ETH_CONN_TYPE conn;
    MAC mac;
    HANDLE tcpip;
    uint8_t response[RNDIS_RESPONSE_SIZE];
    uint8_t response_size;
    uint8_t data_ep, control_ep;
    uint16_t data_ep_size;
    uint8_t data_iface, control_iface;
    bool busy, link_status_queued, connected;
} RNDISD;

static void rndisd_destroy(RNDISD* rndisd)
{
    io_destroy(rndisd->usb_rx);
    io_destroy(rndisd->usb_tx);

    free(rndisd);
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
    return (rndisd->tcpip != INVALID_HANDLE) && (rndisd->usb_rx != NULL) && (rndisd->connected);
}

static void rndisd_indicate_status(RNDISD* rndisd)
{
    RNDIS_INDICATE_STATUS_MSG* msg;
    if (rndisd->busy)
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

static void rndisd_rx(USBD* usbd, RNDISD* rndisd)
{
    if (rndisd_link_ready(rndisd))
        usbd_usb_ep_read(usbd, rndisd->data_ep, rndisd->usb_rx, rndisd->data_ep_size);
}

static void rndisd_link_changed(USBD* usbd, RNDISD* rndisd)
{
    if (rndisd_link_ready(rndisd))
    {
        rndisd_indicate_status(rndisd);
        ipc_post_inline(rndisd->tcpip, HAL_CMD(HAL_ETH, ETH_NOTIFY_LINK_CHANGED), USBD_IFACE(rndisd->control_iface, 0), rndisd->conn, 0);
        rndisd_rx(usbd, rndisd);
    }
    else
    {
        //flush?
        if (rndisd->tcpip != INVALID_HANDLE)
            ipc_post_inline(rndisd->tcpip, HAL_CMD(HAL_ETH, ETH_NOTIFY_LINK_CHANGED), USBD_IFACE(rndisd->control_iface, 0), rndisd->conn, 0);
        if (rndisd->usb_rx != NULL)
            rndisd_indicate_status(rndisd);
    }
}

static void rndisd_reset(USBD* usbd, RNDISD* rndisd)
{
    bool was_ready = rndisd_link_ready(rndisd);
    if (rndisd->usb_rx != NULL)
    {
        io_destroy(rndisd->usb_rx);
        rndisd->usb_rx = NULL;
    }
    rndisd->packet_filter = RNDIS_PACKET_TYPE_DIRECTED | RNDIS_PACKET_TYPE_ALL_MULTICAST | RNDIS_PACKET_TYPE_BROADCAST | RNDIS_PACKET_TYPE_PROMISCUOUS;
    rndisd->link_status_queued = false;
    if (was_ready)
        rndisd_link_changed(usbd, rndisd);
}

static void rndisd_fail(RNDISD* rndisd)
{
    rndisd->response[0] = 0x00;
    rndisd->response_size = 1;
}

void rndisd_class_configured(USBD* usbd, USB_CONFIGURATION_DESCRIPTOR_TYPE* cfg)
{
    USB_INTERFACE_DESCRIPTOR_TYPE* iface;
    USB_INTERFACE_DESCRIPTOR_TYPE* diface;
    CDC_UNION_DESCRIPTOR_TYPE* u;
    USB_ENDPOINT_DESCRIPTOR_TYPE* ep;
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

        ep = (USB_ENDPOINT_DESCRIPTOR_TYPE*)usb_interface_get_first_descriptor(cfg, iface, USB_ENDPOINT_DESCRIPTOR_INDEX);
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
                (u->bFunctionLength > sizeof(CDC_UNION_DESCRIPTOR_TYPE)))
                break;
        }
        if (u == NULL)
        {
#if (USBD_RNDIS_DEBUG)
            printf("RNDIS device: Warning - no UNION descriptor, skipping interface\n");
#endif //USBD_RNDIS_DEBUG
            continue;
        }
        data_iface = ((uint8_t*)u)[sizeof(CDC_UNION_DESCRIPTOR_TYPE)];
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

        ep = (USB_ENDPOINT_DESCRIPTOR_TYPE*)usb_interface_get_first_descriptor(cfg, diface, USB_ENDPOINT_DESCRIPTOR_INDEX);
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
        rndisd->usb_tx = rndisd->usb_rx = NULL;
#if (ETH_DOUBLE_BUFFERING)
        rndisd->tx[0] = rndisd->tx[1] = rndisd->rx[0] = rndisd->rx[1] = NULL;
#else
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
        rndisd->mac.u32.hi = rndisd->mac.u32.lo = 0;
        rndisd->tcpip = INVALID_HANDLE;
        rndisd->busy = false;
        rndisd->connected = false;
        rndisd_reset(usbd, rndisd);
    }
}

void rndisd_class_reset(USBD* usbd, void* param)
{
    RNDISD* rndisd = (RNDISD*)param;

    //TODO: tcpip remote close

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
    //TODO: tcpip set no link

    RNDISD* rndisd = (RNDISD*)param;
    usbd_usb_ep_flush(usbd, USB_EP_IN | rndisd->control_ep);
    usbd_usb_ep_flush(usbd, USB_EP_IN | rndisd->data_ep);
    usbd_usb_ep_flush(usbd, rndisd->data_ep);
}

void rndisd_class_resume(USBD* usbd, void* param)
{
    //TODO: tcpip restore link

    RNDISD* rndisd = (RNDISD*)param;
}

static inline void rndisd_read_complete(USBD* usbd, RNDISD* rndisd, int size)
{
    RNDIS_PACKET_MSG* msg;
    void* data;
    IO* io;
    unsigned int frame_size;
#if (ETH_DOUBLE_BUFFERING)
    int i;
#endif //ETH_DOUBLE_BUFFERING
    if (size < 0 || !rndisd_link_ready(rndisd))
        return;
    do {
        if (rndisd->usb_rx->data_size < sizeof(RNDIS_PACKET_MSG))
            break;
        io_unhide(rndisd->usb_rx);
        msg = io_data(rndisd->usb_rx);
        if (msg->message_type != REMOTE_NDIS_PACKET_MSG)
            break;
        if (msg->message_length > rndisd->transfer_size)
            break;
        //long packet > EP size
        if ((size == rndisd->data_ep_size) && (msg->message_length > rndisd->data_ep_size))
        {
            io_hide(rndisd->usb_rx, rndisd->data_ep_size);
            //still maybe padding
            usbd_usb_ep_read(usbd, rndisd->data_ep, rndisd->usb_rx, (msg->message_length - 1) & ~(rndisd->data_ep_size - 1));
            return;
        }
        if (msg->message_length > rndisd->usb_rx->data_size)
            break;
        if ((msg->data_offset + msg->data_length + offsetof(RNDIS_PACKET_MSG, data_offset) > msg->message_length) || (msg->data_length == 0))
            break;
        data = (uint8_t*)io_data(rndisd->usb_rx) + msg->data_offset + offsetof(RNDIS_PACKET_MSG, data_offset);
        io = NULL;
#if (ETH_DOUBLE_BUFFERING)
        for (i = 0; i < 2; ++i)
            if (rndisd->rx[i] != NULL)
            {
                io = rndisd->rx[i];
                rndisd->rx[i] = NULL;
                break;
            }
#else //ETH_DOUBLE_BUFFERING
        if (rndisd->rx != NULL)
        {
            io = rndisd->rx;
            rndisd->rx = NULL;
        }
#endif //ETH_DOUBLE_BUFFERING
        if (io == NULL)
        {
#if (USBD_RNDIS_DEBUG_FLOW)
            printf("RNDIS device: frame dropped\n");
#endif //USBD_RNDIS_DEBUG_FLOW
            //TODO: stat
            break;
        }
        frame_size = msg->data_length;
        if (frame_size > io_get_free(io))
            frame_size = io_get_free(io);
        memcpy(io_data(io), data, frame_size);
        io->data_size = frame_size;
        io_complete(rndisd->tcpip, HAL_IO_CMD(HAL_ETH, IPC_READ), USBD_IFACE(rndisd->control_iface, 0), io);
#if (USBD_RNDIS_DEBUG_FLOW)
        printf("RNDIS device: RX %d\n", frame_size);
#endif //USBD_RNDIS_DEBUG_FLOW

    } while(false);
    rndisd_rx(usbd, rndisd);
}

void rndisd_write(USBD* usbd, RNDISD* rndisd)
{
}

static inline void rndisd_initialize(USBD* usbd, RNDISD* rndisd, IO* io)
{
    RNDIS_INITIALIZE_MSG* msg;
    RNDIS_INITIALIZE_CMPLT* cmplt;
    if (io->data_size < sizeof(RNDIS_INITIALIZE_MSG))
    {
        rndisd_fail(rndisd);
        return;
    }
    msg = io_data(io);
    //TODO: flush here
    rndisd_reset(usbd, rndisd);
    rndisd->transfer_size = msg->max_transfer_size;
    if (USBD_RNDIS_MAX_PACKET_SIZE < rndisd->transfer_size)
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
    if (rndisd->usb_rx != NULL)
        io_destroy(rndisd->usb_rx);
    rndisd->usb_rx = io_create(rndisd->transfer_size);
    if (rndisd->usb_rx == NULL)
        cmplt->status = RNDIS_STATUS_FAILURE;
#if (USBD_RNDIS_DEBUG_REQUESTS)
    printf("RNDIS device: INITIALIZE, ver%d.%d, size: %d\n", msg->major_version, msg->minor_version, msg->max_transfer_size);
#endif //USBD_RNDIS_DEBUG_REQUESTS
    if (rndisd_link_ready(rndisd))
        rndisd_link_changed(usbd, rndisd);
}

static inline void rndisd_halt(USBD* usbd, RNDISD* rndisd)
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

static inline void rndisd_query_gen_physical_medium(RNDISD* rndisd)
{
    *((uint32_t*)rndisd_query_append(rndisd, sizeof(uint32_t))) = RNDIS_PHYSICAL_MEDIUM_802_3;
#if (USBD_RNDIS_DEBUG_REQUESTS)
    printf("RNDIS device: QUERY physical medium(802.3)\n");
#endif //USBD_RNDIS_DEBUG_REQUESTS
}

static inline void rndisd_query_802_3_permanent_address(RNDISD* rndisd)
{
    MAC* mac;
    mac = rndisd_query_append(rndisd, sizeof(MAC));
    mac->u32.hi = rndisd->mac.u32.hi;
    mac->u32.lo = rndisd->mac.u32.lo;
#if (USBD_RNDIS_DEBUG_REQUESTS)
    printf("RNDIS device: QUERY 802.3 permanent address\n");
#endif //USBD_RNDIS_DEBUG_REQUESTS
}

static inline void rndisd_query(RNDISD* rndisd, IO* io)
{
    RNDIS_QUERY_SET_MSG* msg;
    RNDIS_QUERY_CMPLT* cmplt;
    void* buf;
    if (!rndisd_query_set_check(rndisd, io))
        return;
    msg = io_data(io);
    buf = rndisd_query_set_buf(io);
    cmplt = (RNDIS_QUERY_CMPLT*)rndisd->response;
    cmplt->message_type = REMOTE_NDIS_QUERY_CMPLT;
    rndisd->response_size = cmplt->message_length = sizeof(RNDIS_QUERY_CMPLT);
    cmplt->request_id = msg->request_id;
    cmplt->status = RNDIS_STATUS_SUCCESS;
    cmplt->information_buffer_length = 0;
    cmplt->information_buffer_offset = 0;

    switch (msg->oid)
    {
/*    case OID_GEN_SUPPORTED_LIST:
    case OID_GEN_HARDWARE_STATUS:
    case OID_GEN_MEDIA_SUPPORTED:
    case OID_GEN_MEDIA_IN_USE:
    case OID_GEN_MAXIMUM_FRAME_SIZE:
    case OID_GEN_LINK_SPEED:
    case OID_GEN_TRANSMIT_BLOCK_SIZE:
    case OID_GEN_RECEIVE_BLOCK_SIZE:
    case OID_GEN_VENDOR_ID:
    case OID_GEN_VENDOR_DESCRIPTION:
    case OID_GEN_MAXIMUM_TOTAL_SIZE:
    case OID_GEN_MEDIA_CONNECT_STATUS:*/
    case OID_GEN_PHYSICAL_MEDIUM:
        rndisd_query_gen_physical_medium(rndisd);
        break;
/*    case OID_GEN_RNDIS_CONFIG_PARAMETER:
    case OID_GEN_XMIT_OK:
    case OID_GEN_RCV_OK:
    case OID_GEN_XMIT_ERROR:
    case OID_GEN_RCV_ERROR:
    case OID_GEN_RCV_NO_BUFFER:
    case OID_GEN_DIRECTED_BYTES_XMIT:
    case OID_GEN_DIRECTED_FRAMES_XMIT:
    case OID_GEN_MULTICAST_BYTES_XMIT:
    case OID_GEN_MULTICAST_FRAMES_XMIT:
    case OID_GEN_BROADCAST_BYTES_XMIT:
    case OID_GEN_BROADCAST_FRAMES_XMIT:
    case OID_GEN_DIRECTED_BYTES_RCV:
    case OID_GEN_DIRECTED_FRAMES_RCV:
    case OID_GEN_MULTICAST_BYTES_RCV:
    case OID_GEN_MULTICAST_FRAMES_RCV:
    case OID_GEN_BROADCAST_BYTES_RCV:
    case OID_GEN_BROADCAST_FRAMES_RCV:
    case OID_GEN_RCV_CRC_ERROR:
    case OID_GEN_TRANSMIT_QUEUE_LENGTH:*/
    case OID_802_3_PERMANENT_ADDRESS:
        rndisd_query_802_3_permanent_address(rndisd);
        break;
    case OID_802_3_CURRENT_ADDRESS:
    case OID_802_3_MULTICAST_LIST:
    case OID_802_3_MAXIMUM_LIST_SIZE:
    case OID_802_3_MAC_OPTIONS:
    case OID_802_3_RCV_ERROR_ALIGNMENT:
    case OID_802_3_XMIT_ONE_COLLISION:
    case OID_802_3_XMIT_MORE_COLLISIONS:
    case OID_802_3_XMIT_DEFERRED:
    case OID_802_3_XMIT_MAX_COLLISIONS:
    case OID_802_3_RCV_OVERRUN:
    case OID_802_3_XMIT_UNDERRUN:
    case OID_802_3_XMIT_HEARTBEAT_FAILURE:
    case OID_802_3_XMIT_TIMES_CRS_LOST:
    case OID_802_3_XMIT_LATE_COLLISIONS:
    default:
#if (USBD_RNDIS_DEBUG_REQUESTS)
        printf("RNDIS device: unsupported QUERY Oid: %#X\n", msg->oid);
#endif //USBD_RNDIS_DEBUG_REQUESTS
        if (msg->information_buffer_length)
            dump(io_data(io) + msg->information_buffer_offset + 8, msg->information_buffer_length);
        rndisd_fail(rndisd);
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

static inline void rndisd_set(RNDISD* rndisd, IO* io)
{
    RNDIS_QUERY_SET_MSG* msg;
    RNDIS_SET_CMPLT* cmplt;
    void* buf;
    if (!rndisd_query_set_check(rndisd, io))
        return;
    msg = io_data(io);
    buf = rndisd_query_set_buf(io);
    cmplt = (RNDIS_SET_CMPLT*)rndisd->response;
    cmplt->message_type = REMOTE_NDIS_SET_CMPLT;
    rndisd->response_size = cmplt->message_length = sizeof(RNDIS_SET_CMPLT);
    cmplt->request_id = msg->request_id;
    cmplt->status = RNDIS_STATUS_SUCCESS;

    switch (msg->oid)
    {
/*    case OID_GEN_SUPPORTED_LIST:
    case OID_GEN_HARDWARE_STATUS:
    case OID_GEN_MEDIA_SUPPORTED:
    case OID_GEN_MEDIA_IN_USE:
    case OID_GEN_MAXIMUM_FRAME_SIZE:
    case OID_GEN_LINK_SPEED:
    case OID_GEN_TRANSMIT_BLOCK_SIZE:
    case OID_GEN_RECEIVE_BLOCK_SIZE:
    case OID_GEN_VENDOR_ID:
    case OID_GEN_VENDOR_DESCRIPTION:*/
    case OID_GEN_CURRENT_PACKET_FILTER:
        rndisd_set_packet_filter(rndisd, buf, msg->information_buffer_length);
        break;
/*    case OID_GEN_MAXIMUM_TOTAL_SIZE:
    case OID_GEN_MEDIA_CONNECT_STATUS:*/
    case OID_GEN_PHYSICAL_MEDIUM:
  //      rndisd_gen_oid_physical_medium(rndisd);
  //      break;
/*    case OID_GEN_RNDIS_CONFIG_PARAMETER:
    case OID_GEN_XMIT_OK:
    case OID_GEN_RCV_OK:
    case OID_GEN_XMIT_ERROR:
    case OID_GEN_RCV_ERROR:
    case OID_GEN_RCV_NO_BUFFER:
    case OID_GEN_DIRECTED_BYTES_XMIT:
    case OID_GEN_DIRECTED_FRAMES_XMIT:
    case OID_GEN_MULTICAST_BYTES_XMIT:
    case OID_GEN_MULTICAST_FRAMES_XMIT:
    case OID_GEN_BROADCAST_BYTES_XMIT:
    case OID_GEN_BROADCAST_FRAMES_XMIT:
    case OID_GEN_DIRECTED_BYTES_RCV:
    case OID_GEN_DIRECTED_FRAMES_RCV:
    case OID_GEN_MULTICAST_BYTES_RCV:
    case OID_GEN_MULTICAST_FRAMES_RCV:
    case OID_GEN_BROADCAST_BYTES_RCV:
    case OID_GEN_BROADCAST_FRAMES_RCV:
    case OID_GEN_RCV_CRC_ERROR:
    case OID_GEN_TRANSMIT_QUEUE_LENGTH:*/
    case OID_802_3_PERMANENT_ADDRESS:
//        rndisd_oid_802_3_permanent_address(rndisd);
//        break;
    case OID_802_3_CURRENT_ADDRESS:
    case OID_802_3_MULTICAST_LIST:
    case OID_802_3_MAXIMUM_LIST_SIZE:
    case OID_802_3_MAC_OPTIONS:
    case OID_802_3_RCV_ERROR_ALIGNMENT:
    case OID_802_3_XMIT_ONE_COLLISION:
    case OID_802_3_XMIT_MORE_COLLISIONS:
    case OID_802_3_XMIT_DEFERRED:
    case OID_802_3_XMIT_MAX_COLLISIONS:
    case OID_802_3_RCV_OVERRUN:
    case OID_802_3_XMIT_UNDERRUN:
    case OID_802_3_XMIT_HEARTBEAT_FAILURE:
    case OID_802_3_XMIT_TIMES_CRS_LOST:
    case OID_802_3_XMIT_LATE_COLLISIONS:
    default:
#if (USBD_RNDIS_DEBUG_REQUESTS)
        printf("RNDIS device: unsupported SET Oid: %#X\n", msg->oid);
#endif //USBD_RNDIS_DEBUG_REQUESTS
        if (msg->information_buffer_length)
            dump(io_data(io) + msg->information_buffer_offset + 8, msg->information_buffer_length);
        rndisd_fail(rndisd);
    }
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
        rndisd->busy = true;
        //Fucking Microsoft doesn't use SETUP, they made own interface
        switch (msg->message_type)
        {
        case REMOTE_NDIS_INITIALIZE_MSG:
            rndisd_initialize(usbd, rndisd, io);
            break;
        case REMOTE_NDIS_HALT_MSG:
            rndisd_halt(usbd, rndisd);
            //no need to respond on HALT
            return 0;
        case REMOTE_NDIS_QUERY_MSG:
            rndisd_query(rndisd, io);
            break;
        case REMOTE_NDIS_SET_MSG:
            rndisd_set(rndisd, io);
            break;
        case REMOTE_NDIS_RESET_MSG:
//            break;
        case REMOTE_NDIS_INDICATE_STATUS_MSG:
//            break;
        case REMOTE_NDIS_KEEPALIVE_MSG:
//            break;
        default:
#if (USBD_RNDIS_DEBUG)
            printf("RNDIS device: unsupported MSG %#X\n", msg->message_type);
#endif //USBD_RNDIS_DEBUG
            //TODO: format unsupported
            dump(io_data(io), io->data_size);
        }

    } while (false);
    rndisd_notify(usbd, rndisd);
    return 0;
}

static inline int rndisd_get_encapsulated_response(RNDISD* rndisd, IO* io)
{
    memcpy(io_data(io), rndisd->response, rndisd->response_size);
    io->data_size = rndisd->response_size;
    rndisd->busy = false;
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
        rndisd_write(usbd, rndisd);
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
    }
}

static inline void rndisd_eth_set_mac(RNDISD* rndisd, unsigned int param2, unsigned int param3)
{
    rndisd->mac.u32.hi = param2;
    rndisd->mac.u32.lo = (uint16_t)param3;
}

static inline void rndisd_eth_get_mac(RNDISD* rndisd, IPC* ipc)
{
    ipc->param2 = rndisd->mac.u32.hi;
    ipc->param3 = rndisd->mac.u32.lo;
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

static inline void rndisd_eth_read(RNDISD* rndisd, IO* io)
{
#if (ETH_DOUBLE_BUFFERING)
    int i;
#endif //ETH_DOUBLE_BUFFERING
    io_reset(io);
    if (!rndisd_link_ready(rndisd))
    {
        error(ERROR_NOT_ACTIVE);
        return;
    }
#if (ETH_DOUBLE_BUFFERING)
    for (i = 0; i < 2; ++i)
        if (rndisd->rx[i] == NULL)
        {
            rndisd->rx[i] = io;
            error(ERROR_SYNC);
            return;
        }
#else //ETH_DOUBLE_BUFFERING
    if (rndisd->rx == NULL)
    {
        rndisd->rx = io;
        error(ERROR_SYNC);
        return;
    }
#endif //ETH_DOUBLE_BUFFERING
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
    case IPC_OPEN:
        rndisd_eth_open(usbd, rndisd, ipc->process);
        break;
    case IPC_CLOSE:
        printf("TODO: IPC close\n");
        error(ERROR_NOT_SUPPORTED);
        break;
    case IPC_READ:
        rndisd_eth_read(rndisd, (IO*)ipc->param2);
        break;
    case IPC_WRITE:
        printf("TODO: IPC write\n");
        error(ERROR_NOT_SUPPORTED);
        break;
    case IPC_FLUSH:
        printf("TODO: IPC flush\n");
        error(ERROR_NOT_SUPPORTED);
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

static inline void rndisd_iface_event(USBD* usbd, RNDISD* rndisd, IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
    case RNDIS_SET_LINK:
        rndisd_set_link(usbd, rndisd, (ETH_CONN_TYPE)ipc->param2);
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
