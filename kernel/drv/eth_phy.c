/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "eth_phy.h"
#include "../../userspace/systime.h"
#include "../../userspace/process.h"
#include "sys_config.h"
#include "../../userspace/core/core.h"
#include "../kernel.h"

#define ETH_WAIT_QUANT_MS                   10

static void eth_phy_reset(uint8_t phy_addr)
{
    eth_phy_write(phy_addr, ETH_PHY_REG_CONTROL, eth_phy_read(phy_addr, ETH_PHY_REG_CONTROL) | ETH_CONTROL_RESET);
    while (eth_phy_read(phy_addr, ETH_PHY_REG_CONTROL) & ETH_CONTROL_RESET) {}
}

bool eth_phy_power_on(uint8_t phy_addr, ETH_CONN_TYPE conn_type)
{
    uint16_t sta;
    sta = eth_phy_read(phy_addr, ETH_PHY_REG_STATUS);
    //no PHY connection
    if (sta == 0)
        return false;
    eth_phy_reset(phy_addr);
    switch (conn_type)
    {
    case ETH_10_HALF:
        eth_phy_write(phy_addr, ETH_PHY_REG_CONTROL, 0);
        break;
    case ETH_10_FULL:
        eth_phy_write(phy_addr, ETH_PHY_REG_CONTROL, ETH_CONTROL_DUPLEX);
        break;
    case ETH_100_HALF:
        eth_phy_write(phy_addr, ETH_PHY_REG_CONTROL, ETH_CONTROL_SPEED_LSB);
        break;
    case ETH_100_FULL:
        eth_phy_write(phy_addr, ETH_PHY_REG_CONTROL, ETH_CONTROL_SPEED_LSB | ETH_CONTROL_DUPLEX);
        break;
    case ETH_LOOPBACK:
        eth_phy_write(phy_addr, ETH_PHY_REG_CONTROL, ETH_CONTROL_LOOPBACK);
        break;
    default:
        //auto
        eth_phy_write(phy_addr, ETH_PHY_REG_CONTROL, ETH_CONTROL_AUTO_NEGOTIATION_ENABLE);
    }
    return true;
}

void eth_phy_power_off(uint8_t phy_addr)
{
    eth_phy_write(phy_addr, ETH_PHY_REG_CONTROL, eth_phy_read(phy_addr, ETH_PHY_REG_CONTROL) | ETH_CONTROL_POWER_DOWN);
}

ETH_CONN_TYPE eth_phy_get_conn_status(uint8_t phy_addr)
{
    unsigned int i;
    uint16_t sta, con, anlpar;
    sta = eth_phy_read(phy_addr, ETH_PHY_REG_STATUS);

    if ((sta & ETH_STATUS_LINK_STATUS) == 0)
        return ETH_NO_LINK;
    con = eth_phy_read(phy_addr, ETH_PHY_REG_CONTROL);
    if ((con & ETH_CONTROL_AUTO_NEGOTIATION_ENABLE) == 0)
    {
        //on loopback mode always return 100_FULL
        if (con & ETH_CONTROL_LOOPBACK)
            return ETH_100_FULL;
        if (con & ETH_CONTROL_SPEED_LSB)
        {
            return con & ETH_CONTROL_DUPLEX ? ETH_100_FULL : ETH_100_HALF;
        }
        else
        {
            return con & ETH_CONTROL_DUPLEX ? ETH_10_FULL : ETH_10_HALF;
        }
    }
    //wait for auto-negotiation complete
    for (i = 0; ((sta & ETH_STATUS_AUTO_NEGOTIATION_COMPLETE) == 0) && (i < (ETH_AUTO_NEGOTIATION_TIME / ETH_WAIT_QUANT_MS)); ++i)
    {
        sta = eth_phy_read(phy_addr, ETH_PHY_REG_STATUS);
        exodriver_delay_us(ETH_WAIT_QUANT_MS * 1000);
    }
    //auto negotiation failure
    anlpar = eth_phy_read(phy_addr, ETH_PHY_REG_ANLPAR);
    if (((sta & ETH_STATUS_AUTO_NEGOTIATION_COMPLETE) == 0) || (anlpar & ETH_AN_REMOTE_FAULT))
    {
        eth_phy_reset(phy_addr);
        return ETH_REMOTE_FAULT;
    }
    if (anlpar & ETH_AN_100BASE_TX_FULL_DUPLEX)
        return ETH_100_FULL;
    if (anlpar & ETH_AN_100BASE_TX)
        return ETH_100_HALF;
    if (anlpar & ETH_AN_10BASE_T_FULL_DUPLEX)
        return ETH_10_FULL;
    return ETH_10_HALF;
}
