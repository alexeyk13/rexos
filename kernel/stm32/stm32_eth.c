/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "stm32_eth.h"
#include "stm32_config.h"
#include "../../userspace/stdio.h"
#include "../kipc.h"
#include "../kirq.h"
#include "../kerror.h"
#include "../ksystime.h"
#include "../../userspace/stm32/stm32_driver.h"
#include "../drv/eth_phy.h"
#include "stm32_pin.h"
#include "stm32_power.h"
#include <stdbool.h>
#include <string.h>
#include "stm32_exo_private.h"

void eth_phy_write(uint8_t phy_addr, uint8_t reg_addr, uint16_t data)
{
    while (ETH->MACMIIAR & ETH_MACMIIAR_MB) {}
    ETH->MACMIIAR &= ~(ETH_MACMIIAR_PA | ETH_MACMIIAR_MR);
    ETH->MACMIIAR |= ((phy_addr & 0x1f) << 11) | ((reg_addr & 0x1f) << 6) | ETH_MACMIIAR_MW;
    ETH->MACMIIDR = data & ETH_MACMIIDR_MD;
    ETH->MACMIIAR |= ETH_MACMIIAR_MB;
    while (ETH->MACMIIAR & ETH_MACMIIAR_MB) {}
}

uint16_t eth_phy_read(uint8_t phy_addr, uint8_t reg_addr)
{
    while (ETH->MACMIIAR & ETH_MACMIIAR_MB) {}
    ETH->MACMIIAR &= ~(ETH_MACMIIAR_PA | ETH_MACMIIAR_MR | ETH_MACMIIAR_MW);
    ETH->MACMIIAR |= ((phy_addr & 0x1f) << 11) | ((reg_addr & 0x1f) << 6);
    ETH->MACMIIAR |= ETH_MACMIIAR_MB;
    while (ETH->MACMIIAR & ETH_MACMIIAR_MB) {}
    return ETH->MACMIIDR & ETH_MACMIIDR_MD;
}

static void stm32_eth_flush(EXO* exo)
{
    IO* io;

    //flush TxFIFO controller
    ETH->DMAOMR |= ETH_DMAOMR_FTF;
    while(ETH->DMAOMR & ETH_DMAOMR_FTF) {}
#if (ETH_DOUBLE_BUFFERING)
    int i;
    for (i = 0; i < 2; ++i)
    {
        __disable_irq();
        exo->eth.rx_des[i].ctl = 0;
        io = exo->eth.rx[i];
        exo->eth.rx[i] = NULL;
        __enable_irq();
        if (io != NULL)
            io_complete_ex_exo(exo->eth.tcpip, HAL_IO_CMD(HAL_ETH, IPC_READ), exo->eth.phy_addr, io, ERROR_IO_CANCELLED);

        __disable_irq();
        exo->eth.tx_des[i].ctl = 0;
        io = exo->eth.tx[i];
        exo->eth.tx[i] = NULL;
        __enable_irq();
        if (io != NULL)
            io_complete_ex_exo(exo->eth.tcpip, HAL_IO_CMD(HAL_ETH, IPC_WRITE), exo->eth.phy_addr, io, ERROR_IO_CANCELLED);
    }
    exo->eth.cur_rx = (ETH->DMACHRDR == (unsigned int)(&exo->eth.rx_des[0]) ? 0 : 1);
    exo->eth.cur_tx = (ETH->DMACHTDR == (unsigned int)(&exo->eth.tx_des[0]) ? 0 : 1);
#else
    __disable_irq();
    io = exo->eth.rx;
    exo->eth.rx = NULL;
    __enable_irq();
    if (io != NULL)
        io_complete_ex_exo(exo->eth.tcpip, HAL_IO_CMD(HAL_ETH, IPC_READ), exo->eth.phy_addr, io, ERROR_IO_CANCELLED);

    __disable_irq();
    io = exo->eth.tx;
    exo->eth.tx = NULL;
    __enable_irq();
    if (io != NULL)
        io_complete_ex_exo(exo->eth.tcpip, HAL_IO_CMD(HAL_ETH, IPC_WRITE), exo->eth.phy_addr, io, ERROR_IO_CANCELLED);
#endif
}

static void stm32_eth_conn_check(EXO* exo)
{
    ETH_CONN_TYPE new_conn;
    new_conn = eth_phy_get_conn_status(exo->eth.phy_addr);
    if (new_conn != exo->eth.conn)
    {
        exo->eth.conn = new_conn;
        exo->eth.connected = ((exo->eth.conn != ETH_NO_LINK) && (exo->eth.conn != ETH_REMOTE_FAULT));
        kipc_post_exo(exo->eth.tcpip, HAL_CMD(HAL_ETH, ETH_NOTIFY_LINK_CHANGED), 0, exo->eth.conn, 0);
        if (exo->eth.connected)
        {
            //set speed and duplex
            switch (exo->eth.conn)
            {
            case ETH_10_HALF:
                ETH->MACCR &= ~(ETH_MACCR_FES | ETH_MACCR_DM);
                break;
            case ETH_10_FULL:
                ETH->MACCR &= ~ETH_MACCR_FES;
                ETH->MACCR |= ETH_MACCR_DM;
                break;
            case ETH_100_HALF:
                ETH->MACCR &= ~(ETH_MACCR_DM);
                ETH->MACCR |= ETH_MACCR_FES;
                break;
            case ETH_100_FULL:
                ETH->MACCR |= ETH_MACCR_FES | ETH_MACCR_DM;
                break;
            default:
                break;
            }
            //enable RX/TX and DMA operations
            ETH->MACCR |= ETH_MACCR_TE | ETH_MACCR_RE;
            ETH->DMAOMR |= ETH_DMAOMR_SR | ETH_DMAOMR_ST;
        }
        else
        {
            stm32_eth_flush(exo);
            ETH->MACCR &= ~(ETH_MACCR_TE | ETH_MACCR_RE);
            ETH->DMAOMR &= ~(ETH_DMAOMR_SR | ETH_DMAOMR_ST);
        }
    }
    ksystime_soft_timer_start_ms(exo->eth.timer, 1000);
}

void stm32_eth_isr(int vector, void* param)
{
    int i;
    uint32_t sta;
    EXO* exo = param;
    sta = ETH->DMASR;
    if (sta & ETH_DMASR_RS)
    {
#if (ETH_DOUBLE_BUFFERING)
        for (i = 0; i < 2; ++i)
        {
            if ((exo->eth.rx[exo->eth.cur_rx] != NULL) && ((exo->eth.rx_des[exo->eth.cur_rx].ctl & ETH_RDES_OWN) == 0))
            {
                exo->eth.rx[exo->eth.cur_rx]->data_size = (exo->eth.rx_des[exo->eth.cur_rx].ctl & ETH_RDES_FL_MASK) >> ETH_RDES_FL_POS;
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
            exo->eth.rx->data_size = (exo->eth.rx_des.ctl & ETH_RDES_FL_MASK) >> ETH_RDES_FL_POS;
            iio_complete(exo->eth.tcpip, HAL_IO_CMD(HAL_ETH, IPC_READ), exo->eth.phy_addr, exo->eth.rx);
            exo->eth.rx = NULL;
        }
#endif
        ETH->DMASR = ETH_DMASR_RS;
    }
    if (sta & ETH_DMASR_TS)
    {
#if (ETH_DOUBLE_BUFFERING)
        for (i = 0; i < 2; ++i)
        {
            if ((exo->eth.tx[exo->eth.cur_tx] != NULL) && ((exo->eth.tx_des[exo->eth.cur_tx].ctl & ETH_TDES_OWN) == 0))
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
        ETH->DMASR = ETH_DMASR_TS;
    }
    ETH->DMASR = ETH_DMASR_NIS;
}

static void stm32_eth_close(EXO* exo)
{
    //disable interrupts
    NVIC_DisableIRQ(ETH_IRQn);
    kirq_unregister(KERNEL_HANDLE, ETH_IRQn);

    //flush
    stm32_eth_flush(exo);

    //turn phy off
    eth_phy_power_off(exo->eth.phy_addr);

    //disable clocks
    RCC->AHBENR &= ~(RCC_AHBENR_ETHMACEN | RCC_AHBENR_ETHMACTXEN | RCC_AHBENR_ETHMACRXEN);

    //destroy timer
    ksystime_soft_timer_destroy(exo->eth.timer);

    //switch to unconfigured state
    exo->eth.tcpip = INVALID_HANDLE;
    exo->eth.connected = false;
    exo->eth.conn = ETH_NO_LINK;
}

static inline void stm32_eth_open(EXO* exo, unsigned int phy_addr, ETH_CONN_TYPE conn, HANDLE tcpip)
{
    unsigned int clock;

    exo->eth.phy_addr = phy_addr;
    exo->eth.cc = 0;
    exo->eth.timer_pending = false;
    exo->eth.timer = ksystime_soft_timer_create(KERNEL_HANDLE, 0, HAL_ETH);
    if (exo->eth.timer == INVALID_HANDLE)
        return;
    exo->eth.tcpip = tcpip;

    //enable clocks
    RCC->AHBENR |= RCC_AHBENR_ETHMACEN | RCC_AHBENR_ETHMACTXEN | RCC_AHBENR_ETHMACRXEN;

    //reset DMA
    ETH->DMABMR |= ETH_DMABMR_SR;
    while(ETH->DMABMR & ETH_DMABMR_SR) {}

    //setup DMA
#if (ETH_DOUBLE_BUFFERING)
    exo->eth.rx_des[0].ctl = 0;
    exo->eth.rx_des[0].size = ETH_RDES_RCH;
    exo->eth.rx_des[0].buf2_ndes = &exo->eth.rx_des[1];
    exo->eth.rx_des[1].ctl = 0;
    exo->eth.rx_des[1].size = ETH_RDES_RCH;
    exo->eth.rx_des[1].buf2_ndes = &exo->eth.rx_des[0];

    exo->eth.tx_des[0].ctl = ETH_TDES_TCH | ETH_TDES_IC;
    exo->eth.tx_des[0].buf2_ndes = &exo->eth.tx_des[1];
    exo->eth.tx_des[1].ctl = ETH_TDES_TCH | ETH_TDES_IC;
    exo->eth.tx_des[1].buf2_ndes = &exo->eth.tx_des[0];

    exo->eth.cur_rx = exo->eth.cur_tx = 0;
#else
    exo->eth.rx_des.ctl = 0;
    exo->eth.rx_des.size = ETH_RDES_RCH;
    exo->eth.rx_des.buf2_ndes = &exo->eth.rx_des;
    exo->eth.tx_des.ctl = ETH_TDES_TCH;
    exo->eth.tx_des.buf2_ndes = &exo->eth.tx_des;
#endif
    ETH->DMATDLAR = (unsigned int)&exo->eth.tx_des;
    ETH->DMARDLAR = (unsigned int)&exo->eth.rx_des;

    //disable receiver/transmitter before link established
    ETH->MACCR = 0x8000;
    //setup MAC
    ETH->MACA0HR = (exo->eth.mac.u8[5] << 8) | (exo->eth.mac.u8[4] << 0) |  (1 << 31);
    ETH->MACA0LR = (exo->eth.mac.u8[3] << 24) | (exo->eth.mac.u8[2] << 16) | (exo->eth.mac.u8[1] << 8) | (exo->eth.mac.u8[0] << 0);
    //apply MAC unicast filter
#if (MAC_FILTER)
    ETH->MACFFR = ETH_MACFFR_RA;
#else
    ETH->MACFFR = 0;
#endif

    //configure clock for SMI
    clock = stm32_power_get_clock_inside(exo, POWER_BUS_CLOCK);
    if (clock <= 35000000)
        ETH->MACMIIAR = ETH_MACMIIAR_CR_Div16;
    else if (clock <= 60000000)
        ETH->MACMIIAR = ETH_MACMIIAR_CR_Div26;
    else
        ETH->MACMIIAR = ETH_MACMIIAR_CR_Div42;

    //enable dma interrupts
    kirq_register(KERNEL_HANDLE, ETH_IRQn, stm32_eth_isr, exo);
    NVIC_EnableIRQ(ETH_IRQn);
    NVIC_SetPriority(ETH_IRQn, 13);
    ETH->DMAIER = ETH_DMAIER_NISE | ETH_DMAIER_RIE | ETH_DMAIER_TIE;

    //turn phy on
    if (!eth_phy_power_on(exo->eth.phy_addr, conn))
    {
        kerror(ERROR_NOT_FOUND);
        stm32_eth_close(exo);
        return;
    }

    stm32_eth_conn_check(exo);
}

static inline void stm32_eth_read(EXO* exo, IPC* ipc)
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
    exo->eth.rx_des[i].size &= ~ETH_RDES_RBS1_MASK;
    exo->eth.rx_des[i].size |= ((ipc->param3 << ETH_RDES_RBS1_POS) & ETH_RDES_RBS1_MASK);
    __disable_irq();
    exo->eth.rx[i] = io;
    //give descriptor to DMA
    exo->eth.rx_des[i].ctl = ETH_RDES_OWN;
    __enable_irq();
#else
    if (exo->eth.rx != NULL)
    {
        kerror(ERROR_IN_PROGRESS);
        return;
    }
    exo->eth.rx_des.buf1 = io_data(io);
    exo->eth.rx = io;
    exo->eth.rx_des.size &= ~ETH_RDES_RBS1_MASK;
    exo->eth.rx_des.size |= ((ipc->param3 << ETH_RDES_RBS1_POS) & ETH_RDES_RBS1_MASK);
    //give descriptor to DMA
    exo->eth.rx_des.ctl = ETH_RDES_OWN;
#endif
    //enable and poll DMA. Value is doesn't matter
    ETH->DMARPDR = 0;
    kerror(ERROR_SYNC);
}

static inline void stm32_eth_write(EXO* exo, IPC* ipc)
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
    exo->eth.tx_des[i].size = ((io->data_size << ETH_TDES_TBS1_POS) & ETH_TDES_TBS1_MASK);
    exo->eth.tx_des[i].ctl = ETH_TDES_TCH | ETH_TDES_FS | ETH_TDES_LS | ETH_TDES_IC;
    __disable_irq();
    exo->eth.tx[i] = io;
    //give descriptor to DMA
    exo->eth.tx_des[i].ctl |= ETH_TDES_OWN;
    __enable_irq();
#else
    if (exo->eth.tx != NULL)
    {
        kerror(ERROR_IN_PROGRESS);
        return;
    }
    exo->eth.tx_des.buf1 = io_data(io);
    exo->eth.tx = io;
    exo->eth.tx_des.size = ((io->data_size << ETH_TDES_TBS1_POS) & ETH_TDES_TBS1_MASK);
    //give descriptor to DMA
    exo->eth.tx_des.ctl = ETH_TDES_TCH | ETH_TDES_FS | ETH_TDES_LS | ETH_TDES_IC;
    exo->eth.tx_des.ctl |= ETH_TDES_OWN;
#endif
    //enable and poll DMA. Value is doesn't matter
    ETH->DMATPDR = 0;
    kerror(ERROR_SYNC);
}

static inline void stm32_eth_set_mac(EXO* exo, unsigned int param2, unsigned int param3)
{
    exo->eth.mac.u32.hi = param2;
    exo->eth.mac.u32.lo = (uint16_t)param3;
}

static inline void stm32_eth_get_mac(EXO* exo, IPC* ipc)
{
    ipc->param2 = exo->eth.mac.u32.hi;
    ipc->param3 = exo->eth.mac.u32.lo;
}

void stm32_eth_init(EXO* exo)
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
}

void stm32_eth_request(EXO* exo, IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_OPEN:
        stm32_eth_open(exo, ipc->param1, ipc->param2, ipc->process);
        break;
    case ETH_SET_MAC:
        stm32_eth_set_mac(exo, ipc->param2, ipc->param3);
        break;
    case ETH_GET_MAC:
        stm32_eth_get_mac(exo, ipc);
        break;
    case ETH_GET_HEADER_SIZE:
        //no special header required
        ipc->param2 = 0;
        ipc->param3 = ERROR_OK;
        break;
    default:
        if (exo->eth.tcpip == INVALID_HANDLE)
        {
            error(ERROR_NOT_CONFIGURED);
            return;
        }

        ++exo->eth.cc;
        switch (HAL_ITEM(ipc->cmd))
        {
        case IPC_CLOSE:
            stm32_eth_close(exo);
            break;
        case IPC_FLUSH:
            stm32_eth_flush(exo);
            break;
        case IPC_READ:
            stm32_eth_read(exo, ipc);
            break;
        case IPC_WRITE:
            stm32_eth_write(exo, ipc);
            break;
        case IPC_TIMEOUT:
            if (exo->eth.cc == 1)
                stm32_eth_conn_check(exo);
            else
                exo->eth.timer_pending = true;
            break;
        default:
            kerror(ERROR_NOT_SUPPORTED);
            break;
        }

        if ((--exo->eth.cc == 0) && exo->eth.timer_pending)
            stm32_eth_conn_check(exo);
    }
}

