/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "lpc_eth.h"
#include "lpc_config.h"
#include "lpc_exo_private.h"
#include "../../userspace/stdio.h"
#include "../../userspace/ipc.h"
#include "../kirq.h"
#include "../kipc.h"
#include "../ksystime.h"
#include "../kernel.h"
#include "../kerror.h"
#include "../../userspace/lpc/lpc_driver.h"
#include "../drv/eth_phy.h"
#include "lpc_pin.h"
#include "lpc_power.h"
#include <stdbool.h>
#include <string.h>

#define ETH_TDES0_OWN                               (1 << 31)
#define ETH_TDES0_IC                                (1 << 30)
#define ETH_TDES0_LS                                (1 << 29)
#define ETH_TDES0_FS                                (1 << 28)
#define ETH_TDES0_DC                                (1 << 27)
#define ETH_TDES0_DP                                (1 << 26)
#define ETH_TDES0_TTSE                              (1 << 25)
#define ETH_TDES0_TER                               (1 << 21)
#define ETH_TDES0_TCH                               (1 << 20)
#define ETH_TDES0_TTSS                              (1 << 17)
#define ETH_TDES0_IHE                               (1 << 16)
#define ETH_TDES0_ES                                (1 << 15)
#define ETH_TDES0_JT                                (1 << 14)
#define ETH_TDES0_FF                                (1 << 13)
#define ETH_TDES0_IPE                               (1 << 12)
#define ETH_TDES0_LCA                               (1 << 11)
#define ETH_TDES0_NC                                (1 << 10)
#define ETH_TDES0_LCO                               (1 << 9)
#define ETH_TDES0_EC                                (1 << 8)
#define ETH_TDES0_VF                                (1 << 7)

#define ETH_TDES0_CC_POS                            3
#define ETH_TDES0_CC_MASK                           (0xf << 3)

#define ETH_TDES0_ED                                (1 << 2)
#define ETH_TDES0_UF                                (1 << 1)
#define ETH_TDES0_DB                                (1 << 0)

#define ETH_TDES1_TBS1_POS                          0
#define ETH_TDES1_TBS1_MASK                         (0x1fff << 0)

#define ETH_TDES1_TBS2_POS                          16
#define ETH_TDES1_TBS2_MASK                         (0x1fff << 16)


#define ETH_RDES0_OWN                               (1 << 31)
#define ETH_RDES0_AFM                               (1 << 30)

#define ETH_RDES0_FL_POS                            16
#define ETH_RDES0_FL_MASK                           (0x3fff << 16)

#define ETH_RDES0_ES                                (1 << 15)
#define ETH_RDES0_DE                                (1 << 14)
#define ETH_RDES0_SAF                               (1 << 13)
#define ETH_RDES0_LE                                (1 << 12)
#define ETH_RDES0_OE                                (1 << 11)
#define ETH_RDES0_VLAN                              (1 << 10)
#define ETH_RDES0_FS                                (1 << 9)
#define ETH_RDES0_LS                                (1 << 8)
#define ETH_RDES0_TSA                               (1 << 7)
#define ETH_RDES0_LCO                               (1 << 6)
#define ETH_RDES0_FT                                (1 << 5)
#define ETH_RDES0_RWT                               (1 << 4)
#define ETH_RDES0_RE                                (1 << 3)
#define ETH_RDES0_DBE                               (1 << 2)
#define ETH_RDES0_CE                                (1 << 1)
#define ETH_RDES0_ESA                               (1 << 0)

#define ETH_RDES1_DIC                               (1 << 31)

#define ETH_RDES1_RBS1_POS                          0
#define ETH_RDES1_RBS1_MASK                         (0x1ffc << 0)

#define ETH_RDES1_RBS2_POS                          16
#define ETH_RDES1_RBS2_MASK                         (0x1ffc << 16)

#define ETH_RDES1_RER                               (1 << 15)
#define ETH_RDES1_RCH                               (1 << 14)

#define ETH_RDES4_PTPVERSION                        (1 << 13)
#define ETH_RDES4_PTPTYPE                           (1 << 12)

#define ETH_RDES4_MTP_POS                           8
#define ETH_RDES4_MTP_MASK                          (0xf << 8)

#define ETH_RDES4_MTP_NO_PTP                        (0x0 << 8)
#define ETH_RDES4_MTP_SYNC                          (0x1 << 8)
#define ETH_RDES4_MTP_FOLLOW_UP                     (0x2 << 8)
#define ETH_RDES4_MTP_DELAY_REQ                     (0x3 << 8)
#define ETH_RDES4_MTP_DELAY_RESP                    (0x4 << 8)
#define ETH_RDES4_MTP_PDELAY_REQ                    (0x5 << 8)
#define ETH_RDES4_MTP_PDELAY_RESP                   (0x6 << 8)
#define ETH_RDES4_MTP_PDELAY_RESP_FOLLOW_UP         (0x7 << 8)
#define ETH_RDES4_MTP_ANNOUNCE                      (0x8 << 8)
#define ETH_RDES4_MTP_MANAGEMENT                    (0x9 << 8)
#define ETH_RDES4_MTP_SIGNALLING                    (0x9 << 8)

#define ETH_RDES4_IPV6                              (1 << 7)
#define ETH_RDES4_IPV4                              (1 << 6)

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

static void lpc_eth_flush(EXO* exo)
{
    IO* io;

    //flush TxFIFO controller
    LPC_ETHERNET->DMA_OP_MODE |= ETHERNET_DMA_OP_MODE_FTF_Msk;
    while(LPC_ETHERNET->DMA_OP_MODE & ETHERNET_DMA_OP_MODE_FTF_Msk) {}
#if (ETH_DOUBLE_BUFFERING)
    int i;
    for (i = 0; i < 2; ++i)
    {
        exo->eth.rx_des[0].size = ETH_RDES1_RCH;
        exo->eth.rx_des[0].buf2_ndes = &exo->eth.rx_des[1];
        exo->eth.rx_des[1].size = ETH_RDES1_RCH;
        exo->eth.rx_des[1].buf2_ndes = &exo->eth.rx_des[0];

        exo->eth.tx_des[0].ctl = ETH_TDES0_TCH | ETH_TDES0_IC;
        exo->eth.tx_des[0].buf2_ndes = &exo->eth.tx_des[1];
        exo->eth.tx_des[1].ctl = ETH_TDES0_TCH | ETH_TDES0_IC;
        exo->eth.tx_des[1].buf2_ndes = &exo->eth.tx_des[0];

        __disable_irq();
        exo->eth.rx_des[i].ctl = 0;
        io = exo->eth.rx[i];
        exo->eth.rx[i] = NULL;
        __enable_irq();
        if (io != NULL)
            kipc_post_exo(exo->eth.tcpip, HAL_IO_CMD(HAL_ETH, IPC_READ), exo->eth.phy_addr, (unsigned int)io, ERROR_IO_CANCELLED);

        __disable_irq();
        exo->eth.tx_des[i].ctl = ETH_TDES0_TCH | ETH_TDES0_IC;;
        io = exo->eth.tx[i];
        exo->eth.tx[i] = NULL;
        __enable_irq();
        if (io != NULL)
            kipc_post_exo(exo->eth.tcpip, HAL_IO_CMD(HAL_ETH, IPC_WRITE), exo->eth.phy_addr, (unsigned int)io, ERROR_IO_CANCELLED);
    }
    exo->eth.cur_rx = (LPC_ETHERNET->DMA_CURHOST_REC_BUF == (unsigned int)(&exo->eth.rx_des[0]) ? 0 : 1);
    exo->eth.cur_tx = (LPC_ETHERNET->DMA_CURHOST_TRANS_BUF == (unsigned int)(&exo->eth.tx_des[0]) ? 0 : 1);
#else
    __disable_irq();
    io = exo->eth.rx;
    exo->eth.rx = NULL;
    __enable_irq();
    if (io != NULL)
        kipc_post_exo(exo->eth.tcpip, HAL_IO_CMD(HAL_ETH, IPC_READ), exo->eth.phy_addr, (unsigned int)io, ERROR_IO_CANCELLED);

    __disable_irq();
    io = exo->eth.tx;
    exo->eth.tx = NULL;
    __enable_irq();
    if (io != NULL)
        kipc_post_exo(exo->eth.tcpip, HAL_IO_CMD(HAL_ETH, IPC_WRITE), exo->eth.phy_addr, (unsigned int)io, ERROR_IO_CANCELLED);
#endif
}

static void lpc_eth_conn_check(EXO* exo)
{
    ETH_CONN_TYPE new_conn;
    new_conn = eth_phy_get_conn_status(exo->eth.phy_addr);
    if (new_conn != exo->eth.conn)
    {
        exo->eth.conn = new_conn;
        exo->eth.connected = ((exo->eth.conn != ETH_NO_LINK) && (exo->eth.conn != ETH_REMOTE_FAULT));
        kipc_post_exo(exo->eth.tcpip, HAL_CMD(HAL_ETH, ETH_NOTIFY_LINK_CHANGED), exo->eth.phy_addr, exo->eth.conn, 0);
        if (exo->eth.connected)
        {
            //set speed and duplex
            switch (exo->eth.conn)
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
#ifdef EXODRIVERS
            exodriver_delay_us(1 * 1000);
#else
            sleep_ms(1);
#endif //EXODRIVERS
            //enable RX/TX, PAD/CRC strip
            LPC_ETHERNET->MAC_CONFIG |= ETHERNET_MAC_CONFIG_RE_Msk | ETHERNET_MAC_CONFIG_TE_Msk | ETHERNET_MAC_CONFIG_ACS_Msk;
            LPC_ETHERNET->DMA_OP_MODE |= ETHERNET_DMA_OP_MODE_SR_Msk | ETHERNET_DMA_OP_MODE_ST_Msk;
        }
        else
        {
            lpc_eth_flush(exo);
            LPC_ETHERNET->DMA_OP_MODE &= ~(ETHERNET_DMA_OP_MODE_SR_Msk | ETHERNET_DMA_OP_MODE_ST_Msk);
            LPC_ETHERNET->MAC_CONFIG &= ~(ETHERNET_MAC_CONFIG_RE_Msk | ETHERNET_MAC_CONFIG_TE_Msk | ETHERNET_MAC_CONFIG_ACS_Msk);
        }
    }
    ksystime_soft_timer_start_ms(exo->eth.timer, 1000);
}

void lpc_eth_isr(int vector, void* param)
{
#if (ETH_DOUBLE_BUFFERING)
    int i;
#endif //ETH_DOUBLE_BUFFERING
    uint32_t sta;
    EXO* exo = (EXO*)param;
    sta = LPC_ETHERNET->DMA_STAT;
    if (sta & ETHERNET_DMA_STAT_RI_Msk)
    {
#if (ETH_DOUBLE_BUFFERING)
        for (i = 0; i < 2; ++i)
        {
            if ((exo->eth.rx[exo->eth.cur_rx] != NULL) && ((exo->eth.rx_des[exo->eth.cur_rx].ctl & ETH_RDES0_OWN) == 0))
            {
                exo->eth.rx[exo->eth.cur_rx]->data_size = (exo->eth.rx_des[exo->eth.cur_rx].ctl & ETH_RDES0_FL_MASK) >> ETH_RDES0_FL_POS;
                iio_complete(exo->eth.tcpip, HAL_IO_CMD(HAL_ETH, IPC_READ), exo->eth.phy_addr, exo->eth.rx[exo->eth.cur_rx]);
                exo->eth.rx[exo->eth.cur_rx] = NULL;
                exo->eth.cur_rx = (exo->eth.cur_rx + 1) & 1;
            }
            else
                break;
        }
#else
        if (exo->eth.rx != NULL)
        {
            exo->eth.rx->data_size = (exo->eth.rx_des.ctl & ETH_RDES0_FL_MASK) >> ETH_RDES0_FL_POS;
            iio_complete(exo->eth.tcpip, HAL_IO_CMD(HAL_ETH, IPC_READ), exo->eth.phy_addr, exo->eth.rx);
            exo->eth.rx = NULL;
        }
#endif
        LPC_ETHERNET->DMA_STAT = ETHERNET_DMA_STAT_RI_Msk;
    }
    if (sta & ETHERNET_DMA_STAT_TI_Msk)
    {
#if (ETH_DOUBLE_BUFFERING)
        for (i = 0; i < 2; ++i)
        {
            if ((exo->eth.tx[exo->eth.cur_tx] != NULL) && ((exo->eth.tx_des[exo->eth.cur_tx].ctl & ETH_TDES0_OWN) == 0))
            {
                iio_complete(exo->eth.tcpip, HAL_IO_CMD(HAL_ETH, IPC_WRITE), exo->eth.phy_addr, exo->eth.tx[exo->eth.cur_tx]);
                exo->eth.tx[exo->eth.cur_tx] = NULL;
                exo->eth.cur_tx = (exo->eth.cur_tx + 1) & 1;
            }
            else
                break;
        }
#else
        if (exo->eth.tx != NULL)
        {
            iio_complete(exo->eth.tcpip, HAL_IO_CMD(HAL_ETH, IPC_WRITE), exo->eth.phy_addr, exo->eth.tx);
            exo->eth.tx = NULL;
        }
#endif
        LPC_ETHERNET->DMA_STAT = ETHERNET_DMA_STAT_TI_Msk;
    }
    LPC_ETHERNET->DMA_STAT = ETHERNET_DMA_STAT_NIS_Msk;
}

static void lpc_eth_close(EXO* exo)
{
    //disable interrupts
    NVIC_DisableIRQ(ETHERNET_IRQn);
    kirq_unregister(KERNEL_HANDLE, ETHERNET_IRQn);

    //flush
    lpc_eth_flush(exo);

    //turn phy off
    eth_phy_power_off(exo->eth.phy_addr);

    //destroy timer
    ksystime_soft_timer_destroy(exo->eth.timer);
    exo->eth.timer = INVALID_HANDLE;

    LPC_CGU->BASE_PHY_TX_CLK = CGU_BASE_PHY_TX_CLK_PD_Msk;
    LPC_CGU->BASE_PHY_RX_CLK = CGU_BASE_PHY_RX_CLK_PD_Msk;

    //switch to unconfigured state
    exo->eth.tcpip = INVALID_HANDLE;
    exo->eth.connected = false;
    exo->eth.conn = ETH_NO_LINK;
}

static inline void lpc_eth_open(EXO* exo, unsigned int phy_addr, ETH_CONN_TYPE conn, HANDLE tcpip)
{
    unsigned int clock;

    exo->eth.timer = ksystime_soft_timer_create(KERNEL_HANDLE, 0, HAL_ETH);
    exo->eth.timeout = false;
    if (exo->eth.timer == INVALID_HANDLE)
        return;
    exo->eth.tcpip = tcpip;
    exo->eth.phy_addr = phy_addr;

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
    memset(exo->eth.tx_des, 0, sizeof(ETH_DESCRIPTOR) * 2);
    memset(exo->eth.rx_des, 0, sizeof(ETH_DESCRIPTOR) * 2);
    exo->eth.rx_des[0].size = ETH_RDES1_RCH;
    exo->eth.rx_des[0].buf2_ndes = &exo->eth.rx_des[1];
    exo->eth.rx_des[1].size = ETH_RDES1_RCH;
    exo->eth.rx_des[1].buf2_ndes = &exo->eth.rx_des[0];

    exo->eth.tx_des[0].ctl = ETH_TDES0_TCH | ETH_TDES0_IC;
    exo->eth.tx_des[0].buf2_ndes = &exo->eth.tx_des[1];
    exo->eth.tx_des[1].ctl = ETH_TDES0_TCH | ETH_TDES0_IC;
    exo->eth.tx_des[1].buf2_ndes = &exo->eth.tx_des[0];

    exo->eth.cur_rx = exo->eth.cur_tx = 0;
#else
    memset(&exo->eth.tx_des, 0, sizeof(ETH_DESCRIPTOR));
    memset(&exo->eth.rx_des, 0, sizeof(ETH_DESCRIPTOR));
    exo->eth.rx_des.size = ETH_RDES1_RCH;
    exo->eth.rx_des.buf2_ndes = &exo->eth.rx_des;
    exo->eth.tx_des.ctl = ETH_TDES0_TCH;
    exo->eth.tx_des.buf2_ndes = &exo->eth.tx_des;
#endif
    LPC_ETHERNET->DMA_TRANS_DES_ADDR = (unsigned int)&exo->eth.tx_des;
    LPC_ETHERNET->DMA_REC_DES_ADDR = (unsigned int)&exo->eth.rx_des;

    //setup MAC
    LPC_ETHERNET->MAC_ADDR0_HIGH = (exo->eth.mac.u8[5] << 8) | (exo->eth.mac.u8[4] << 0) |  (1 << 31);
    LPC_ETHERNET->MAC_ADDR0_LOW = (exo->eth.mac.u8[3] << 24) | (exo->eth.mac.u8[2] << 16) | (exo->eth.mac.u8[1] << 8) | (exo->eth.mac.u8[0] << 0);
    //apply MAC unicast filter
#if (MAC_FILTER)
    LPC_ETHERNET->MAC_FRAME_FILTER = ETHERNET_MAC_FRAME_FILTER_PR_Msk | ETHERNET_MAC_FRAME_FILTER_RA_Msk;
#else
    LPC_ETHERNET->MAC_FRAME_FILTER = 0;
#endif

    //configure SMI
    clock = lpc_power_get_clock_inside(POWER_BUS_CLOCK);
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
    kirq_register(KERNEL_HANDLE, ETHERNET_IRQn, lpc_eth_isr, exo);
    NVIC_EnableIRQ(ETHERNET_IRQn);
    NVIC_SetPriority(ETHERNET_IRQn, 13);
    LPC_ETHERNET->DMA_INT_EN = ETHERNET_DMA_INT_EN_TIE_Msk | ETHERNET_DMA_INT_EN_RIE_Msk | ETHERNET_DMA_INT_EN_NIE_Msk;

    //turn phy on
    if (!eth_phy_power_on(exo->eth.phy_addr, conn))
    {
        kerror(ERROR_NOT_FOUND);
        lpc_eth_close(exo);
        return;
    }

    lpc_eth_conn_check(exo);
}

static inline void lpc_eth_read(EXO* exo, IPC* ipc)
{
    IO* io = (IO*)ipc->param2;
    if (!exo->eth.connected)
    {
        kerror(ERROR_NOT_ACTIVE);
        return;
    }
#if (ETH_DOUBLE_BUFFERING)
    int i = -1;
    uint8_t cur_rx = exo->eth.cur_rx;

    if (exo->eth.rx[cur_rx] == NULL)
        i = cur_rx;
    else if (exo->eth.rx[(cur_rx + 1) & 1] == NULL)
        i = (cur_rx + 1) & 1;
    if (i < 0)
    {
        kerror(ERROR_IN_PROGRESS);
        return;
    }
    exo->eth.rx_des[i].buf1 = io_data(io);
    exo->eth.rx_des[i].size &= ~ETH_RDES1_RBS1_MASK;
    exo->eth.rx_des[i].size |= (((ipc->param3 + 3) << ETH_RDES1_RBS1_POS) & ETH_RDES1_RBS1_MASK);
    __disable_irq();
    exo->eth.rx[i] = io;
    //give descriptor to DMA
    exo->eth.rx_des[i].ctl = ETH_RDES0_OWN;
    __enable_irq();
#else
    if (exo->eth.rx != NULL)
    {
        kerror(ERROR_IN_PROGRESS);
        return;
    }
    exo->eth.rx_des.buf1 = io_data(io);
    exo->eth.rx = io;
    exo->eth.rx_des.size &= ~ETH_RDES1_RBS1_MASK;
    exo->eth.rx_des.size |= (((ipc->param3 + 3) << ETH_RDES1_RBS1_POS) & ETH_RDES1_RBS1_MASK);
    //give descriptor to DMA
    exo->eth.rx_des.ctl = ETH_RDES0_OWN;
#endif
    //enable and poll DMA. Value is doesn't matter
    LPC_ETHERNET->DMA_REC_POLL_DEMAND = 1;
    kerror(ERROR_SYNC);
}

static inline void lpc_eth_write(EXO* exo, IPC* ipc)
{
    IO* io = (IO*)ipc->param2;
    if (!exo->eth.connected)
    {
        kerror(ERROR_NOT_ACTIVE);
        return;
    }
#if (ETH_DOUBLE_BUFFERING)
    int i = -1;
    uint8_t cur_tx = exo->eth.cur_tx;

    if (exo->eth.tx[cur_tx] == NULL)
        i = cur_tx;
    else if (exo->eth.tx[(cur_tx + 1) & 1] == NULL)
        i = (cur_tx + 1) & 1;
    if (i < 0)
    {
        kerror(ERROR_IN_PROGRESS);
        return;
    }
    exo->eth.tx_des[i].buf1 = io_data(io);
    exo->eth.tx_des[i].size = ((io->data_size << ETH_TDES1_TBS1_POS) & ETH_TDES1_TBS1_MASK);
    exo->eth.tx_des[i].ctl = ETH_TDES0_TCH | ETH_TDES0_FS | ETH_TDES0_LS | ETH_TDES0_IC;
    __disable_irq();
    exo->eth.tx[i] = io;
    //give descriptor to DMA
    exo->eth.tx_des[i].ctl |= ETH_TDES0_OWN;
    __enable_irq();
#else
    if (exo->eth.tx != NULL)
    {
        kerror(ERROR_IN_PROGRESS);
        return;
    }
    exo->eth.tx_des.buf1 = io_data(io);
    exo->eth.tx = io;
    exo->eth.tx_des.size = ((io->data_size << ETH_TDES1_TBS1_POS) & ETH_TDES1_TBS1_MASK);
    //give descriptor to DMA
    exo->eth.tx_des.ctl = ETH_TDES0_TCH | ETH_TDES0_FS | ETH_TDES0_LS | ETH_TDES0_IC;
    exo->eth.tx_des.ctl |= ETH_TDES0_OWN;
#endif
    //enable and poll DMA. Value is doesn't matter
    LPC_ETHERNET->DMA_TRANS_POLL_DEMAND = 1;
    kerror(ERROR_SYNC);
}

static inline void lpc_eth_set_mac(EXO* exo, unsigned int param2, unsigned int param3)
{
    exo->eth.mac.u32.hi = param2;
    exo->eth.mac.u32.lo = (uint16_t)param3;
}

static inline void lpc_eth_get_mac(EXO* exo, IPC* ipc)
{
    ipc->param2 = exo->eth.mac.u32.hi;
    ipc->param3 = exo->eth.mac.u32.lo;
}

void lpc_eth_init(EXO* exo)
{
    exo->eth.tcpip = INVALID_HANDLE;
    exo->eth.conn = ETH_NO_LINK;
    exo->eth.connected = false;
    exo->eth.mac.u32.hi = exo->eth.mac.u32.lo = 0;
#if (ETH_DOUBLE_BUFFERING)
    exo->eth.rx[0] = exo->eth.tx[0] = NULL;
    exo->eth.rx[1] = exo->eth.tx[1] = NULL;
#else
    exo->eth.rx = exo->eth.tx = NULL;
#endif
    exo->eth.processing = 0;
}

void lpc_eth_request(EXO* exo, IPC* ipc)
{
    __disable_irq();
    ++exo->eth.processing;
    __enable_irq();
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_OPEN:
        lpc_eth_open(exo, ipc->param1, ipc->param2, ipc->process);
        break;
    case IPC_CLOSE:
        lpc_eth_close(exo);
        break;
    case IPC_FLUSH:
        lpc_eth_flush(exo);
        break;
    case IPC_READ:
        lpc_eth_read(exo, ipc);
        break;
    case IPC_WRITE:
        lpc_eth_write(exo, ipc);
        break;
    case IPC_TIMEOUT:
        if (exo->eth.processing > 1)
            exo->eth.timeout = true;
        else
            lpc_eth_conn_check(exo);
        break;
    case ETH_SET_MAC:
        lpc_eth_set_mac(exo, ipc->param2, ipc->param3);
        break;
    case ETH_GET_MAC:
        lpc_eth_get_mac(exo, ipc);
        break;
    default:
        kerror(ERROR_NOT_SUPPORTED);
        break;
    }
    __disable_irq();
    --exo->eth.processing;
    __enable_irq();
    if (exo->eth.timeout)
    {
        exo->eth.timeout = false;
        lpc_eth_conn_check(exo);
    }
}
