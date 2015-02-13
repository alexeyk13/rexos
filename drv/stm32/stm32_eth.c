/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "stm32_eth.h"
#include "stm32_config.h"
#include "sys_config.h"
#include "../../userspace/stdio.h"
#include "../../userspace/ipc.h"
#include "../../userspace/sys.h"
#include "../../userspace/direct.h"
#include "../../userspace/timer.h"
#include "../../userspace/stm32_driver.h"
#include "../eth_phy.h"
#include "stm32_gpio.h"
#include "stm32_power.h"
#include <stdbool.h>
#include <string.h>

#define _printd                 printf
#define get_clock               stm32_power_get_clock_outside
#define ack_gpio                stm32_core_request_outside
#define ack_power               stm32_core_request_outside

#define STM32_ETH_MDC           C1
#define STM32_ETH_MDIO          A2
#define STM32_ETH_TX_CLK        C3
#define STM32_ETH_TX_EN         B11
#define STM32_ETH_TX_D0         B12
#define STM32_ETH_TX_D1         B13
#define STM32_ETH_TX_D2         C2
#define STM32_ETH_TX_D3         B8

#define STM32_ETH_RX_CLK        A1
#define STM32_ETH_RX_ER         B10

#if (STM32_ETH_REMAP)
#define STM32_ETH_RX_DV         D8
#define STM32_ETH_RX_D0         D9
#define STM32_ETH_RX_D1         D10
#define STM32_ETH_RX_D2         D11
#define STM32_ETH_RX_D3         D12
#else
#define STM32_ETH_RX_DV         A7
#define STM32_ETH_RX_D0         C4
#define STM32_ETH_RX_D1         C5
#define STM32_ETH_RX_D2         B0
#define STM32_ETH_RX_D3         B1
#endif

#define STM32_ETH_PPS_OUT       B5
#define STM32_ETH_COL           A3
#define STM32_ETH_CRS_WKUP      A0

void stm32_eth();

const REX __STM32_ETH = {
    //name
    "STM32 ETH",
    //size
    STM32_ETH_PROCESS_SIZE,
    //priority - driver priority
    91,
    //flags
    PROCESS_FLAGS_ACTIVE | REX_HEAP_FLAGS(HEAP_PERSISTENT_NAME),
    //ipc size
    STM32_ETH_IPC_COUNT,
    //function
    stm32_eth
};

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

static void stm32_eth_conn_check(ETH_DRV* drv)
{
    ETH_CONN_TYPE new_conn;
    new_conn = eth_phy_get_conn_status(ETH_PHY_ADDRESS);
    if (new_conn != drv->conn)
    {
        drv->conn = new_conn;
        ipc_post_inline(drv->arp, ETH_NOTIFY_LINK_CHANGED, HAL_HANDLE(HAL_ETH, 0), drv->conn, 0);
    }
    timer_start_ms(drv->timer, 1000, 0);
}

static inline void stm32_eth_open(ETH_DRV* drv, ETH_CONN_TYPE conn, HANDLE arp)
{
    unsigned int clock;
    if (drv->active)
    {
        error(ERROR_ALREADY_CONFIGURED);
        return;
    }
    //enable pins
    ack_gpio(drv, STM32_GPIO_ENABLE_PIN, STM32_ETH_MDC, STM32_GPIO_MODE_OUTPUT_AF_PUSH_PULL_50MHZ, false);
    ack_gpio(drv, STM32_GPIO_ENABLE_PIN, STM32_ETH_MDIO, STM32_GPIO_MODE_OUTPUT_AF_PUSH_PULL_50MHZ, false);

    ack_gpio(drv, STM32_GPIO_ENABLE_PIN, STM32_ETH_TX_CLK, STM32_GPIO_MODE_INPUT_FLOAT, false);
    ack_gpio(drv, STM32_GPIO_ENABLE_PIN, STM32_ETH_TX_EN, STM32_GPIO_MODE_OUTPUT_AF_PUSH_PULL_50MHZ, false);
    ack_gpio(drv, STM32_GPIO_ENABLE_PIN, STM32_ETH_TX_D0, STM32_GPIO_MODE_OUTPUT_AF_PUSH_PULL_50MHZ, false);
    ack_gpio(drv, STM32_GPIO_ENABLE_PIN, STM32_ETH_TX_D1, STM32_GPIO_MODE_OUTPUT_AF_PUSH_PULL_50MHZ, false);
    ack_gpio(drv, STM32_GPIO_ENABLE_PIN, STM32_ETH_TX_D2, STM32_GPIO_MODE_OUTPUT_AF_PUSH_PULL_50MHZ, false);
    ack_gpio(drv, STM32_GPIO_ENABLE_PIN, STM32_ETH_TX_D3, STM32_GPIO_MODE_OUTPUT_AF_PUSH_PULL_50MHZ, false);

    ack_gpio(drv, STM32_GPIO_ENABLE_PIN, STM32_ETH_RX_CLK, STM32_GPIO_MODE_INPUT_FLOAT, false);
    ack_gpio(drv, STM32_GPIO_ENABLE_PIN, STM32_ETH_RX_DV, STM32_GPIO_MODE_INPUT_FLOAT, false);
    ack_gpio(drv, STM32_GPIO_ENABLE_PIN, STM32_ETH_RX_ER, STM32_GPIO_MODE_INPUT_FLOAT, false);
    ack_gpio(drv, STM32_GPIO_ENABLE_PIN, STM32_ETH_RX_D0, STM32_GPIO_MODE_INPUT_FLOAT, false);
    ack_gpio(drv, STM32_GPIO_ENABLE_PIN, STM32_ETH_RX_D1, STM32_GPIO_MODE_INPUT_FLOAT, false);
    ack_gpio(drv, STM32_GPIO_ENABLE_PIN, STM32_ETH_RX_D2, STM32_GPIO_MODE_INPUT_FLOAT, false);
    ack_gpio(drv, STM32_GPIO_ENABLE_PIN, STM32_ETH_RX_D3, STM32_GPIO_MODE_INPUT_FLOAT, false);

#if (STM32_ETH_REMAP)
    AFIO->MAPR |= AFIO_MAPR_ETH_REMAP;
#endif
#if (STM32_ETH_PPS_OUT_ENABLE)
    ack_gpio(drv, STM32_GPIO_ENABLE_PIN, STM32_ETH_PPS_OUT, STM32_GPIO_MODE_OUTPUT_AF_PUSH_PULL_50MHZ, false);
#endif
    ack_gpio(drv, STM32_GPIO_ENABLE_PIN, STM32_ETH_COL, STM32_GPIO_MODE_INPUT_FLOAT, false);
    ack_gpio(drv, STM32_GPIO_ENABLE_PIN, STM32_ETH_CRS_WKUP, STM32_GPIO_MODE_INPUT_FLOAT, false);

    //enable clocks
    RCC->AHBENR |= RCC_AHBENR_ETHMACEN | RCC_AHBENR_ETHMACTXEN | RCC_AHBENR_ETHMACRXEN;

    drv->active = true;
    drv->arp = arp;

    //configure clock for SMI
    clock = get_clock(drv, STM32_CLOCK_AHB);
    if (clock <= 35000000)
        ETH->MACMIIAR = ETH_MACMIIAR_CR_Div16;
    else if (clock <= 60000000)
        ETH->MACMIIAR = ETH_MACMIIAR_CR_Div26;
    else
        ETH->MACMIIAR = ETH_MACMIIAR_CR_Div42;
    //turn phy on
    if (!eth_phy_power_on(ETH_PHY_ADDRESS, conn))
    {
        error(ERROR_NOT_FOUND);
        //TODO: disable internal
        return;
    }

    stm32_eth_conn_check(drv);
}

static inline void stm32_eth_set_mac(ETH_DRV* drv, unsigned int param1, unsigned int param2)
{
    if (drv->active)
    {
        error(ERROR_IN_PROGRESS);
        return;
    }
    drv->mac[0] = (param1 >> 24) & 0xff;
    drv->mac[1] = (param1 >> 16) & 0xff;
    drv->mac[2] = (param1 >> 8) & 0xff;
    drv->mac[3] = (param1 >> 0) & 0xff;
    drv->mac[4] = (param2 >> 8) & 0xff;
    drv->mac[5] = (param2 >> 0) & 0xff;
}

static inline void stm32_eth_get_mac(ETH_DRV* drv, IPC* ipc)
{
    ipc->param1 = (drv->mac[0] << 24) | (drv->mac[1] << 16) | (drv->mac[2] << 8) | drv->mac[3];
    ipc->param2 = (drv->mac[4] << 8) | drv->mac[5];
}

void stm32_eth_init(ETH_DRV* drv)
{
    drv->active = false;
    drv->timer = timer_create(HAL_HANDLE(HAL_ETH, 0));
    drv->arp = INVALID_HANDLE;
    drv->conn = ETH_NO_LINK;
    memset(drv->mac, 0x00, MAC_SIZE);
    drv->rx_block_count = drv->tx_block_count = 0;
}

bool stm32_eth_request(ETH_DRV* drv, IPC* ipc)
{
    bool need_post = false;
    switch (ipc->cmd)
    {
#if (SYS_INFO)
    case IPC_GET_INFO:
//        stm32_eth_info(drv);
        need_post = true;
        break;
#endif
    case IPC_OPEN:
        stm32_eth_open(drv, ipc->param2, ipc->process);
        need_post = true;
        break;
    case IPC_CLOSE:
        //TODO:
        need_post = true;
        break;
    case IPC_FLUSH:
        //TODO:
        need_post = true;
        break;
    case IPC_READ:
        if ((int)ipc->param3 < 0)
            break;
        //TODO:
        //generally posted with block, no return IPC
        break;
    case IPC_WRITE:
        if ((int)ipc->param3 < 0)
            break;
        //TODO:
        //generally posted with block, no return IPC
        break;
    case IPC_TIMEOUT:
        stm32_eth_conn_check(drv);
        break;
    case ETH_SET_MAC:
        stm32_eth_set_mac(drv, ipc->param1, ipc->param2);
        need_post = true;
        break;
    case ETH_GET_MAC:
        stm32_eth_get_mac(drv, ipc);
        need_post = true;
        break;
    default:
        break;
    }
    return need_post;
}

void stm32_eth()
{
    IPC ipc;
    ETH_DRV drv;
    bool need_post;
    stm32_eth_init(&drv);
#if (SYS_INFO)
    open_stdout();
#endif
    object_set_self(SYS_OBJ_ETH);
    for (;;)
    {
        error(ERROR_OK);

        need_post = false;
        ipc_read_ms(&ipc, 0, ANY_HANDLE);
        switch (ipc.cmd)
        {
        case IPC_PING:
            need_post = true;
            break;
        default:
            need_post = stm32_eth_request(&drv, &ipc);
            break;
        }
        if (need_post)
            ipc_post_or_error(&ipc);
    }
}
