/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
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
    IPC_USER = 0x100
} IPCS;

typedef enum {
    //reserved for kernel/system
    HAL_SYSTEM = 0,
    //real hardware
    HAL_PIN,
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
    //device stacks
    HAL_USBD,
    HAL_USBD_IFACE,
    HAL_TCPIP,
    HAL_MAC,
    HAL_ARP,
    HAL_ROUTE,
    HAL_IP,
    HAL_ICMP,
    HAL_PINBOARD,
    //application level
    HAL_APP
} HAL;

#define HAL_CMD(group, item)                                ((group) << 16 | (item))
#define HAL_ITEM(cmd)                                       ((cmd) & 0xffff)
#define HAL_GROUP(cmd)                                      ((cmd) >> 16)

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
    \brief post IPC or error if any
    \param ipc: IPC structure
    \retval none
*/
void ipc_post_or_error(IPC* ipc);

/**
    \brief post IPC
    \details This version must be called for IRQ context
    \param ipc: IPC structure
    \retval none
*/
void ipc_ipost(IPC* ipc);

/**
    \brief read IPC. Ping is processed internally
    \param ipc: ipc
    \retval none
*/
void ipc_read(IPC* ipc);


bool call(IPC* ipc);

/**
    \brief call to process with no return value
    \param cmd: command to post
    \param process: IPC receiver
    \param param1: cmd specific
    \param param2: cmd specific
    \param param3: cmd specific
    \retval true on success
*/

bool ack(HANDLE process, unsigned int cmd, unsigned int param1, unsigned int param2, unsigned int param3);

/**
    \brief get value from process
    \param cmd: command to post
    \param process: IPC receiver
    \param param1: cmd specific
    \param param2: cmd specific
    \param param3: cmd specific
    \retval param1
*/

unsigned int get(HANDLE process, unsigned int cmd, unsigned int param1, unsigned int param2, unsigned int param3);

/** \} */ // end of sem group

#endif // IPC_H
