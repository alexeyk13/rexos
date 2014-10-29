/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "../../rexos/sys/sys.h"
#include "../../rexos/sys/drv/stm32_core.h"
#include "../../rexos/sys/gpio.h"
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

    open_stdout();

    gpio_enable_pin(RED, PIN_MODE_OUT);
    gpio_enable_pin(GREEN, PIN_MODE_OUT);
    gpio_set_pin(GREEN, true);

    bool set = true;
    printf("core clock: %d\n\r", get(core, STM32_POWER_GET_CLOCK, STM32_CLOCK_CORE, 0, 0));
    for (;;)
    {
        gpio_set_pin(RED, set);
        printf("test printf\n\r");
        set = !set;
        sleep_ms(1000);
    }
}
