/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2016, Alexey Kramarenko
    All rights reserved.
*/

#ifndef LPC_CORE_PRIVATE_H
#define LPC_CORE_PRIVATE_H

#include "lpc_config.h"
#include "lpc_core.h"
#include "lpc_power.h"
#include "lpc_timer.h"

#include "lpc_uart.h"
#include "lpc_i2c.h"
#ifdef LPC11Uxx
#include "lpc_usb.h"
#else //LPC18xx
#include "lpc_otg.h"
#endif //LPC11Uxx
#include "lpc_eep.h"
#include "lpc_sdmmc.h"


typedef struct _CORE {
    POWER_DRV power;
    TIMER_DRV timer;
    UART_DRV uart;
#if (LPC_I2C_DRIVER)
    I2C_DRV i2c;
#endif
#if (MONOLITH_USB)
#ifdef LPC11Uxx
    USB_DRV usb;
#else //LPC18xx
    OTG_DRV otg;
#endif //LPC11Uxx
#endif
#if (LPC_SDMMC_DRIVER)
    SDMMC_DRV sdmmc;
#endif
}CORE;


#endif // LPC_CORE_PRIVATE_H
