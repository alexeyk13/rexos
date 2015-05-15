/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
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
#include "lpc_usb.h"
#include "lpc_eep.h"


typedef struct _CORE {
    POWER_DRV power;
    TIMER_DRV timer;
#if (MONOLITH_UART)
    UART_DRV uart;
#endif
#if (MONOLITH_I2C)
    I2C_DRV i2c;
#endif
#if (MONOLITH_USB)
    USB_DRV usb;
#endif
#if (LPC_EEPROM_DRIVER)
    EEP eep;
#endif
}CORE;


#endif // LPC_CORE_PRIVATE_H
