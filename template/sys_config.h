/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef SYS_CONFIG_H
#define SYS_CONFIG_H

//----------------------------- generic ----------------------------------------------
//add some debug info in SYS
#define SYS_INFO                                            0
//will save few bytes, but not recommended to disable
#define LIB_CHECK_PRESENCE                                  0
//----------------------------- objects ----------------------------------------------
//make sure, you know what are you doing, before change
#define SYS_OBJ_STDOUT                                      0
#define SYS_OBJ_CORE                                        1
#define SYS_OBJ_USBD                                        2
#define SYS_OBJ_PINBOARD                                    3

#define SYS_OBJ_I2C                                         SYS_OBJ_CORE
#define SYS_OBJ_UART                                        SYS_OBJ_CORE
#define SYS_OBJ_USB                                         SYS_OBJ_CORE
#define SYS_OBJ_STDIN                                       INVALID_HANDLE
//------------------------------ stdio -----------------------------------------------
#define STDIO_STREAM_SIZE                                   32
//-------------------------------- USB -----------------------------------------------
#define USB_EP_COUNT_MAX                                    4
//low-level USB debug. Turn on only in case of IO problems
#define USB_DEBUG_ERRORS                                    1
//USB test mode support. Not all hardware supported.
#define USB_TEST_MODE                                       0
//support for high speed, qualifier, other speed, test, etc
#define USB_2_0                                             0

//----------------------------- USB device--------------------------------------------
//all other device-related debug depends on this
#define USBD_DEBUG                                          1
#define USBD_DEBUG_ERRORS                                   1
#define USBD_DEBUG_REQUESTS                                 0

#define USBD_PROCESS_SIZE                                   900
#define USBD_BLOCK_SIZE                                     256

#define USBD_CDC_CLASS                                      0
#define USBD_HID_KBD_CLASS                                  1
#define USBD_CCID_CLASS                                     1

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


#endif // SYS_CONFIG_H
