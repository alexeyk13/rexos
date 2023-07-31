/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: Alexey E. Kramarenko (alexeyk13@yandex.ru)
*/

#include "net.h"
#include "app_private.h"
#include "config.h"
#include "../../rexos/userspace/eth.h"
#include "../../rexos/userspace/ip.h"
#include "../../rexos/userspace/icmp.h"
#include "../../rexos/userspace/tcp.h"
#include "../../rexos/userspace/tcpip.h"
#include "../../rexos/userspace/stm32/stm32_driver.h"
#include "../../rexos/userspace/stdio.h"
#include "../../rexos/userspace/pin.h"
#include <string.h>

static const MAC __MAC =                    {{0x20, 0xD9, 0x97, 0xA1, 0x90, 0x42}};
static const IP __IP =                      {{192, 168, 8, 126}};

static const IP __HOST_IP =                 {{192, 168, 8, 1}};
#define PING_COUNT                          5

void net_init(APP* app)
{
    pin_enable(ETH_MDC, STM32_GPIO_MODE_OUTPUT_AF_PUSH_PULL_50MHZ, false);
    pin_enable(ETH_MDIO, STM32_GPIO_MODE_OUTPUT_AF_PUSH_PULL_50MHZ, false);

    pin_enable(ETH_TX_CLK, STM32_GPIO_MODE_INPUT_FLOAT, false);
    pin_enable(ETH_TX_EN, STM32_GPIO_MODE_OUTPUT_AF_PUSH_PULL_50MHZ, false);
    pin_enable(ETH_TX_D0, STM32_GPIO_MODE_OUTPUT_AF_PUSH_PULL_50MHZ, false);
    pin_enable(ETH_TX_D1, STM32_GPIO_MODE_OUTPUT_AF_PUSH_PULL_50MHZ, false);
    pin_enable(ETH_TX_D2, STM32_GPIO_MODE_OUTPUT_AF_PUSH_PULL_50MHZ, false);
    pin_enable(ETH_TX_D3, STM32_GPIO_MODE_OUTPUT_AF_PUSH_PULL_50MHZ, false);

    pin_enable(ETH_RX_CLK, STM32_GPIO_MODE_INPUT_FLOAT, false);
    pin_enable(ETH_RX_DV, STM32_GPIO_MODE_INPUT_FLOAT, false);
    pin_enable(ETH_RX_ER, STM32_GPIO_MODE_INPUT_FLOAT, false);
    pin_enable(ETH_RX_D0, STM32_GPIO_MODE_INPUT_FLOAT, false);
    pin_enable(ETH_RX_D1, STM32_GPIO_MODE_INPUT_FLOAT, false);
    pin_enable(ETH_RX_D2, STM32_GPIO_MODE_INPUT_FLOAT, false);
    pin_enable(ETH_RX_D3, STM32_GPIO_MODE_INPUT_FLOAT, false);

    pin_enable(ETH_COL, STM32_GPIO_MODE_INPUT_FLOAT, false);
    pin_enable(ETH_CRS_WKUP, STM32_GPIO_MODE_INPUT_FLOAT, false);

    eth_set_mac(KERNEL_HANDLE, 0, &__MAC);

    app->net.tcpip = tcpip_create(TCPIP_PROCESS_SIZE, TCPIP_PROCESS_PRIORITY, ETH_PHY_ADDRESS);
    ip_set(app->net.tcpip, &__IP);
    tcpip_open(app->net.tcpip, KERNEL_HANDLE, ETH_PHY_ADDRESS, ETH_AUTO);

    app->net.io = io_create(504);
}

static inline void ping_test(APP* app)
{
    SYSTIME uptime;
    int i, success, total, cur;
    success = 0;
    total = 0;
    printf("Starting ping test to ");
    ip_print(&__HOST_IP);
    printf("\n");
    for (i = 0; i < PING_COUNT; ++i)
    {
        printf("Ping %d: ", i + 1);
        get_uptime(&uptime);
        if (icmp_ping(app->net.tcpip, &__HOST_IP))
        {
            cur = systime_elapsed_us(&uptime);
            total += cur;
            ++success;
            printf("OK! (%d.%d ms)\n", cur / 1000, cur % 1000);
        }
        else
            printf("Fail!\n");
    }
    total /= success;
    printf("Ping test done. Success %d/%d(%d %%). Average time: %d.%d\n", success, PING_COUNT, (success * 100) / PING_COUNT, total / 1000, total % 1000);
}

static inline void ip_up(APP* app)
{
    printf("Network interface up\n");
    ping_test(app);

    app->net.listener = tcp_listen(app->net.tcpip, 23);
    app->net.connection = INVALID_HANDLE;
}

static inline void ip_down(APP* app)
{
    printf("Network interface down\n");
}

static inline void ip_request(APP* app, IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
    case IP_UP:
        ip_up(app);
        break;
    case IP_DOWN:
        ip_down(app);
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
        break;
    }
}

static inline void net_open(APP* app, HANDLE con)
{
    TCP_STACK* tcp_stack;
    tcp_get_remote_addr(app->net.tcpip, con, &app->net.remote_addr);
    printf("NET: got socket connection from ");
    ip_print(&app->net.remote_addr);
    printf("\n");
    if (app->net.connection != INVALID_HANDLE)
    {
        printf("NET: already have connection\n");
        tcp_close(app->net.tcpip, con);
        return;
    }
    app->net.connection = con;
    strcpy(io_data(app->net.io), "REx OS 0.3.9\n\r");
    app->net.io->data_size = strlen(io_data(app->net.io));
    tcp_stack = io_push(app->net.io, sizeof(TCP_STACK));
    tcp_stack->flags = TCP_PSH;
    tcp_write_sync(app->net.tcpip, app->net.connection, app->net.io);
    tcp_read(app->net.tcpip, app->net.connection, app->net.io, 500);
}

static inline void net_close(APP* app)
{
    printf("NET: Closed socket connection from ");
    ip_print(&app->net.remote_addr);
    printf("\n");
    app->net.connection = INVALID_HANDLE;
}

static inline void net_read_complete(APP* app, int size)
{
    int i;
    uint8_t c;
    if (size > 0)
    {
        printf("rx: ");
        for (i = 0; i < app->net.io->data_size; ++i)
        {
            c = ((uint8_t*)io_data(app->net.io))[i];
            if (c >= ' ' && c <= '~')
                printf("%c", c);
            else
                printf("\\x%02x", c);
        }
        printf("\n");
        //echo send
        tcp_write_sync(app->net.tcpip, app->net.connection, app->net.io);
        io_reset(app->net.io);
        tcp_read(app->net.tcpip, app->net.connection, app->net.io, 500);
    }
}

static inline void tcp_request(APP* app, IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_OPEN:
        net_open(app, (HANDLE)ipc->param1);
        break;
    case IPC_CLOSE:
        net_close(app);
        break;
    case IPC_READ:
        net_read_complete(app, (int)ipc->param3);
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
        break;
    }
}

void net_request(APP* app, IPC* ipc)
{
    switch (HAL_GROUP(ipc->cmd))
    {
    case HAL_IP:
        ip_request(app, ipc);
        break;
    case HAL_TCP:
        tcp_request(app, ipc);
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
    }
}
