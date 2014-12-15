/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef LPC_GPIO_H
#define LPC_GPIO_H

#include <stdbool.h>
#include "../../../userspace/cc_macro.h"
#include "../../../userspace/sys.h"
#include "../../../userspace/gpio.h"

typedef enum {
    LPC_GPIO_ENABLE_PIN_SYSTEM = GPIO_HAL_MAX
} LPC_GPIO_IPCS;

typedef enum {
    PIO0_0 = 0, PIO0_1,  PIO0_2,  PIO0_3,  PIO0_4,  PIO0_5,  PIO0_6,  PIO0_7,
    PIO0_8,     PIO0_9,  PIO0_10, PIO0_11, PIO0_12, PIO0_13, PIO0_14, PIO0_15,
    PIO0_16,    PIO0_17, PIO0_18, PIO0_19, PIO0_20, PIO0_21, PIO0_22, PIO0_23,
    PIO0_24,    PIO0_25, PIO0_26, PIO0_27, PIO0_28, PIO0_29, PIO0_30, PIO0_31,
    PIO1_0,     PIO1_1,  PIO1_2,  PIO1_3,  PIO1_4,  PIO1_5,  PIO1_6,  PIO1_7,
    PIO1_8,     PIO1_9,  PIO1_10, PIO1_11, PIO1_12, PIO1_13, PIO1_14, PIO1_15,
    PIO1_16,    PIO1_17, PIO1_18, PIO1_19, PIO1_20, PIO1_21, PIO1_22, PIO1_23,
    PIO1_24,    PIO1_25, PIO1_26, PIO1_27, PIO1_28, PIO1_29, PIO1_30, PIO1_31,
    //only for service calls
    PIN_DEFAULT,
    PIN_UNUSED
} PIN;

#define GPIO_MODE_MASK                          0x7ff
#define GPIO_MODE_OUT                           (1 << 11)

//Ignore AF, value is set in mode
#define AF_IGNORE                               0
//PIN as GPIO
#define AF_DEFAULT                              1
//PIN as UART IO pins. UART module must be enabled
#define AF_UART                                 2
//PIN as I2C IO pins. I2C module must be enabled
#define AF_I2C                                  3
//PIN as FAST I2C IO pins. I2C module must be enabled
#define AF_FAST_I2C                             4

void lpc_gpio_init();
bool lpc_gpio_request(IPC* ipc);

__STATIC_INLINE unsigned int lpc_gpio_request_inside(void* unused, unsigned int cmd, unsigned int param1, unsigned int param2, unsigned int param3)
{
    IPC ipc;
    ipc.cmd = cmd;
    ipc.param1 = param1;
    ipc.param2 = param2;
    ipc.param3 = param3;
    lpc_gpio_request(&ipc);
    return ipc.param1;
}

#endif // LPC_GPIO_H
