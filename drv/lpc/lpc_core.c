/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2016, Alexey Kramarenko
    All rights reserved.
*/

#include "lpc_core.h"
#include "lpc_core_private.h"
//#include "lpc_timer.h"
#include "lpc_pin.h"
//#include "lpc_power.h"
#include "../../userspace/object.h"
#include "../../userspace/svc.h"

#include "lpc_uart.h"
#include "lpc_i2c.h"
#ifdef LPC11Uxx
#include "lpc_usb.h"
#else //LPC18xx
#include "lpc_otg.h"
#endif //LPC11Uxx
#include "lpc_eep.h"
#include "lpc_sdmmc.h"

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
    for (;;)
    {
        ipc_read(&ipc);
        switch (HAL_GROUP(ipc.cmd))
        {
        case HAL_POWER:
            lpc_power_request(core, &ipc);
            break;
        case HAL_PIN:
            lpc_pin_request(&ipc);
            break;
        case HAL_TIMER:
            lpc_timer_request(core, &ipc);
            break;
        case HAL_UART:
            lpc_uart_request(core, &ipc);
            break;
#if (LPC_I2C_DRIVER)
        case HAL_I2C:
            lpc_i2c_request(core, &ipc);
            break;
#endif //LPC_I2C_DRIVER
#if (MONOLITH_USB)
        case HAL_USB:
#ifdef LPC11Uxx
            lpc_usb_request(core, &ipc);
#else //LPC18xx
            lpc_otg_request(core, &ipc);
#endif //LPC11Uxx
            break;
#endif //MONOLITH_USB
#if (LPC_EEPROM_DRIVER)
        case HAL_EEPROM:
            lpc_eep_request(core, &ipc);
            break;
#endif //LPC_EEPROM_DRIVER
#if (LPC_SDMMC_DRIVER)
        case HAL_SDMMC:
            lpc_sdmmc_request(core, &ipc);
            break;
#endif //LPC_SDMMC_DRIVER
        default:
            error(ERROR_NOT_SUPPORTED);
            break;
        }
        ipc_write(&ipc);
    }
}

void lpc_core()
{
    CORE core;
    object_set_self(SYS_OBJ_CORE);

    //manage pools
#if defined(LPC18xx)
    //Local SRAM1
    svc_add_pool(SRAM1_BASE, SRAM1_SIZE);

#ifdef AHB_SRAM_ONE_PIECE
    //AHB SRAM1 + SRAM2/ETB SRAM. Shared with USB
#if (LPC_USB_USE_BOTH)
    svc_add_pool(AHB_SRAM1_BASE, AHB_SRAM1_SIZE + AHB_SRAM2_SIZE - 2048 * 2);
#else
    svc_add_pool(AHB_SRAM1_BASE, AHB_SRAM1_SIZE + AHB_SRAM2_SIZE - 2048);
#endif //LPC_USB_USE_BOTH
#else
    //AHB SRAM1
    svc_add_pool(AHB_SRAM1_BASE, AHB_SRAM1_SIZE);

    //AHB SRAM2/ETB SRAM. Shared with USB
#if (LPC_USB_USE_BOTH)
    svc_add_pool(AHB_SRAM2_BASE, AHB_SRAM2_SIZE - 2048 * 2);
#else
    svc_add_pool(AHB_SRAM2_BASE, AHB_SRAM2_SIZE - 2048);
#endif //LPC_USB_USE_BOTH
#endif //AHB_SRAM_ONE_PIECE

#endif //LPC18xx

    lpc_power_init(&core);
    lpc_pin_init(&core);
    lpc_timer_init(&core);
    lpc_uart_init(&core);
#if (LPC_I2C_DRIVER)
    lpc_i2c_init(&core);
#endif //LPC_I2C_DRIVER
#if (LPC_SDMMC_DRIVER)
    lpc_sdmmc_init(&core);
#endif //LPC_SDMMC_DRIVER
#if (MONOLITH_USB)
#ifdef LPC11Uxx
    lpc_usb_init(&core);
#else //LPC18xx
    lpc_otg_init(&core);
#endif //LPC11Uxx
#endif //MONOLITH_USB
#if (LPC_EEPROM_DRIVER)
#ifdef LPC18xx
    lpc_eep_init(&core);
#endif //LPC18xx
#endif //LPC_EEPROM_DRIVER

    lpc_core_loop(&core);
}
