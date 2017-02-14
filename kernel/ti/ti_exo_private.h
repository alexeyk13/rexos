/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef TI_EXO_PRIVATE_H
#define TI_EXO_PRIVATE_H

#include "ti_power.h"
#include "ti_pin.h"
#include "ti_timer.h"
#include "ti_uart.h"
#include "ti_rf.h"
#include "ti_config.h"

typedef struct _EXO {
    POWER_DRV power;
    PIN_DRV pin;
    TIMER_DRV timer;
#if (TI_UART)
    UART_DRV uart;
#endif //TI_UART
#if (TI_RF)
    RF_DRV rf;
#endif //TI_RF
} EXO;


#endif // TI_EXO_PRIVATE_H
