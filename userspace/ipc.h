/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef IPC_H
#define IPC_H

#include "types.h"

typedef struct _SYSTIME SYSTIME;

typedef enum {
    IPC_PING = 0x0,
    IPC_STREAM_WRITE,                                   //!< Sent by kernel when stream write is complete. Param1: handle, Param2: size, Param3: application specific
    IPC_TIMEOUT,                                        //!< Sent by kernel when soft timer is timed out. Param1: handle
    IPC_READ,
    IPC_WRITE,
    IPC_CANCEL_IO,
    IPC_FLUSH,
    IPC_SEEK,
    IPC_OPEN,
    IPC_CLOSE,
    IPC_GET_RX_STREAM,
    IPC_GET_TX_STREAM,
    IPC_SYNC,
    IPC_USER = 0x100
} IPCS;

typedef enum {
    //reserved for kernel/system
    HAL_SYSTEM = 0,
    //real hardware
    HAL_PIN,
    HAL_GPIO,
    HAL_POWER,
    HAL_TIMER,
    HAL_RTC,
    HAL_WDT,
    HAL_UART,
    HAL_USB,
    HAL_ADC,
    HAL_DAC,
    HAL_I2C,
    HAL_LCD,
    HAL_ETH,
    HAL_FLASH,
    HAL_EEPROM,
    HAL_SDMMC,
    HAL_RF,
    //device stacks
    HAL_USBD,
    HAL_USBD_IFACE,
    HAL_TCPIP,
    HAL_MAC,
    HAL_ARP,
    HAL_ROUTE,
    HAL_IP,
    HAL_ICMP,
    HAL_UDP,
    HAL_TCP,
    HAL_HTTP,
    HAL_TLS,
    HAL_PINBOARD,
    //virtual file system
    HAL_VFS,
    //bluetooth controller
    HAL_BTC,
    //bluetooth host
    HAL_BTH,
    //application level
    HAL_APP
} HAL;

//ipc contains IO in param2
#define HAL_IO_FLAG                                         (1 << 15)
//response required
#define HAL_REQ_FLAG                                        (1 << 31)

#define HAL_CMD(group, item)                                ((((group) & 0x7fff) << 16) | ((item) & 0x7fff))
#define HAL_REQ(group, item)                                ((((group) & 0x7fff) << 16) | ((item) & 0x7fff) | HAL_REQ_FLAG)
#define HAL_ITEM(cmd)                                       ((cmd) & 0x7fff)
#define HAL_GROUP(cmd)                                      (((cmd) >> 16) & 0x7fff)

#define HAL_IO_CMD(group, item)                             ((((group) & 0x7fff) << 16) | ((item) & 0x7fff) | HAL_IO_FLAG)
#define HAL_IO_REQ(group, item)                             ((((group) & 0x7fff) << 16) | ((item) & 0x7fff) | HAL_IO_FLAG | HAL_REQ_FLAG)

#define ANY_CMD                                             0xffffffff

typedef struct {
    HANDLE process;
    unsigned int cmd;
    unsigned int param1;
    unsigned int param2;
    unsigned int param3;
} IPC;

/** \addtogroup IPC IPC
    IPC is sending messages from sender process to receiver

    \{
 */

/**
    \brief remove IPCs from queue
    \param process: process or ANY_HANDLE wildcard
    \param cmd: command or ANY_CMD wildcard
    \param param1: extra param, generally object handle or ANY_HANDLE wildcard
    \retval number of removed IPCs
*/
unsigned int ipc_remove(HANDLE process, unsigned int cmd, unsigned int param1);

/**
    \brief post IPC
    \param ipc: IPC structure
    \retval none
*/
void ipc_post(IPC* ipc);

/**
    \brief post IPC, inline version
    \param process: receiver process
    \param cmd: command
    \param param1: cmd-specific param1
    \param param2: cmd-specific param2
    \param param3: cmd-specific param3
    \retval none
*/
void ipc_post_inline(HANDLE process, unsigned int cmd, unsigned int param1, unsigned int param2, unsigned int param3);

/**
    \brief post IPC, inline version for exo driver
    \param cmd: command
    \param param1: cmd-specific param1
    \param param2: cmd-specific param2
    \param param3: cmd-specific param3
    \retval none
*/
#define ipc_post_exo(cmd, param1, param2, param3)                   ipc_post_inline(KERNEL_HANDLE, (cmd), (param1), (param2), (param3))

/**
    \brief post IPC
    \details This version must be called for IRQ context
    \param ipc: IPC structure
    \retval none
*/
void ipc_ipost(IPC* ipc);

/**
    \brief post IPC, ISR inline version
    \param process: receiver process
    \param cmd: command
    \param param1: cmd-specific param1
    \param param2: cmd-specific param2
    \param param3: cmd-specific param3
    \retval none
*/
void ipc_ipost_inline(HANDLE process, unsigned int cmd, unsigned int param1, unsigned int param2, unsigned int param3);

/**
    \brief read IPC. Ping is processed internally
    \param ipc: ipc
    \retval none
*/
void ipc_read(IPC* ipc);

/**
    \brief read message fro process
    \param ipc: ipc to read
    \param process: target process. ANY_HANDLE - ignore
    \param cmd: cmd to waid, ANY_CMD - ignore
    \param param1: param1, generally handle, ANY_HANDLE - ignore
    \retval none
*/
void ipc_read_ex(IPC* ipc, HANDLE process, unsigned int cmd, unsigned int param1);

/**
    \brief write IPC or error if any
    \param ipc: IPC structure
    \retval none
*/
void ipc_write(IPC* ipc);

/**
    \brief call to process, wait for response
    \param IPC: ipc to call
    \retval none
*/

void call(IPC* ipc);

/**
    \brief call to process, inline version
    \param cmd: command to post
    \param process: IPC receiver
    \param param1: cmd specific
    \param param2: cmd specific
    \param param3: cmd specific
    \retval none
*/

void ack(HANDLE process, unsigned int cmd, unsigned int param1, unsigned int param2, unsigned int param3);

/**
    \brief get value from process.
    \details Error set if param3 is negative
    \param cmd: command to post
    \param process: IPC receiver
    \param param1: cmd specific
    \param param2: cmd specific
    \param param3: cmd specific
    \retval returned param2
*/

unsigned int get(HANDLE process, unsigned int cmd, unsigned int param1, unsigned int param2, unsigned int param3);

/**
    \brief get value from driver
    \details Error set if param3 is negative
    \param cmd: command to post
    \param param1: cmd specific
    \param param2: cmd specific
    \param param3: cmd specific
    \retval returned param2
*/

#define get_exo(cmd, param1, param2, param3)                        get(KERNEL_HANDLE, (cmd), (param1), (param2), param3)

/**
    \brief get hangle value from process
    \details INVALID_HANDLE on error
    \param cmd: command to post
    \param process: IPC receiver
    \param param1: cmd specific
    \param param2: cmd specific
    \param param3: cmd specific
    \retval param1
*/

unsigned int get_handle(HANDLE process, unsigned int cmd, unsigned int param1, unsigned int param2, unsigned int param3);

/**
    \brief get hangle value from driver
    \details INVALID_HANDLE on error
    \param cmd: command to post
    \param param1: cmd specific
    \param param2: cmd specific
    \param param3: cmd specific
    \retval param1
*/

#define get_handle_exo(cmd, param1, param2, param3)                 get_handle(KERNEL_HANDLE, (cmd), (param1), (param2), param3)

/**
    \brief get size (positive) value from process.
    \details Error set if param3 is negative
    \param cmd: command to post
    \param process: IPC receiver
    \param param1: cmd specific
    \param param2: cmd specific
    \param param3: cmd specific
    \retval param3
*/

int get_size(HANDLE process, unsigned int cmd, unsigned int param1, unsigned int param2, unsigned int param3);

/**
    \brief get size (positive) value from driver
    \details Error set if param3 is negative
    \param cmd: command to post
    \param param1: cmd specific
    \param param2: cmd specific
    \param param3: cmd specific
    \retval param3
*/

#define get_size_exo(cmd, param1, param2, param3)                   get_size(KERNEL_HANDLE, (cmd), (param1), (param2), param3)

/** \} */ // end of sem group

#endif // IPC_H
