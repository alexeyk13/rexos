/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "../../rexos/userspace/sys.h"
#include "../../rexos/userspace/gpio.h"
#include "../../rexos/sys/drv/stm32/stm32_core.h"
#include "../../rexos/sys/drv/stm32/stm32_power.h"
#include "../../rexos/userspace/stm32_driver.h"
#include "../../rexos/userspace/stdio.h"
#include "../../rexos/userspace/object.h"
#include "../../rexos/userspace/gpio.h"
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

    gpio_enable_pin(RED, GPIO_MODE_OUT);
    gpio_enable_pin(GREEN, GPIO_MODE_OUT);
    gpio_set_pin(GREEN);

    bool set = true;
    printf("core clock: %d\n\r", get(core, STM32_POWER_GET_CLOCK, STM32_CLOCK_CORE, 0, 0));
    for (;;)
    {
        if (set)
          gpio_set_pin(RED);
        else
          gpio_reset_pin(RED);
        printf("test printf\n\r");
        set = !set;
        sleep_ms(1000);
    }
}
