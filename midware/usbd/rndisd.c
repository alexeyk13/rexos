/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "rndisd.h"
#include "usbd.h"
#include "../../userspace/sys.h"
#include "../../userspace/mac.h"
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
    IO* rx;
    IO* tx;
    IO* notify;
    unsigned int transfer_size;
    uint32_t packet_filter;
    uint8_t response[RNDIS_RESPONSE_SIZE];
    uint8_t response_size;
    uint8_t data_ep, control_ep;
    uint16_t data_ep_size, rx_free, tx_size;
    uint8_t tx_idle, data_iface, control_iface;
    bool suspended;
} RNDISD;

#define SEND_ENCAPSULATED_COMMAND                                               0x00
#define GET_ENCAPSULATED_RESPONSE                                               0x01

#define RNDIS_RESPONSE_AVAILABLE                                                0x00000001

//The host and device use this to send network data to one another.
#define REMOTE_NDIS_PACKET_MSG                                                  0x00000001
//Sent by the host to initialize the device
#define REMOTE_NDIS_INITIALIZE_MSG                                              0x00000002
//Device response to an initialize message
#define REMOTE_NDIS_INITIALIZE_CMPLT                                            0x80000002
//Sent by the host to halt the device. This does
//not have a response. It is optional for the
//device to send this message to the host
#define REMOTE_NDIS_HALT_MSG                                                    0x00000003
//Sent by the host to send a query OID
#define REMOTE_NDIS_QUERY_MSG                                                   0x00000004
//Device response to a query OID
#define REMOTE_NDIS_QUERY_CMPLT                                                 0x80000004
//Sent by the host to send a set OID
#define REMOTE_NDIS_SET_MSG                                                     0x00000005
//Device response to a set OID
#define REMOTE_NDIS_SET_CMPLT                                                   0x80000005
//Sent by the host to perform a soft reset on the device
#define REMOTE_NDIS_RESET_MSG                                                   0x00000006
//Device response to reset message
#define REMOTE_NDIS_RESET_CMPLT                                                 0x80000006
//Sent by the device to indicate its status or an
//error when an unrecognized message is received
#define REMOTE_NDIS_INDICATE_STATUS_MSG                                         0x00000007
//During idle periods, sent every few seconds
//by the host to check that the device is still
//responsive. It is optional for the device to
//send this message to check if the host is active
#define REMOTE_NDIS_KEEPALIVE_MSG                                               0x00000008
//The device response to a keepalive
//message. The host can respond with this
//message to a keepalive message from the
//device when the device implements the
//optional KeepAliveTimer
#define REMOTE_NDIS_KEEPALIVE_CMPLT                                             0x80000008

//Success
#define RNDIS_STATUS_SUCCESS                                                    0x00000000
//Unspecified error
#define RNDIS_STATUS_FAILURE                                                    0xC0000001
//Invalid data error
#define RNDIS_STATUS_INVALID_DATA                                               0xC0010015
//Unsupported request error
#define RNDIS_STATUS_NOT_SUPPORTED                                              0xC00000BB
//Device is connected to a network medium
#define RNDIS_STATUS_MEDIA_CONNECT                                              0x4001000B
//Device is disconnected from the medium
#define RNDIS_STATUS_MEDIA_DISCONNECT                                           0x4001000C

#define RNDIS_DF_CONNECTIONLESS                                                 0x00000001
#define RNDIS_DF_CONNECTION_ORIENTED                                            0x00000002

#define RNDIS_MEDIUM_802_3                                                      0x00000000

#define RNDIS_VERSION_MAJOR                                                     1
#define RNDIS_VERSION_MINOR                                                     0

//General OIDs
#define OID_GEN_SUPPORTED_LIST                                                  0x00010101
#define OID_GEN_HARDWARE_STATUS                                                 0x00010102
#define OID_GEN_MEDIA_SUPPORTED                                                 0x00010103
#define OID_GEN_MEDIA_IN_USE                                                    0x00010104
#define OID_GEN_MAXIMUM_FRAME_SIZE                                              0x00010106
#define OID_GEN_LINK_SPEED                                                      0x00010107
#define OID_GEN_TRANSMIT_BLOCK_SIZE                                             0x0001010A
#define OID_GEN_RECEIVE_BLOCK_SIZE                                              0x0001010B
#define OID_GEN_VENDOR_ID                                                       0x0001010C
#define OID_GEN_VENDOR_DESCRIPTION                                              0x0001010D
#define OID_GEN_CURRENT_PACKET_FILTER                                           0x0001010E
#define OID_GEN_MAXIMUM_TOTAL_SIZE                                              0x00010111
#define OID_GEN_MEDIA_CONNECT_STATUS                                            0x00010114
#define OID_GEN_PHYSICAL_MEDIUM                                                 0x00010202
#define OID_GEN_RNDIS_CONFIG_PARAMETER                                          0x0001021B

// General Statistics OIDs
#define OID_GEN_XMIT_OK                                                         0x00020101
#define OID_GEN_RCV_OK                                                          0x00020102
#define OID_GEN_XMIT_ERROR                                                      0x00020103
#define OID_GEN_RCV_ERROR                                                       0x00020104
#define OID_GEN_RCV_NO_BUFFER                                                   0x00020105
#define OID_GEN_DIRECTED_BYTES_XMIT                                             0x00020201
#define OID_GEN_DIRECTED_FRAMES_XMIT                                            0x00020202
#define OID_GEN_MULTICAST_BYTES_XMIT                                            0x00020203
#define OID_GEN_MULTICAST_FRAMES_XMIT                                           0x00020204
#define OID_GEN_BROADCAST_BYTES_XMIT                                            0x00020205
#define OID_GEN_BROADCAST_FRAMES_XMIT                                           0x00020206
#define OID_GEN_DIRECTED_BYTES_RCV                                              0x00020207
#define OID_GEN_DIRECTED_FRAMES_RCV                                             0x00020208
#define OID_GEN_MULTICAST_BYTES_RCV                                             0x00020209
#define OID_GEN_MULTICAST_FRAMES_RCV                                            0x0002020A
#define OID_GEN_BROADCAST_BYTES_RCV                                             0x0002020B
#define OID_GEN_BROADCAST_FRAMES_RCV                                            0x0002020C
#define OID_GEN_RCV_CRC_ERROR                                                   0x0002020D
#define OID_GEN_TRANSMIT_QUEUE_LENGTH                                           0x0002020E

//802.3 NDIS OIDs
#define OID_802_3_PERMANENT_ADDRESS                                             0x01010101
#define OID_802_3_CURRENT_ADDRESS                                               0x01010102
#define OID_802_3_MULTICAST_LIST                                                0x01010103
#define OID_802_3_MAXIMUM_LIST_SIZE                                             0x01010104
#define OID_802_3_MAC_OPTIONS                                                   0x01010105

//802.3 Statistics NDIS OIDs
#define OID_802_3_RCV_ERROR_ALIGNMENT                                           0x01020101
#define OID_802_3_XMIT_ONE_COLLISION                                            0x01020102
#define OID_802_3_XMIT_MORE_COLLISIONS                                          0x01020103
#define OID_802_3_XMIT_DEFERRED                                                 0x01020201
#define OID_802_3_XMIT_MAX_COLLISIONS                                           0x01020202
#define OID_802_3_RCV_OVERRUN                                                   0x01020203
#define OID_802_3_XMIT_UNDERRUN                                                 0x01020204
#define OID_802_3_XMIT_HEARTBEAT_FAILURE                                        0x01020205
#define OID_802_3_XMIT_TIMES_CRS_LOST                                           0x01020206
#define OID_802_3_XMIT_LATE_COLLISIONS                                          0x01020207

typedef enum {
    RNDIS_PHYSICAL_MEDIUM_UNSPECIFIED = 0,
    RNDIS_PHYSICAL_MEDIUM_WIRELESS_LAN,
    RNDIS_PHYSICAL_MEDIUM_CABLE_MODEM,
    RNDIS_PHYSICAL_MEDIUM_PHONE_LINE,
    RNDIS_PHYSICAL_MEDIUM_POWER_LINE,
    RNDIS_PHYSICAL_MEDIUM_DSL,
    RNDIS_PHYSICAL_MEDIUM_FIBRE_CHANNEL,
    RNDIS_PHYSICAL_MEDIUM_1394,
    RNDIS_PHYSICAL_MEDIUM_WIRELESS_WAN,
    RNDIS_PHYSICAL_MEDIUM_NATIVE_802_11,
    RNDIS_PHYSICAL_MEDIUM_BLUETOOTH,
    RNDIS_PHYSICAL_MEDIUM_INFINIBAND,
    RNDIS_PHYSICAL_MEDIUM_WIMAX,
    RNDIS_PHYSICAL_MEDIUM_UMB,
    RNDIS_PHYSICAL_MEDIUM_802_3,
    RNDIS_PHYSICAL_MEDIUM_802_5,
    RNDIS_PHYSICAL_MEDIUM_IRDA,
    RNDIS_PHYSICAL_MEDIUM_WIRED_WAN,
    RNDIS_PHYSICAL_MEDIUM_WIRED_CO_WAN,
    RNDIS_PHYSICAL_MEDIUM_OTHER
} RNDIS_PHYSICAL_MEDIUM;

#define RNDIS_PACKET_TYPE_DIRECTED                                              0x00000001
#define RNDIS_PACKET_TYPE_MULTICAST                                             0x00000002
#define RNDIS_PACKET_TYPE_ALL_MULTICAST                                         0x00000004
#define RNDIS_PACKET_TYPE_BROADCAST                                             0x00000008
#define RNDIS_PACKET_TYPE_SOURCE_ROUTING                                        0x00000010
#define RNDIS_PACKET_TYPE_PROMISCUOUS                                           0x00000020
#define RNDIS_PACKET_TYPE_SMT                                                   0x00000040
#define RNDIS_PACKET_TYPE_ALL_LOCAL                                             0x00000080
#define RNDIS_PACKET_TYPE_GROUP                                                 0x00001000
#define RNDIS_PACKET_TYPE_ALL_FUNCTIONAL                                        0x00002000
#define RNDIS_PACKET_TYPE_FUNCTIONAL                                            0x00004000
#define RNDIS_PACKET_TYPE_MAC_FRAME                                             0x00008000

#pragma pack(push, 1)
typedef struct {
    uint32_t notify;
    uint32_t reserved;
} RNDIS_RESPONSE_AVAILABLE_NOTIFY;

typedef struct {
    uint32_t message_type;
    uint32_t message_length;
    uint32_t request_id;
} RNDIS_GENERIC_MSG;

typedef struct {
    uint32_t message_type;
    uint32_t message_length;
    uint32_t request_id;
    uint32_t major_version;
    uint32_t minor_version;
    uint32_t max_transfer_size;
} RNDIS_INITIALIZE_MSG;

typedef struct {
    uint32_t message_type;
    uint32_t message_length;
    uint32_t request_id;
    uint32_t status;
    uint32_t major_version;
    uint32_t minor_version;
    uint32_t device_flags;
    uint32_t medium;
    uint32_t max_packets_per_transfer;
    uint32_t max_transfer_size;
    uint32_t packet_alignment_factor;
    uint32_t reserved[2];
} RNDIS_INITIALIZE_CMPLT;

typedef struct {
    uint32_t message_type;
    uint32_t message_length;
    uint32_t request_id;
    uint32_t oid;
    uint32_t information_buffer_length;
    uint32_t information_buffer_offset;
    uint32_t device_vc_handle;
} RNDIS_QUERY_SET_MSG;

typedef struct {
    uint32_t message_type;
    uint32_t message_length;
    uint32_t request_id;
    uint32_t status;
    uint32_t information_buffer_length;
    uint32_t information_buffer_offset;
} RNDIS_QUERY_CMPLT;

typedef struct {
    uint32_t message_type;
    uint32_t message_length;
    uint32_t request_id;
    uint32_t status;
} RNDIS_SET_CMPLT;
#pragma pack(pop)

static void rndisd_destroy(RNDISD* rndisd)
{
    io_destroy(rndisd->rx);
    io_destroy(rndisd->tx);

    free(rndisd);
}

static void rndisd_reset(RNDISD* rndisd)
{
    if (rndisd->rx != NULL)
    {
        io_destroy(rndisd->rx);
        rndisd->rx = NULL;
    }
    rndisd->packet_filter = RNDIS_PACKET_TYPE_DIRECTED | RNDIS_PACKET_TYPE_ALL_MULTICAST | RNDIS_PACKET_TYPE_BROADCAST | RNDIS_PACKET_TYPE_PROMISCUOUS;
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
        rndisd->tx = rndisd->rx = NULL;
        rndisd->notify = io_create(control_ep_size);
        rndisd->suspended = false;

        usbd_usb_ep_open(usbd, USB_EP_IN | rndisd->data_ep, USB_EP_BULK, rndisd->data_ep_size);
        usbd_usb_ep_open(usbd, rndisd->data_ep, USB_EP_BULK, rndisd->data_ep_size);
        usbd_usb_ep_open(usbd, USB_EP_IN | rndisd->control_ep, USB_EP_INTERRUPT, control_ep_size);

        usbd_register_interface(usbd, rndisd->control_iface, &__RNDISD_CLASS, rndisd);
        usbd_register_interface(usbd, rndisd->data_iface, &__RNDISD_CLASS, rndisd);
        usbd_register_endpoint(usbd, rndisd->data_iface, rndisd->data_ep);
        usbd_register_endpoint(usbd, rndisd->control_iface, rndisd->control_ep);
        rndisd_reset(rndisd);
    }
}

void rndisd_class_reset(USBD* usbd, void* param)
{
    RNDISD* rndisd = (RNDISD*)param;

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
    RNDISD* rndisd = (RNDISD*)param;
    usbd_usb_ep_flush(usbd, USB_EP_IN | rndisd->control_ep);
    usbd_usb_ep_flush(usbd, USB_EP_IN | rndisd->data_ep);
    usbd_usb_ep_flush(usbd, rndisd->data_ep);

    rndisd->suspended = true;
}

void rndisd_class_resume(USBD* usbd, void* param)
{
    RNDISD* rndisd = (RNDISD*)param;
    rndisd->suspended = false;
}

static inline void rndisd_read_complete(USBD* usbd, RNDISD* rndisd)
{
    if (rndisd->suspended)
        return;

}

void rndisd_write(USBD* usbd, RNDISD* rndisd)
{
    if (!rndisd->tx_idle || rndisd->suspended)
        return;

}

static inline void rndisd_initialize(RNDISD* rndisd, IO* io)
{
    RNDIS_INITIALIZE_MSG* msg;
    RNDIS_INITIALIZE_CMPLT* cmplt;
    if (io->data_size < sizeof(RNDIS_INITIALIZE_MSG))
    {
        rndisd_fail(rndisd);
        return;
    }
    msg = io_data(io);
    rndisd_reset(rndisd);
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
#if (ETH_DOUBLE_BUFFERING)
    cmplt->max_packets_per_transfer = 2;
#else //ETH_DOUBLE_BUFFERING
    cmplt->max_packets_per_transfer = 1;
#endif //ETH_DOUBLE_BUFFERING
    cmplt->max_transfer_size = rndisd->transfer_size;
    //The value is specified as an exponent of 2
    cmplt->packet_alignment_factor = 2;
    //Reserved for future use and MUST be set to zero. It SHOULD be treated as an error otherwise
    cmplt->reserved[0] = cmplt->reserved[1] = 0x00000000;
    if (rndisd->rx != NULL)
        io_destroy(rndisd->rx);
    rndisd->rx = io_create(rndisd->transfer_size);
    if (rndisd->rx == NULL)
        cmplt->status = RNDIS_STATUS_FAILURE;
#if (USBD_RNDIS_DEBUG_REQUESTS)
    printf("RNDIS device: INITIALIZE, ver%d.%d, size: %d\n", msg->major_version, msg->minor_version, msg->max_transfer_size);
#endif //USBD_RNDIS_DEBUG_REQUESTS
}

static inline void rndisd_halt(RNDISD* rndisd)
{
    if (rndisd->rx != NULL)
    {
        io_destroy(rndisd->rx);
        rndisd->rx = NULL;
    }
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
    //TODO:
    mac->u8[0] = 0x20;
    mac->u8[1] = 0xD9;
    mac->u8[2] = 0x97;
    mac->u8[3] = 0xA1;
    mac->u8[4] = 0x90;
    mac->u8[5] = 0x42;
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
    RNDIS_RESPONSE_AVAILABLE_NOTIFY* resp;
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
        //Fucking Microsoft doesn't use SETUP, they made own interface
        switch (msg->message_type)
        {
        case REMOTE_NDIS_INITIALIZE_MSG:
            rndisd_initialize(rndisd, io);
            break;
        case REMOTE_NDIS_HALT_MSG:
            rndisd_halt(rndisd);
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
            printf("RNDISD: unsupported MSG %#X\n", msg->message_type);
    #endif //USBD_RNDIS_DEBUG
            //TODO: format unsupported
            dump(io_data(io), io->data_size);
        }

    } while (false);
    //notify host
    resp = io_data(rndisd->notify);
    resp->notify = RNDIS_RESPONSE_AVAILABLE;
    resp->reserved = 0;
    rndisd->notify->data_size = sizeof(RNDIS_RESPONSE_AVAILABLE_NOTIFY);
    usbd_usb_ep_write(usbd, USB_EP_IN | rndisd->control_ep, rndisd->notify);
    return 0;
}

static inline int rndisd_get_encapsulated_response(RNDISD* rndisd, IO* io)
{
    memcpy(io_data(io), rndisd->response, rndisd->response_size);
    io->data_size = rndisd->response_size;
    return rndisd->response_size;
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
        rndisd_read_complete(usbd, rndisd);
        break;
    case IPC_WRITE:
        rndisd_write(usbd, rndisd);
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
    }
}

void rndisd_class_request(USBD* usbd, void* param, IPC* ipc)
{
    RNDISD* rndisd = (RNDISD*)param;
    if (HAL_GROUP(ipc->cmd) == HAL_USB)
        rndisd_driver_event(usbd, rndisd, ipc);
    else
        switch (HAL_ITEM(ipc->cmd))
        {
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
