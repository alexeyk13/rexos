/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "../../rexos/sys/sys.h"
#include "../../rexos/sys/drv/stm32_core.h"
#include "../../rexos/sys/drv/stm32_gpio.h"
#include "../../rexos/sys/drv/stm32_power.h"
#include "../../rexos/userspace/lib/stdio.h"
#include "../../rexos/userspace/object.h"
#include "sys_config.h"

#define GREEN                           B4
#define RED                             A5

void app();

const REX __APP = {
    //name
    "App main",
    //size
    512,
    //priority
    120,
    //flags
    PROCESS_FLAGS_ACTIVE | REX_HEAP_FLAGS(HEAP_PERSISTENT_NAME),
    //ipc size
    10,
    //function
    app
};

void app()
{
    HANDLE core;
    core = process_create(&__STM32_CORE);
    process_create(&__STM32_UART);

    open_stdout();

    ack(core, STM32_GPIO_ENABLE_PIN, RED, PIN_MODE_OUT, 0);
    ack(core, STM32_GPIO_ENABLE_PIN, GREEN, PIN_MODE_OUT, 0);
    ack(core, STM32_GPIO_SET_PIN, GREEN, true, 0);

    ack(core, STM32_GPIO_ENABLE_PIN, RED, PIN_MODE_OUT, 0);
    ack(core, STM32_GPIO_ENABLE_PIN, GREEN, PIN_MODE_OUT, 0);
    ack(core, STM32_GPIO_SET_PIN, GREEN, true, 0);

    bool set = true;
    printf("core clock: %d\n\r", get(core, STM32_POWER_GET_CLOCK, STM32_CLOCK_CORE, 0, 0));
    for (;;)
    {
        ack(core, STM32_GPIO_SET_PIN, RED, set, 0);
        printf("test printf\n\r");
        set = !set;
        sleep_ms(1000);
    }
}
