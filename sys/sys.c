/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "sys_config.h"
#include "sys.h"
#include "../userspace/svc.h"
#include "../userspace/process.h"
#include "../userspace/lib/stdio.h"

//TODO refactor all this shit
#include "../arch/cortex_m3/stm/timer_stm32.h"
#include "drv/rcc_stm32f2.h"
#include "drv/uart_stm32.h"

void sys();

const REX __SYS = {
    //name
    "RExOS SYS",
    //size
    512,
    //priority - sys priority
    101,
    //flags
    PROCESS_FLAGS_ACTIVE,
    //ipc size
    10,
    //function
    sys
};

static inline void sys_loop ()
{
    IPC ipc;
    for (;;)
    {
        ipc_wait_peek_ms(&ipc, 0, 0);
        switch (ipc.cmd)
        {
        case IPC_PING:
            ipc.cmd = IPC_PONG;
            ipc_post(&ipc);
            break;
        case IPC_UNKNOWN:
            //ignore unknown IPC
            break;
        default:
            ipc.cmd = IPC_UNKNOWN;
            ipc_post(&ipc);
        }
    }
}

void sys ()
{
    setup_system();
    //TODO too much refactoring
    set_core_freq(0);

    uart_enable(UART_2, (P_UART_CB)NULL, NULL, 2);
    UART_BAUD baud;
    baud.data_bits = 8;
    baud.parity = 'N';
    baud.stop_bits = 1;
    baud.baud = 115200;
    uart_set_baudrate(UART_2, &baud);

    setup_dbg(uart_write_svc, (void*)UART_2);
    //refactor me later
    setup_stdout(uart_write_svc, (void*)UART_2);
    __HEAP->stdout = uart_write_svc;
    __HEAP->stdout_param = (void*)UART_2;

    timer_init_hw();


#if (SYS_DEBUG)
    printf("RExOS system v. 0.0.1 started\n\r");
#endif
    sys_loop();
}
