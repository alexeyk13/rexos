/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: Alexey E. Kramarenko (alexeyk13@yandex.ru)
*/

#ifndef CONFIG_H
#define CONFIG_H

#define USB_PORT_NUM                                USB_0
#define USBD_PROCESS_SIZE                           900
#define USBD_PROCESS_PRIORITY                       150

#define ETH_PHY_ADDRESS                             0x1f
#define ETH_MDC                                     C1
#define ETH_MDIO                                    A2
#define ETH_TX_CLK                                  C3
#define ETH_TX_EN                                   B11
#define ETH_TX_D0                                   B12
#define ETH_TX_D1                                   B13
#define ETH_TX_D2                                   C2
#define ETH_TX_D3                                   B8

#define ETH_RX_CLK                                  A1
#define ETH_RX_ER                                   B10

#define ETH_RX_DV                                   A7
#define ETH_RX_D0                                   C4
#define ETH_RX_D1                                   C5
#define ETH_RX_D2                                   B0
#define ETH_RX_D3                                   B1

#define ETH_PPS_OUT                                 B5
#define ETH_COL                                     A3
#define ETH_CRS_WKUP                                A0

#define TCPIP_PROCESS_SIZE                          1200
#define TCPIP_PROCESS_PRIORITY                      149

#define DBG_CONSOLE                                 UART_2
#define DBG_CONSOLE_BAUD                            115200
#define DBG_CONSOLE_TX_PIN                          D5

#define TEST_ROUNDS                                 10000

#endif // CONFIG_H
