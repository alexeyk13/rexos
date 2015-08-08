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

//will save few bytes, but not recommended to disable
#define LIB_CHECK_PRESENCE                                  0
//----------------------------- objects ----------------------------------------------
//make sure, you know what are you doing, before change
#define SYS_OBJ_STDOUT                                      0
#define SYS_OBJ_CORE                                        1

#define SYS_OBJ_UART                                        SYS_OBJ_CORE
#define SYS_OBJ_USB                                         SYS_OBJ_CORE
#define SYS_OBJ_ADC                                         SYS_OBJ_CORE
#define SYS_OBJ_DAC                                         SYS_OBJ_CORE
#define SYS_OBJ_STDIN                                       INVALID_HANDLE
//------------------------------ POWER -----------------------------------------------
//depends on hardware implementation
#define POWER_MANAGEMENT_SUPPORT                            1
#define LOW_POWER_ON_STARTUP                                0
//#define LOW_POWER_ON_STARTUP                                1
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
#define USBD_DEBUG_REQUESTS                                 1

//vendor-specific requests support
#define USBD_VSR                                            0

#define USBD_PROCESS_SIZE                                   800
#define USBD_BLOCK_SIZE                                     128

#define USBD_CDC_CLASS                                      1
#define USBD_HID_KBD_CLASS                                  0
#define USBD_CCID_CLASS                                     0
#define USBD_MSC_CLASS                                      0

//----------------------------- CDCD class --------------------------------------------
//At least EP size required, or data will be lost. Double EP size is recommended
#define USBD_CDC_TX_STREAM_SIZE                             32
#define USBD_CDC_RX_STREAM_SIZE                             32

#define USBD_CDC_DEBUG_ERRORS                               0
#define USBD_CDC_DEBUG_REQUESTS                             0
#define USBD_CDC_DEBUG_IO                                   0

//------------------------------ HIDD class -------------------------------------------
#define USBD_HID_DEBUG_ERRORS                               0
#define USBD_HID_DEBUG_REQUESTS                             0
#define USBD_HID_DEBUG_IO                                   0

//----------------------------- CCIDD class -------------------------------------------
#define USBD_CCID_REMOVABLE_CARD                            0

#define USBD_CCID_DEBUG_ERRORS                              0
#define USBD_CCID_DEBUG_REQUESTS                            0
#define USBD_CCID_DEBUG_IO                                  0

//------------------------------ MSCD class -------------------------------------------
#define USBD_MSC_DEBUG_ERRORS                               0
#define USBD_MSC_DEBUG_REQUESTS                             0
#define USBD_MSC_DEBUG_IO                                   0

//only one LUN supported for now
#define USBD_MSC_LUN_COUNT                                  1
//-------------------------------- SCSI ----------------------------------------------
#define SCSI_SENSE_DEPTH                                    10
//can be disabled for flash memory saving
#define SCSI_LONG_LBA                                       0
#define SCSI_VERIFY_SUPPORTED                               0
//send PASS before data was written
#define SCSI_WRITE_CACHE                                    1
//write cache directly from application to usbd
#define SCSI_READ_CACHE                                     1
//SATA over SCSI. Just stub for more verbose error processing
//Found on some linux recent kernels
#define SCSI_SAT                                            0
//exclude SCSI stack. Generally sector_size * num_sectors
#define SCSI_IO_SIZE                                        512

#define SCSI_DEBUG_REQUESTS                                 1
#define SCSI_DEBUG_ERRORS                                   1

//------------------------------ PIN board -------------------------------------------
#define PINBOARD_PROCESS_SIZE                               500
#define PINBOARD_POLL_TIME_MS                               100
//--------------------------------- DAC ----------------------------------------------
#define SAMPLE                                              uint16_t
//disable for some flash saving
#define WAVEGEN_SQUARE                                      1
#define WAVEGEN_TRIANGLE                                    0
#define WAVEGEN_SINE                                        0
//--------------------------------- ETH ----------------------------------------------
#define ETH_PHY_ADDRESS                                     0x1f
#define ETH_AUTO_NEGOTIATION_TIME                           5000

#define ETH_DOUBLE_BUFFERING                                1
//------------------------------- TCP/IP ---------------------------------------------
#define TCPIP_PROCESS_SIZE                                  700
#define TCPIP_PROCESS_PRIORITY                              150

#define TCPIP_DEBUG                                         1
#define TCPIP_DEBUG_ERRORS                                  1

#define TCPIP_MTU                                           1500
#define TCPIP_MAX_FRAMES_COUNT                              20

//----------------------------- TCP/IP MAC --------------------------------------------
//software MAC filter. Turn on in case of hardware is not supporting
#define MAC_FILTER                                          1
#define TCPIP_MAC_DEBUG                                     0

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
