/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef ETH_PHY_H
#define ETH_PHY_H

#include <stdbool.h>
#include <stdint.h>
#include "../../userspace/eth.h"

/*
    Ethernet phy interface
*/


/******************************************************************************/
/*                                                                            */
/*                            PHY regs                                        */
/*                                                                            */
/******************************************************************************/

#define ETH_PHY_REG_CONTROL                             0
#define ETH_PHY_REG_STATUS                              1
#define ETH_PHY_REG_ID1                                 2
#define ETH_PHY_REG_ID2                                 3
#define ETH_PHY_REG_ANAR                                4
#define ETH_PHY_REG_ANLPAR                              5
#define ETH_PHY_REG_ANER                                6
#define ETH_PHY_REG_ANAR_NEXT                           7
#define ETH_PHY_REG_ANLPAR_NEXT                         8
#define ETH_PHY_REG_MS_CONTROL                          9
#define ETH_PHY_REG_MS_STATUS                           10
#define ETH_PHY_REG_PSE_CONTROL                         11
#define ETH_PHY_REG_PSE_STATUS                          12
#define ETH_PHY_REG_MMD_CONTROL                         13
#define ETH_PHY_REG_MMD_ADDERSS                         14
#define ETH_PHY_REG_EXT_STATUS                          15

/**********  Bit definition for Control register *****************************/

#define ETH_CONTROL_RESET                               (1 << 15)
#define ETH_CONTROL_LOOPBACK                            (1 << 14)
#define ETH_CONTROL_SPEED_LSB                           (1 << 13)
#define ETH_CONTROL_AUTO_NEGOTIATION_ENABLE             (1 << 12)
#define ETH_CONTROL_POWER_DOWN                          (1 << 11)
#define ETH_CONTROL_ISOLATE                             (1 << 10)
#define ETH_CONTROL_AUTO_NEGOTIATION_RESTART            (1 << 9)
#define ETH_CONTROL_DUPLEX                              (1 << 8)
#define ETH_CONTROL_COLLISION_TEST                      (1 << 7)
#define ETH_CONTROL_SPEED_MSB                           (1 << 6)
#define ETH_CONTROL_UNIDIRECTIONAL_ENABLE               (1 << 5)

/**********  Bit definition for Status register ******************************/

#define ETH_STATUS_100BASE_T4                           (1 << 15)
#define ETH_STATUS_100BASE_X_FULL_DUPLEX                (1 << 14)
#define ETH_STATUS_100BASE_X_HALF_DUPLEX                (1 << 13)
#define ETH_STATUS_10BASE_FULL_DUPLEX                   (1 << 12)
#define ETH_STATUS_10BASE_HALF_DUPLEX                   (1 << 11)
#define ETH_STATUS_10BASE_T2_FULL_DUPLEX                (1 << 10)
#define ETH_STATUS_10BASE_T2_HALF_DUPLEX                (1 << 9)
#define ETH_STATUS_EXT_STATUS                           (1 << 8)
#define ETH_STATUS_UNIDIRECTIONAL_ABILITY               (1 << 7)
#define ETH_STATUS_MF_PREAMBLE_SUPPRESSION              (1 << 6)
#define ETH_STATUS_AUTO_NEGOTIATION_COMPLETE            (1 << 5)
#define ETH_STATUS_REMOTE_FAULT                         (1 << 4)
#define ETH_STATUS_AUTO_NEGOTIATION_ABILITY             (1 << 3)
#define ETH_STATUS_LINK_STATUS                          (1 << 2)
#define ETH_STATUS_JABBER_DETECT                        (1 << 1)
#define ETH_STATUS_EXT_CAPABILITY                       (1 << 0)

/**********  Bit definition for ANAR/NEXT registers ******************************/

#define ETH_AN_NEXT_PAGE                                (1 << 15)
#define ETH_AN_REMOTE_FAULT                             (1 << 13)
#define ETH_AN_EXT_NEXT_PAGE                            (1 << 12)
#define ETH_AN_ASSYMETRIC_PAUSE                         (1 << 11)
#define ETH_AN_PAUSE                                    (1 << 10)
#define ETH_AN_100BASE_T4                               (1 << 9)
#define ETH_AN_100BASE_TX_FULL_DUPLEX                   (1 << 8)
#define ETH_AN_100BASE_TX                               (1 << 7)
#define ETH_AN_10BASE_T_FULL_DUPLEX                     (1 << 6)
#define ETH_AN_10BASE_T                                 (1 << 5)

#define ETH_AN_SELECTOR_IEEE_802_3                      (1 << 0)
#define ETH_AN_SELECTOR_IEEE_802_9A                     (2 << 0)
#define ETH_AN_SELECTOR_IEEE_802_5V                     (3 << 0)
#define ETH_AN_SELECTOR_IEEE_1394                       (4 << 0)
#define ETH_AN_SELECTOR_INCITS                          (5 << 0)


//prototypes, that must be defined by driver
extern void eth_phy_write(uint8_t phy_addr, uint8_t reg_addr, uint16_t data);
extern uint16_t eth_phy_read(uint8_t phy_addr, uint8_t reg_addr);

bool eth_phy_power_on(uint8_t phy_addr, ETH_CONN_TYPE conn_type);
void eth_phy_power_off(uint8_t phy_addr);
ETH_CONN_TYPE eth_phy_get_conn_status(uint8_t phy_addr);

#endif // ETH_PHY_H
