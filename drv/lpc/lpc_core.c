/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "lpc_core.h"
#include "lpc_core_private.h"
//#include "lpc_timer.h"
#include "lpc_pin.h"
//#include "lpc_power.h"
#include "../../userspace/object.h"

#include "lpc_uart.h"
#include "lpc_i2c.h"
#ifdef LPC11Uxx
#include "lpc_usb.h"
#else //LPC18xx
#include "lpc_otg.h"
#endif //LPC11Uxx
#include "lpc_eep.h"

void lpc_core();

const REX __LPC_CORE = {
    //name
    "LPC core driver",
    //size
    LPC_CORE_PROCESS_SIZE,
    //priority - driver priority
    90,
    //flags
    PROCESS_FLAGS_ACTIVE | REX_FLAG_PERSISTENT_NAME,
    //function
    lpc_core
};

void lpc_core_loop(CORE* core)
{
    IPC ipc;
    bool need_post;
    for (;;)
    {
        ipc_read(&ipc);
        need_post = false;
        switch (HAL_GROUP(ipc.cmd))
        {
        case HAL_POWER:
            need_post = lpc_power_request(core, &ipc);
            break;
        case HAL_PIN:
            need_post = lpc_pin_request(&ipc);
            break;
        case HAL_TIMER:
            need_post = lpc_timer_request(core, &ipc);
            break;
#if (MONOLITH_UART)
        case HAL_UART:
            need_post = lpc_uart_request(core, &ipc);
            break;
#endif //MONOLITH_UART
#if (LPC_I2C_DRIVER)
        case HAL_I2C:
            need_post = lpc_i2c_request(core, &ipc);
            break;
#endif //LPC_I2C_DRIVER
#if (MONOLITH_USB)
        case HAL_USB:
#ifdef LPC11Uxx
            need_post = lpc_usb_request(core, &ipc);
#else //LPC18xx
            need_post = lpc_otg_request(core, &ipc);
#endif //LPC11Uxx
            break;
#endif //MONOLITH_USB
#if (LPC_EEPROM_DRIVER)
        case HAL_EEPROM:
            need_post = lpc_eep_request(core, &ipc);
            break;
#endif //LPC_EEPROM_DRIVER
        default:
            error(ERROR_NOT_SUPPORTED);
            need_post = true;
            break;
        }
        if (need_post)
            ipc_post_or_error(&ipc);
    }
}

void lpc_core()
{
    CORE core;
    object_set_self(SYS_OBJ_CORE);

    lpc_power_init(&core);
    lpc_pin_init(&core);
    lpc_timer_init(&core);
#if (MONOLITH_UART)
    lpc_uart_init(&core);
#endif
#if (LPC_I2C_DRIVER)
    lpc_i2c_init(&core);
#endif //LPC_I2C_DRIVER
#if (MONOLITH_USB)
#ifdef LPC11Uxx
    lpc_usb_init(&core);
#else //LPC18xx
    lpc_otg_init(&core);
#endif //LPC11Uxx
#endif //MONOLITH_USB
#if (LPC_EEPROM_DRIVER)
    lpc_eep_init(&core);
#endif

    lpc_core_loop(&core);
}
