/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "rtl8201.h"

bool rtl8201_power_on(uint8_t phy_addr, ETH_CONN_TYPE conn_type, ETH_SMI_CALLBACKS* cb, void* param)
{
    return false;
}

void rtl8201_power_off(uint8_t phy_addr, ETH_SMI_CALLBACKS* cb, void* param)
{

}

unsigned int rtl8201_get_status(uint8_t phy_addr, ETH_SMI_CALLBACKS* cb, void* param)
{
    return 0;
}

ETH_CONN_TYPE rtl8201_get_conn_status(uint8_t phy_addr, ETH_SMI_CALLBACKS* cb, void* param)
{
    return ETH_NOT_CONNECTED;
}

const ETH_PHY __ETH_PHY_RTL8201 = {
    rtl8201_power_on,
    rtl8201_power_off,
    rtl8201_get_status,
    rtl8201_get_conn_status,
    ETH_PHY_FEATURE_LINK_STATUS | ETH_PHY_FEATURE_AUTO_NEGOTIATION | ETH_PHY_FEATURE_LOOPBACK
};
