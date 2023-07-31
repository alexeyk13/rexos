/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: Alexey E. Kramarenko (alexeyk13@yandex.ru)
*/

#include "userspace/stdio.h"
#include "userspace/stdlib.h"
#include "userspace/process.h"
#include "userspace/sys.h"
#include "userspace/gpio.h"
#include "userspace/stm32/stm32.h"
#include "userspace/stm32/stm32_driver.h"
#include "userspace/ipc.h"
#include "userspace/systime.h"
#include "userspace/wdt.h"
#include "userspace/uart.h"
#include "userspace/process.h"
#include "userspace/power.h"
#include "userspace/adc.h"
#include "userspace/pin.h"
#include "midware/pinboard.h"
#include "app_private.h"
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

    printf("core clock: %d\n", power_get_core_clock());
    process_info();
}

static inline void app_setup_dbg()
{
    BAUD baudrate;
    pin_enable(DBG_CONSOLE_TX_PIN, STM32_GPIO_MODE_OUTPUT_AF_PUSH_PULL_50MHZ, false);
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
    gpio_enable_pin(C8, GPIO_MODE_OUT);
    gpio_set_pin(C8);

    app_setup_dbg();
    app->timer = timer_create(0, HAL_APP);
    timer_start_ms(app->timer, 1000);

    stat();
    printf("App init\n");
}

static inline void app_timeout(APP* app)
{
    printf("app timer timeout test\n");
    timer_start_ms(app->timer, 1000);
    if(gpio_get_pin(C8)) {
        gpio_reset_pin(C8);
    } else {
        gpio_set_pin(C8);
    }

}

void app()
{
    APP app;
    IPC ipc;

    app_init(&app);

    for (;;)
    {
        ipc_read(&ipc);
        switch (HAL_GROUP(ipc.cmd))
        {
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
