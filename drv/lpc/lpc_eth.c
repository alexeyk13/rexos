/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2016, Alexey Kramarenko
    All rights reserved.
*/

#include "lpc_eth.h"
#include "lpc_config.h"
#include "../../userspace/stdio.h"
#include "../../userspace/ipc.h"
#include "../../userspace/irq.h"
#include "../../userspace/sys.h"
#include "../../userspace/systime.h"
#include "../../userspace/power.h"
#include "../../userspace/lpc/lpc_driver.h"
#include "../eth_phy.h"
#include "lpc_pin.h"
#include "lpc_power.h"
#include <stdbool.h>
#include <string.h>

void lpc_eth();

const REX __LPC_ETH = {
    //name
    "LPC ETH",
    //size
    LPC_ETH_PROCESS_SIZE,
    //priority - driver priority
    91,
    //flags
    PROCESS_FLAGS_ACTIVE | REX_FLAG_PERSISTENT_NAME,
    //function
    lpc_eth
};

void eth_phy_write(uint8_t phy_addr, uint8_t reg_addr, uint16_t data)
{
    while (LPC_ETHERNET->MAC_MII_ADDR & ETHERNET_MAC_MII_ADDR_GB_Msk) {}
    LPC_ETHERNET->MAC_MII_ADDR &= ~(ETHERNET_MAC_MII_ADDR_GR_Msk | ETHERNET_MAC_MII_ADDR_PA_Msk);
    LPC_ETHERNET->MAC_MII_DATA = data;
    LPC_ETHERNET->MAC_MII_ADDR |= ETHERNET_MAC_MII_ADDR_W_Msk | ((reg_addr & 0x1f) << ETHERNET_MAC_MII_ADDR_GR_Pos) |
                                  ((phy_addr & 0x1f) << ETHERNET_MAC_MII_ADDR_PA_Pos);
    LPC_ETHERNET->MAC_MII_ADDR |= ETHERNET_MAC_MII_ADDR_GB_Msk;
    while (LPC_ETHERNET->MAC_MII_ADDR & ETHERNET_MAC_MII_ADDR_GB_Msk) {}
}

uint16_t eth_phy_read(uint8_t phy_addr, uint8_t reg_addr)
{
    while (LPC_ETHERNET->MAC_MII_ADDR & ETHERNET_MAC_MII_ADDR_GB_Msk) {}
    LPC_ETHERNET->MAC_MII_ADDR &= ~(ETHERNET_MAC_MII_ADDR_W_Msk | ETHERNET_MAC_MII_ADDR_GR_Msk | ETHERNET_MAC_MII_ADDR_PA_Msk);
    LPC_ETHERNET->MAC_MII_ADDR |= ((reg_addr & 0x1f) << ETHERNET_MAC_MII_ADDR_GR_Pos) | ((phy_addr & 0x1f) << ETHERNET_MAC_MII_ADDR_PA_Pos);
    LPC_ETHERNET->MAC_MII_ADDR |= ETHERNET_MAC_MII_ADDR_GB_Msk;
    while (LPC_ETHERNET->MAC_MII_ADDR & ETHERNET_MAC_MII_ADDR_GB_Msk) {}
    return LPC_ETHERNET->MAC_MII_DATA & 0xffff;
}

static void lpc_eth_flush(ETH_DRV* drv)
{
    IO* io;

    //flush TxFIFO controller
    LPC_ETHERNET->DMA_OP_MODE |= ETHERNET_DMA_OP_MODE_FTF_Msk;
    while(LPC_ETHERNET->DMA_OP_MODE & ETHERNET_DMA_OP_MODE_FTF_Msk) {}
#if (ETH_DOUBLE_BUFFERING)
    int i;
    for (i = 0; i < 2; ++i)
    {
        drv->rx_des[0].size = ETH_RDES1_RCH;
        drv->rx_des[0].buf2_ndes = &drv->rx_des[1];
        drv->rx_des[1].size = ETH_RDES1_RCH;
        drv->rx_des[1].buf2_ndes = &drv->rx_des[0];

        drv->tx_des[0].ctl = ETH_TDES0_TCH | ETH_TDES0_IC;
        drv->tx_des[0].buf2_ndes = &drv->tx_des[1];
        drv->tx_des[1].ctl = ETH_TDES0_TCH | ETH_TDES0_IC;
        drv->tx_des[1].buf2_ndes = &drv->tx_des[0];

        __disable_irq();
        drv->rx_des[i].ctl = 0;
        io = drv->rx[i];
        drv->rx[i] = NULL;
        __enable_irq();
        if (io != NULL)
            io_complete_ex(drv->tcpip, HAL_IO_CMD(HAL_ETH, IPC_READ), drv->phy_addr, io, ERROR_IO_CANCELLED);

        __disable_irq();
        drv->tx_des[i].ctl = ETH_TDES0_TCH | ETH_TDES0_IC;;
        io = drv->tx[i];
        drv->tx[i] = NULL;
        __enable_irq();
        if (io != NULL)
            io_complete_ex(drv->tcpip, HAL_IO_CMD(HAL_ETH, IPC_WRITE), drv->phy_addr, io, ERROR_IO_CANCELLED);
    }
    drv->cur_rx = (LPC_ETHERNET->DMA_CURHOST_REC_BUF == (unsigned int)(&drv->rx_des[0]) ? 0 : 1);
    drv->cur_tx = (LPC_ETHERNET->DMA_CURHOST_TRANS_BUF == (unsigned int)(&drv->tx_des[0]) ? 0 : 1);
#else
    __disable_irq();
    io = drv->rx;
    drv->rx = NULL;
    __enable_irq();
    if (io != NULL)
        io_complete_ex(drv->tcpip, HAL_IO_CMD(HAL_ETH, IPC_READ), drv->phy_addr, io, ERROR_IO_CANCELLED);

    __disable_irq();
    io = drv->tx;
    drv->tx = NULL;
    __enable_irq();
    if (io != NULL)
        io_complete_ex(drv->tcpip, HAL_IO_CMD(HAL_ETH, IPC_WRITE), drv->phy_addr, io, ERROR_IO_CANCELLED);
#endif
}

static void lpc_eth_conn_check(ETH_DRV* drv)
{
    ETH_CONN_TYPE new_conn;
    new_conn = eth_phy_get_conn_status(drv->phy_addr);
    if (new_conn != drv->conn)
    {
        drv->conn = new_conn;
        drv->connected = ((drv->conn != ETH_NO_LINK) && (drv->conn != ETH_REMOTE_FAULT));
        ipc_post_inline(drv->tcpip, HAL_CMD(HAL_ETH, ETH_NOTIFY_LINK_CHANGED), drv->phy_addr, drv->conn, 0);
        if (drv->connected)
        {
            //set speed and duplex
            switch (drv->conn)
            {
            case ETH_10_HALF:
                LPC_ETHERNET->MAC_CONFIG &= ~(ETHERNET_MAC_CONFIG_DM_Msk | ETHERNET_MAC_CONFIG_FES_Msk);
                break;
            case ETH_10_FULL:
                LPC_ETHERNET->MAC_CONFIG &= ~ETHERNET_MAC_CONFIG_FES_Msk;
                LPC_ETHERNET->MAC_CONFIG |= ETHERNET_MAC_CONFIG_DM_Msk;
                break;
            case ETH_100_HALF:
                LPC_ETHERNET->MAC_CONFIG &= ~ETHERNET_MAC_CONFIG_DM_Msk;
                LPC_ETHERNET->MAC_CONFIG |= ETHERNET_MAC_CONFIG_FES_Msk;
                break;
            case ETH_100_FULL:
                LPC_ETHERNET->MAC_CONFIG |= ETHERNET_MAC_CONFIG_DM_Msk | ETHERNET_MAC_CONFIG_FES_Msk;
                break;
            default:
                break;
            }
            //Looks like some timeout is required after connection mode is changed to setup state machin
            //in other case there is chance it will hang
            sleep_ms(1);
            //enable RX/TX, PAD/CRC strip
            LPC_ETHERNET->MAC_CONFIG |= ETHERNET_MAC_CONFIG_RE_Msk | ETHERNET_MAC_CONFIG_TE_Msk | ETHERNET_MAC_CONFIG_ACS_Msk;
            LPC_ETHERNET->DMA_OP_MODE |= ETHERNET_DMA_OP_MODE_SR_Msk | ETHERNET_DMA_OP_MODE_ST_Msk;
        }
        else
        {
            lpc_eth_flush(drv);
            LPC_ETHERNET->DMA_OP_MODE &= ~(ETHERNET_DMA_OP_MODE_SR_Msk | ETHERNET_DMA_OP_MODE_ST_Msk);
            LPC_ETHERNET->MAC_CONFIG &= ~(ETHERNET_MAC_CONFIG_RE_Msk | ETHERNET_MAC_CONFIG_TE_Msk | ETHERNET_MAC_CONFIG_ACS_Msk);
        }
    }
    timer_start_ms(drv->timer, 1000);
}

void lpc_eth_isr(int vector, void* param)
{
#if (ETH_DOUBLE_BUFFERING)
    int i;
#endif //ETH_DOUBLE_BUFFERING
    uint32_t sta;
    ETH_DRV* drv = (ETH_DRV*)param;
    sta = LPC_ETHERNET->DMA_STAT;
    if (sta & ETHERNET_DMA_STAT_RI_Msk)
    {
#if (ETH_DOUBLE_BUFFERING)
        for (i = 0; i < 2; ++i)
        {
            if ((drv->rx[drv->cur_rx] != NULL) && ((drv->rx_des[drv->cur_rx].ctl & ETH_RDES0_OWN) == 0))
            {
                drv->rx[drv->cur_rx]->data_size = (drv->rx_des[drv->cur_rx].ctl & ETH_RDES0_FL_MASK) >> ETH_RDES0_FL_POS;
                iio_complete(drv->tcpip, HAL_IO_CMD(HAL_ETH, IPC_READ), drv->phy_addr, drv->rx[drv->cur_rx]);
                drv->rx[drv->cur_rx] = NULL;
                drv->cur_rx = (drv->cur_rx + 1) & 1;
            }
            else
                break;
        }
#else
        if (drv->rx != NULL)
        {
            drv->rx->data_size = (drv->rx_des.ctl & ETH_RDES0_FL_MASK) >> ETH_RDES0_FL_POS;
            iio_complete(drv->tcpip, HAL_IO_CMD(HAL_ETH, IPC_READ), drv->phy_addr, drv->rx);
            drv->rx = NULL;
        }
#endif
        LPC_ETHERNET->DMA_STAT = ETHERNET_DMA_STAT_RI_Msk;
    }
    if (sta & ETHERNET_DMA_STAT_TI_Msk)
    {
#if (ETH_DOUBLE_BUFFERING)
        for (i = 0; i < 2; ++i)
        {
            if ((drv->tx[drv->cur_tx] != NULL) && ((drv->tx_des[drv->cur_tx].ctl & ETH_TDES0_OWN) == 0))
            {
                iio_complete(drv->tcpip, HAL_IO_CMD(HAL_ETH, IPC_WRITE), drv->phy_addr, drv->tx[drv->cur_tx]);
                drv->tx[drv->cur_tx] = NULL;
                drv->cur_tx = (drv->cur_tx + 1) & 1;
            }
            else
                break;
        }
#else
        if (drv->tx != NULL)
        {
            iio_complete(drv->tcpip, HAL_IO_CMD(HAL_ETH, IPC_WRITE), drv->phy_addr, drv->tx);
            drv->tx = NULL;
        }
#endif
        LPC_ETHERNET->DMA_STAT = ETHERNET_DMA_STAT_TI_Msk;
    }
    LPC_ETHERNET->DMA_STAT = ETHERNET_DMA_STAT_NIS_Msk;
}

static void lpc_eth_close(ETH_DRV* drv)
{
    //disable interrupts
    NVIC_DisableIRQ(ETHERNET_IRQn);
    irq_unregister(ETHERNET_IRQn);

    //flush
    lpc_eth_flush(drv);

    //turn phy off
    eth_phy_power_off(drv->phy_addr);

    //destroy timer
    timer_destroy(drv->timer);
    drv->timer = INVALID_HANDLE;

    LPC_CGU->BASE_PHY_TX_CLK = CGU_BASE_PHY_TX_CLK_PD_Msk;
    LPC_CGU->BASE_PHY_RX_CLK = CGU_BASE_PHY_RX_CLK_PD_Msk;

    //switch to unconfigured state
    drv->tcpip = INVALID_HANDLE;
    drv->connected = false;
    drv->conn = ETH_NO_LINK;
}

static inline void lpc_eth_open(ETH_DRV* drv, unsigned int phy_addr, ETH_CONN_TYPE conn, HANDLE tcpip)
{
    unsigned int clock;

    drv->timer = timer_create(0, HAL_ETH);
    if (drv->timer == INVALID_HANDLE)
        return;
    drv->tcpip = tcpip;
    drv->phy_addr = phy_addr;

    //setup PHY interface type and reset ETHERNET
    LPC_CREG->CREG6 &= ~CREG_CREG6_ETHMODE_Msk;
    LPC_CGU->BASE_PHY_TX_CLK = CGU_BASE_PHY_TX_CLK_PD_Msk;
    LPC_CGU->BASE_PHY_TX_CLK |= CGU_CLK_ENET_TX;
    LPC_CGU->BASE_PHY_TX_CLK &= ~CGU_BASE_PHY_TX_CLK_PD_Msk;

#if (LPC_ETH_MII)
    LPC_CGU->BASE_PHY_RX_CLK = CGU_BASE_PHY_RX_CLK_PD_Msk;
    LPC_CGU->BASE_PHY_RX_CLK |= CGU_CLK_ENET_RX;
    LPC_CGU->BASE_PHY_RX_CLK &= ~CGU_BASE_PHY_RX_CLK_PD_Msk;
#else
    LPC_CGU->BASE_PHY_RX_CLK = CGU_BASE_PHY_RX_CLK_PD_Msk;
    LPC_CGU->BASE_PHY_RX_CLK |= CGU_CLK_ENET_TX;
    LPC_CGU->BASE_PHY_RX_CLK &= ~CGU_BASE_PHY_RX_CLK_PD_Msk;
    LPC_CREG->CREG6 |= CREG_CREG6_ETHMODE_RMII;
#endif //LPC_ETH_MII
    LPC_RGU->RESET_CTRL0 = RGU_RESET_CTRL0_ETHERNET_RST_Msk;
    while ((LPC_RGU->RESET_ACTIVE_STATUS0 & RGU_RESET_ACTIVE_STATUS0_ETHERNET_RST_Msk) == 0) {}

    //reset DMA
    LPC_ETHERNET->DMA_BUS_MODE |= ETHERNET_DMA_BUS_MODE_SWR_Msk;
    while(LPC_ETHERNET->DMA_BUS_MODE & ETHERNET_DMA_BUS_MODE_SWR_Msk) {}

    //setup descriptors
#if (ETH_DOUBLE_BUFFERING)
    memset(drv->tx_des, 0, sizeof(ETH_DESCRIPTOR) * 2);
    memset(drv->rx_des, 0, sizeof(ETH_DESCRIPTOR) * 2);
    drv->rx_des[0].size = ETH_RDES1_RCH;
    drv->rx_des[0].buf2_ndes = &drv->rx_des[1];
    drv->rx_des[1].size = ETH_RDES1_RCH;
    drv->rx_des[1].buf2_ndes = &drv->rx_des[0];

    drv->tx_des[0].ctl = ETH_TDES0_TCH | ETH_TDES0_IC;
    drv->tx_des[0].buf2_ndes = &drv->tx_des[1];
    drv->tx_des[1].ctl = ETH_TDES0_TCH | ETH_TDES0_IC;
    drv->tx_des[1].buf2_ndes = &drv->tx_des[0];

    drv->cur_rx = drv->cur_tx = 0;
#else
    memset(&drv->tx_des, 0, sizeof(ETH_DESCRIPTOR));
    memset(&drv->rx_des, 0, sizeof(ETH_DESCRIPTOR));
    drv->rx_des.size = ETH_RDES1_RCH;
    drv->rx_des.buf2_ndes = &drv->rx_des;
    drv->tx_des.ctl = ETH_TDES0_TCH;
    drv->tx_des.buf2_ndes = &drv->tx_des;
#endif
    LPC_ETHERNET->DMA_TRANS_DES_ADDR = (unsigned int)&drv->tx_des;
    LPC_ETHERNET->DMA_REC_DES_ADDR = (unsigned int)&drv->rx_des;

    //setup MAC
    LPC_ETHERNET->MAC_ADDR0_HIGH = (drv->mac.u8[5] << 8) | (drv->mac.u8[4] << 0) |  (1 << 31);
    LPC_ETHERNET->MAC_ADDR0_LOW = (drv->mac.u8[3] << 24) | (drv->mac.u8[2] << 16) | (drv->mac.u8[1] << 8) | (drv->mac.u8[0] << 0);
    //apply MAC unicast filter
#if (MAC_FILTER)
    LPC_ETHERNET->MAC_FRAME_FILTER = ETHERNET_MAC_FRAME_FILTER_PR_Msk | ETHERNET_MAC_FRAME_FILTER_RA_Msk;
#else
    LPC_ETHERNET->MAC_FRAME_FILTER = 0;
#endif

    //configure SMI
    clock = power_get_clock(POWER_BUS_CLOCK);
    if (clock > 250000000)
        LPC_ETHERNET->MAC_MII_ADDR |= 5 << ETHERNET_MAC_MII_ADDR_CR_Pos;
    else if (clock > 150000000)
        LPC_ETHERNET->MAC_MII_ADDR |= 4 << ETHERNET_MAC_MII_ADDR_CR_Pos;
    else if (clock > 100000000)
        LPC_ETHERNET->MAC_MII_ADDR |= 1 << ETHERNET_MAC_MII_ADDR_CR_Pos;
    else if (clock > 60000000)
        LPC_ETHERNET->MAC_MII_ADDR |= 0 << ETHERNET_MAC_MII_ADDR_CR_Pos;
    else if (clock > 35000000)
        LPC_ETHERNET->MAC_MII_ADDR |= 3 << ETHERNET_MAC_MII_ADDR_CR_Pos;
    else
        LPC_ETHERNET->MAC_MII_ADDR |= 2 << ETHERNET_MAC_MII_ADDR_CR_Pos;

    //enable dma interrupts
    irq_register(ETHERNET_IRQn, lpc_eth_isr, (void*)drv);
    NVIC_EnableIRQ(ETHERNET_IRQn);
    NVIC_SetPriority(ETHERNET_IRQn, 13);
    LPC_ETHERNET->DMA_INT_EN = ETHERNET_DMA_INT_EN_TIE_Msk | ETHERNET_DMA_INT_EN_RIE_Msk | ETHERNET_DMA_INT_EN_NIE_Msk;

    //turn phy on
    if (!eth_phy_power_on(drv->phy_addr, conn))
    {
        error(ERROR_NOT_FOUND);
        lpc_eth_close(drv);
        return;
    }

    lpc_eth_conn_check(drv);
}

static inline void lpc_eth_read(ETH_DRV* drv, IPC* ipc)
{
    IO* io = (IO*)ipc->param2;
    if (!drv->connected)
    {
        error(ERROR_NOT_ACTIVE);
        return;
    }
#if (ETH_DOUBLE_BUFFERING)
    int i = -1;
    uint8_t cur_rx = drv->cur_rx;

    if (drv->rx[cur_rx] == NULL)
        i = cur_rx;
    else if (drv->rx[(cur_rx + 1) & 1] == NULL)
        i = (cur_rx + 1) & 1;
    if (i < 0)
    {
        error(ERROR_IN_PROGRESS);
        return;
    }
    drv->rx_des[i].buf1 = io_data(io);
    drv->rx_des[i].size &= ~ETH_RDES1_RBS1_MASK;
    drv->rx_des[i].size |= (((ipc->param3 + 3) << ETH_RDES1_RBS1_POS) & ETH_RDES1_RBS1_MASK);
    __disable_irq();
    drv->rx[i] = io;
    //give descriptor to DMA
    drv->rx_des[i].ctl = ETH_RDES0_OWN;
    __enable_irq();
#else
    if (drv->rx != NULL)
    {
        error(ERROR_IN_PROGRESS);
        return;
    }
    drv->rx_des.buf1 = io_data(io);
    drv->rx = io;
    drv->rx_des.size &= ~ETH_RDES1_RBS1_MASK;
    drv->rx_des.size |= (((ipc->param3 + 3) << ETH_RDES1_RBS1_POS) & ETH_RDES1_RBS1_MASK);
    //give descriptor to DMA
    drv->rx_des.ctl = ETH_RDES0_OWN;
#endif
    //enable and poll DMA. Value is doesn't matter
    LPC_ETHERNET->DMA_REC_POLL_DEMAND = 1;
    error(ERROR_SYNC);
}

static inline void lpc_eth_write(ETH_DRV* drv, IPC* ipc)
{
    IO* io = (IO*)ipc->param2;
    if (!drv->connected)
    {
        error(ERROR_NOT_ACTIVE);
        return;
    }
#if (ETH_DOUBLE_BUFFERING)
    int i = -1;
    uint8_t cur_tx = drv->cur_tx;

    if (drv->tx[cur_tx] == NULL)
        i = cur_tx;
    else if (drv->tx[(cur_tx + 1) & 1] == NULL)
        i = (cur_tx + 1) & 1;
    if (i < 0)
    {
        error(ERROR_IN_PROGRESS);
        return;
    }
    drv->tx_des[i].buf1 = io_data(io);
    drv->tx_des[i].size = ((io->data_size << ETH_TDES1_TBS1_POS) & ETH_TDES1_TBS1_MASK);
    drv->tx_des[i].ctl = ETH_TDES0_TCH | ETH_TDES0_FS | ETH_TDES0_LS | ETH_TDES0_IC;
    __disable_irq();
    drv->tx[i] = io;
    //give descriptor to DMA
    drv->tx_des[i].ctl |= ETH_TDES0_OWN;
    __enable_irq();
#else
    if (drv->tx != NULL)
    {
        error(ERROR_IN_PROGRESS);
        return;
    }
    drv->tx_des.buf1 = io_data(io);
    drv->tx = io;
    drv->tx_des.size = ((io->data_size << ETH_TDES1_TBS1_POS) & ETH_TDES1_TBS1_MASK);
    //give descriptor to DMA
    drv->tx_des.ctl = ETH_TDES0_TCH | ETH_TDES0_FS | ETH_TDES0_LS | ETH_TDES0_IC;
    drv->tx_des.ctl |= ETH_TDES0_OWN;
#endif
    //enable and poll DMA. Value is doesn't matter
    LPC_ETHERNET->DMA_TRANS_POLL_DEMAND = 1;
    error(ERROR_SYNC);
}

static inline void lpc_eth_set_mac(ETH_DRV* drv, unsigned int param2, unsigned int param3)
{
    drv->mac.u32.hi = param2;
    drv->mac.u32.lo = (uint16_t)param3;
}

static inline void lpc_eth_get_mac(ETH_DRV* drv, IPC* ipc)
{
    ipc->param2 = drv->mac.u32.hi;
    ipc->param3 = drv->mac.u32.lo;
}

void lpc_eth_init(ETH_DRV* drv)
{
    drv->tcpip = INVALID_HANDLE;
    drv->conn = ETH_NO_LINK;
    drv->connected = false;
    drv->mac.u32.hi = drv->mac.u32.lo = 0;
#if (ETH_DOUBLE_BUFFERING)
    drv->rx[0] = drv->tx[0] = NULL;
    drv->rx[1] = drv->tx[1] = NULL;
#else
    drv->rx = drv->tx = NULL;
#endif
}

void lpc_eth_request(ETH_DRV* drv, IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_OPEN:
        lpc_eth_open(drv, ipc->param1, ipc->param2, ipc->process);
        break;
    case IPC_CLOSE:
        lpc_eth_close(drv);
        break;
    case IPC_FLUSH:
        lpc_eth_flush(drv);
        break;
    case IPC_READ:
        lpc_eth_read(drv, ipc);
        break;
    case IPC_WRITE:
        lpc_eth_write(drv, ipc);
        break;
    case IPC_TIMEOUT:
        lpc_eth_conn_check(drv);
        break;
    case ETH_SET_MAC:
        lpc_eth_set_mac(drv, ipc->param2, ipc->param3);
        break;
    case ETH_GET_MAC:
        lpc_eth_get_mac(drv, ipc);
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
        break;
    }
}

void lpc_eth()
{
    IPC ipc;
    ETH_DRV drv;
    lpc_eth_init(&drv);
    object_set_self(SYS_OBJ_ETH);
    for (;;)
    {
        ipc_read(&ipc);
        lpc_eth_request(&drv, &ipc);
        ipc_write(&ipc);
    }
}
