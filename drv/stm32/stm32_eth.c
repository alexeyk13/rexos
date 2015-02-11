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
#include <stdbool.h>

#define _printd                 printf
#define get_clock               stm32_power_get_clock_outside
#define ack_gpio                stm32_core_request_outside
#define ack_power               stm32_core_request_outside

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

void stm32_eth_init(ETH_DRV* drv)
{
    drv->stub = 0;
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
        //TODO:
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
