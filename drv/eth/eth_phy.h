/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef ETH_PHY_H
#define ETH_PHY_H

#include <stdbool.h>
#include <stdint.h>
/*
    Ethernet phy interface
 */

//TODO: move to userspace/eth.h
typedef enum {
    ETH_10_HALF = 0,
    ETH_10_FULL,
    ETH_100_HALF,
    ETH_100_FULL,
    ETH_NOT_CONNECTED
} ETH_CONN_TYPE;

//bool eth_smi_write(uint8_t phy_addr, uint8_t reg_addr, uint16_t data, void* param);
//uint16_t eth_smi_read(uint8_t phy_addr, uint8_t reg_addr, void* param);

typedef struct {
    bool (*eth_smi_write)(uint8_t, uint8_t, uint16_t, void*);
    uint16_t (*eth_smi_read)(uint8_t, uint8_t, void*);
} ETH_SMI_CALLBACKS;

//List of PHY features
#define ETH_PHY_FEATURE_LINK_STATUS                     (1 << 0)
#define ETH_PHY_FEATURE_AUTO_NEGOTIATION                (1 << 1)
#define ETH_PHY_FEATURE_LOOPBACK                        (1 << 2)

//list of modes, that can be passed on power_on
#define ETH_PHY_MODE_AUTO_NEGOTIATION                   (1 << 1)
#define ETH_PHY_MODE_LOOPBACK                           (1 << 2)

//List of status bits
#define ETH_PHY_STATUS_LINK_ACTIVE                      (1 << 0)
#define ETH_PHY_STATUS_FAULT                            (1 << 1)

typedef struct {
    bool (*eth_phy_power_on)(uint8_t, ETH_CONN_TYPE, unsigned int, ETH_SMI_CALLBACKS*, void*);
    void (*eth_phy_power_off)(uint8_t, ETH_SMI_CALLBACKS*, void*);
    unsigned int (*eth_phy_get_status)(uint8_t, ETH_SMI_CALLBACKS*, void*);
    ETH_CONN_TYPE (*eth_phy_get_conn_status)(uint8_t, ETH_SMI_CALLBACKS*, void*);
    const unsigned int features;
} ETH_PHY;

#endif // ETH_PHY_H
