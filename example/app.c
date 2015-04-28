/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "../userspace/sys.h"
#include "../userspace/gpio.h"
#include "../drv/stm32/stm32_power.h"
#include "../userspace/stdio.h"
#include "../userspace/stdlib.h"
#include "../userspace/ipc.h"
#include "../userspace/timer.h"
#include "../userspace/file.h"
#include "../userspace/wdt.h"
#include "../userspace/uart.h"
#include "../midware/pinboard.h"
#include "app_private.h"
#include "comm.h"
#include "config.h"


void app();

const REX __APP = {
    //name
    "App main",
    //size
    1024,
    //priority
    200,
    //flags
    PROCESS_FLAGS_ACTIVE | REX_HEAP_FLAGS(HEAP_PERSISTENT_NAME),
    //ipc size
    5,
    //function
    app
};

static inline void stat()
{
    TIME uptime;
    int i;
    unsigned int diff;

    get_uptime(&uptime);
    for (i = 0; i < TEST_ROUNDS; ++i)
        svc_test();
    diff = time_elapsed_us(&uptime);
    printf("average kernel call time: %d.%dus\n\r", diff / TEST_ROUNDS, (diff / (TEST_ROUNDS / 10)) % 10);

    get_uptime(&uptime);
    for (i = 0; i < TEST_ROUNDS; ++i)
        process_switch_test();
    diff = time_elapsed_us(&uptime);
    printf("average switch time: %d.%dus\n\r", diff / TEST_ROUNDS, (diff / (TEST_ROUNDS / 10)) % 10);

    printf("core clock: %d\n\r", get(object_get(SYS_OBJ_CORE), STM32_POWER_GET_CLOCK, STM32_CLOCK_CORE, 0, 0));
    process_info();
}

static inline void app_setup_dbg()
{
    BAUD baudrate;
    ack(object_get(SYS_OBJ_CORE), STM32_GPIO_ENABLE_PIN, DBG_CONSOLE_TX_PIN, STM32_GPIO_MODE_AF | GPIO_OT_PUSH_PULL | GPIO_SPEED_HIGH, DBG_CONSOLE_TX_PIN_AF);
    fopen(object_get(SYS_OBJ_UART), HAL_HANDLE(HAL_UART, DBG_CONSOLE), FILE_MODE_WRITE);
    baudrate.baud = DBG_CONSOLE_BAUD;
    baudrate.data_bits = 8;
    baudrate.parity = 'N';
    baudrate.stop_bits= 1;
    uart_set_baudrate(DBG_CONSOLE, &baudrate);
    uart_setup_printk(DBG_CONSOLE);
    uart_setup_stdout(DBG_CONSOLE);
    open_stdout();
}

static inline void app_init(APP* app)
{
    process_create(&__STM32_CORE);

    gpio_enable_pin(B14, GPIO_MODE_OUT);
    gpio_reset_pin(B14);


    app_setup_dbg();

    app->timer = timer_create(HAL_HANDLE(HAL_APP, 0));
    timer_start_ms(app->timer, 1000, 0);

    stat();
    printf("App init\n\r");
}

static inline void app_timeout(APP* app)
{
    printf("app timer timeout test\n\r");
    timer_start_ms(app->timer, 1000, 0);
}

void app()
{
    APP app;
    IPC ipc;
    bool need_post = false;

    app_init(&app);
    comm_init(&app);

    for (;;)
    {
        error(ERROR_OK);
        ipc_read_ms(&ipc, 0, ANY_HANDLE);
        need_post = false;
        switch (ipc.cmd)
        {
        case IPC_PING:
            need_post = true;
            break;
        case USBD_ALERT:
            need_post = comm_usbd_alert(&app, ipc.param1);
            break;
        case IPC_STREAM_WRITE:
            comm_usbd_stream_rx(&app, ipc.param3);
            break;
        case IPC_TIMEOUT:
            app_timeout(&app);
            break;
        default:
            printf("Unhandled cmd: %#X\n\r", ipc.cmd);
            printf("sender: %#X\n\r", ipc.process);
            printf("p1: %#X\n\r", ipc.param1);
            error(ERROR_NOT_SUPPORTED);
            need_post = true;
            break;
        }
        if (need_post)
            ipc_post_or_error(&ipc);
    }
}
