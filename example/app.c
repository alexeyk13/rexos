/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "../../rexos/userspace/stdio.h"
#include "../../rexos/userspace/stdlib.h"
#include "../../rexos/userspace/process.h"
#include "../../rexos/userspace/sys.h"
#include "../../rexos/userspace/gpio.h"
#include "../../rexos/userspace/stm32_driver.h"
#include "../../rexos/userspace/ipc.h"
#include "../../rexos/userspace/systime.h"
#include "../../rexos/userspace/wdt.h"
#include "../../rexos/userspace/uart.h"
#include "../../rexos/userspace/process.h"
#include "../../rexos/midware/pinboard.h"
#include "app_private.h"
#include "comm.h"
#include "net.h"
#include "config.h"
#include "../../rexos/userspace/adc.h"


void app();

const REX __APP = {
    //name
    "App main",
    //size
    1024,
    //priority
    200,
    //flags
    PROCESS_FLAGS_ACTIVE | REX_FLAG_PERSISTENT_NAME,
    //function
    app
};


static inline void stat()
{
    SYSTIME uptime;
    int i;
    unsigned int diff;

    get_uptime(&uptime);
    for (i = 0; i < TEST_ROUNDS; ++i)
        svc_test();
    diff = systime_elapsed_us(&uptime);
    printf("average kernel call time: %d.%dus\n", diff / TEST_ROUNDS, (diff / (TEST_ROUNDS / 10)) % 10);

    get_uptime(&uptime);
    for (i = 0; i < TEST_ROUNDS; ++i)
        process_switch_test();
    diff = systime_elapsed_us(&uptime);
    printf("average switch time: %d.%dus\n", diff / TEST_ROUNDS, (diff / (TEST_ROUNDS / 10)) % 10);

    printf("core clock: %d\n", get_handle(object_get(SYS_OBJ_CORE), HAL_REQ(HAL_POWER, STM32_POWER_GET_CLOCK), STM32_CLOCK_CORE, 0, 0));
    process_info();
}

static inline void app_setup_dbg()
{
    BAUD baudrate;
    ack(object_get(SYS_OBJ_CORE), HAL_REQ(HAL_PIN, STM32_GPIO_ENABLE_PIN), DBG_CONSOLE_TX_PIN, STM32_GPIO_MODE_OUTPUT_AF_PUSH_PULL_50MHZ, false);
    uart_open(DBG_CONSOLE, UART_MODE_STREAM | UART_TX_STREAM);
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
    app->timer = timer_create(0, HAL_APP);
//    timer_start_ms(app->timer, 1000);

    stat();
    printf("App init\n");
}

static inline void app_timeout(APP* app)
{
    printf("app timer timeout test\n");
    timer_start_ms(app->timer, 1000);
}

void app()
{
    APP app;
    IPC ipc;

    app_init(&app);
    comm_init(&app);
    net_init(&app);

    for (;;)
    {
        ipc_read(&ipc);
        switch (HAL_GROUP(ipc.cmd))
        {
        case HAL_USBD:
            comm_request(&app, &ipc);
            break;
        case HAL_IP:
        case HAL_UDP:
        case HAL_TCP:
            net_request(&app, &ipc);
            break;
        case HAL_APP:
            app_timeout(&app);
            break;
        default:
            error(ERROR_NOT_SUPPORTED);
            break;
        }
        ipc_write(&ipc);
    }
}
