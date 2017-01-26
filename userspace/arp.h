/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef ARP_H
#define ARP_H

#include "ipc.h"
#include "ip.h"
#include "mac.h"
#include "types.h"

typedef enum {
    ARP_ADD_STATIC = IPC_USER,
    ARP_REMOVE,
    ARP_SHOW_TABLE
}ARP_IPCS;

/** \addtogroup arp arp
    TCPIP ARP routines
    \{
 */

/**
    \brief add ARP static route
    \param tcpip: tcpip stack handle
    \param ip: pointer to IP address
    \param mac: pointer to MAC address
*/
bool arp_add_static(HANDLE tcpip, const IP* ip, const MAC* mac);

/**
    \brief remove ARP route
    \param tcpip: tcpip stack handle
    \param ip: pointer to IP address
*/
bool arp_remove(HANDLE tcpip, const IP* ip);

/**
    \brief flush ARP cache
    \param tcpip: tcpip stack handle
*/
void arp_flush(HANDLE tcpip);

/**
    \brief show current ARP table
    \details ARP_DEBUG must be enabled
    \param tcpip: tcpip stack handle
*/
void arp_show_table(HANDLE tcpip);

/** \} */ // end of arp group

#endif // ARP_H
