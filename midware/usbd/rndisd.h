/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef RNDISD_H
#define RNDISD_H

#include "usbd.h"

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
#define RNDIS_STATUS_MULTICAST_FULL                                             0xC0010009

#define RNDIS_DF_CONNECTIONLESS                                                 0x00000001
#define RNDIS_DF_CONNECTION_ORIENTED                                            0x00000002

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

typedef enum {
    RNDIS_HARDWARE_STATUS_READY = 0,
    RNDIS_HARDWARE_STATUS_INITIALIZING,
    RNDIS_HARDWARE_STATUS_RESET,
    RNDIS_HARDWARE_STATUS_CLOSING,
    RNDIS_HARDWARE_STATUS_NOT_READY
} RNDIS_HARDWARE_STATUS;

typedef enum {
    RNDIS_MEDIUM_802_3 = 0,
    RNDIS_MEDIUM_802_5,
    RNDIS_MEDIUM_FDDI,
    RNDIS_MEDIUM_WAN,
    RNDIS_MEDIUM_LOCAL_TALK,
    RNDIS_MEDIUM_DIX,
    RNDIS_MEDIUM_ARCNET_RAW,
    RNDIS_MEDIUM_ARCNET878_2,
    RNDIS_MEDIUM_ATM,
    RNDIS_MEDIUM_802_11,
    RNDIS_MEDIUM_WIRELESS_WAN,
    RNDIS_MEDIUM_IRDA,
    RNDIS_MEDIUM_CO_WAN,
    RNDIS_MEDIUM_1394,
    RNDIS_MEDIUM_BPC,
    RNDIS_MEDIUM_INFINI_BAND,
    RNDIS_MEDIUM_TUNNEL,
    RNDIS_MEDIUM_LOOPBACK,
    RNDIS_MEDIUM_IP
} RNDIS_MEDIUM;

typedef enum {
    RNDIS_MEDIA_STATE_CONNECTED = 0,
    RNDIS_MEDIA_STATE_DISCONNECTED
} RNDIS_MEDIA_STATE;

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
} RNDIS_SET_KEEP_ALIVE_CMPLT;

typedef struct {
    uint32_t message_type;
    uint32_t message_length;
    uint32_t status;
    uint32_t addressing_reset;
} RNDIS_RESET_CMPLT;

typedef struct {
    uint32_t message_type;
    uint32_t message_length;
    uint32_t status;
    uint32_t information_buffer_length;
    uint32_t information_buffer_offset;
} RNDIS_INDICATE_STATUS_MSG;

typedef struct {
    uint32_t message_type;
    uint32_t message_length;
    uint32_t data_offset;
    uint32_t data_length;
    uint32_t out_of_band_data_offset;
    uint32_t out_of_band_data_length;
    uint32_t out_of_band_num_elements;
    uint32_t per_packet_info_offset;
    uint32_t per_packet_info_length;
    uint32_t reserved[2];
} RNDIS_PACKET_MSG;

#pragma pack(pop)

extern const USBD_CLASS __RNDISD_CLASS;

#endif // RNDISD_H
