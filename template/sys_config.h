/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef SYS_CONFIG_H
#define SYS_CONFIG_H

/*
    config.h - userspace config
 */

//add some debug info in SYS
#define SYS_INFO                                            1

//will save few bytes, but not recommended to disable
#define LIB_CHECK_PRESENCE                                  0
//----------------------------- objects ----------------------------------------------
//make sure, you know what are you doing, before change
#define SYS_OBJ_STDOUT                                      0
#define SYS_OBJ_STDIN                                       1
#define SYS_OBJ_CORE                                        2
#define SYS_OBJ_USBD                                        3
#define SYS_OBJ_ETH                                         4
#define SYS_OBJ_TCPIP                                       5

#define SYS_OBJ_I2C                                         SYS_OBJ_CORE
#define SYS_OBJ_UART                                        SYS_OBJ_CORE
#define SYS_OBJ_USB                                         SYS_OBJ_CORE
#define SYS_OBJ_ANALOG                                      SYS_OBJ_CORE
//------------------------------ stdio -----------------------------------------------
#define STDIO_STREAM_SIZE                                   32
//-------------------------------- USB -----------------------------------------------
#define USB_EP_COUNT_MAX                                    4
//low-level USB debug. Turn on only in case of IO problems
#define USB_DEBUG_ERRORS                                    1
//support for high speed, qualifier, other speed, test, etc
#define USB_2_0                                             0

//----------------------------- USB device--------------------------------------------
//all other device-related debug depends on this
#define USBD_DEBUG                                          1
#define USBD_DEBUG_ERRORS                                   1
#define USBD_DEBUG_REQUESTS                                 0

//vendor-specific requests support
#define USBD_VSR                                            1

#define USBD_PROCESS_SIZE                                   900
#define USBD_IPC_COUNT                                      10
#define USBD_BLOCK_SIZE                                     256

#define USBD_CDC_CLASS                                      1
#define USBD_HID_KBD_CLASS                                  0
#define USBD_CCID_CLASS                                     0

//----------------------------- CDCD class --------------------------------------------
//At least EP size required, or data will be lost. Double EP size is recommended
#define USBD_CDC_TX_STREAM_SIZE                             64
#define USBD_CDC_RX_STREAM_SIZE                             64

#define USBD_CDC_DEBUG_ERRORS                               1
#define USBD_CDC_DEBUG_REQUESTS                             1
#define USBD_CDC_DEBUG_IO                                   0

//------------------------------ HIDD class -------------------------------------------
#define USBD_HID_DEBUG_ERRORS                               1
#define USBD_HID_DEBUG_REQUESTS                             1
#define USBD_HID_DEBUG_IO                                   0

//----------------------------- CCIDD class -------------------------------------------
#define USBD_CCID_MAX_ATR_SIZE                              32

#define USBD_CCID_DEBUG_ERRORS                              1
#define USBD_CCID_DEBUG_REQUESTS                            1
#define USBD_CCID_DEBUG_IO                                  0

//------------------------------ PIN board -------------------------------------------
#define PINBOARD_PROCESS_SIZE                               400
#define PINBOARD_POLL_TIME_MS                               100

//--------------------------------- ETH ----------------------------------------------
#define ETH_PHY_ADDRESS                                     0x1f
#define ETH_AUTO_NEGOTIATION_TIME                           5000

#define ETH_DOUBLE_BUFFERING                                1
//------------------------------- TCP/IP ---------------------------------------------
#define TCPIP_PROCESS_SIZE                                  700
#define TCPIP_PROCESS_PRIORITY                              150
#define TCPIP_PROCESS_IPC_COUNT                             5

#define TCPIP_DEBUG                                         1
#define TCPIP_DEBUG_ERRORS                                  1

#define TCPIP_MTU                                           1500
#define TCPIP_MAX_FRAMES_COUNT                              20

//----------------------------- TCP/IP MAC --------------------------------------------
//software MAC filter. Turn on in case of hardware is not supporting
#define MAC_FILTER                                          1
#define MAC_DEBUG                                           0

//----------------------------- TCP/IP ARP --------------------------------------------
#define ARP_DEBUG                                           1
#define ARP_DEBUG_FLOW                                      0

#define ARP_CACHE_SIZE_MAX                                  10
//in seconds
#define ARP_CACHE_INCOMPLETE_TIMEOUT                        5
#define ARP_CACHE_TIMEOUT                                   600

//----------------------------- TCP/IP IP ---------------------------------------------
#define IP_DEBUG                                            1
#define IP_DEBUG_FLOW                                       0

//set, if not supported by hardware
#define IP_CHECKSUM                                         1

#define IP_FRAGMENTATION                                    1
#define IP_FRAGMENT_ASSEMBLY_TIMEOUT                        10
#define IP_MAX_LONG_SIZE                                    5000
#define IP_MAX_LONG_PACKETS                                 5

//---------------------------- TCP/IP ICMP --------------------------------------------
#define ICMP                                                1
//reply on ICMP echo and echo request
#define ICMP_ECHO                                           1
#define ICMP_ECHO_REPLY                                     1
#define ICMP_ECHO_TIMEOUT                                   5

#define ICMP_DEBUG                                          1


#endif // SYS_CONFIG_H
