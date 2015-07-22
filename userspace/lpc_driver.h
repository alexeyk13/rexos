/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef LPC_DRIVER_H
#define LPC_DRIVER_H

#include "ipc.h"
#include "uart.h"

extern const REX __LPC_CORE;

//------------------------------------------------- GPIO ---------------------------------------------------------------------
typedef enum {
    LPC_GPIO_ENABLE_PIN = IPC_USER,
    LPC_GPIO_DISABLE_PIN,
} LPC_GPIO_IPCS;

typedef enum {
    PIO0_0 = 0, PIO0_1,  PIO0_2,  PIO0_3,  PIO0_4,  PIO0_5,  PIO0_6,  PIO0_7,
    PIO0_8,     PIO0_9,  PIO0_10, PIO0_11, PIO0_12, PIO0_13, PIO0_14, PIO0_15,
    PIO0_16,    PIO0_17, PIO0_18, PIO0_19, PIO0_20, PIO0_21, PIO0_22, PIO0_23,
    PIO0_24,    PIO0_25, PIO0_26, PIO0_27, PIO0_28, PIO0_29, PIO0_30, PIO0_31,
    PIO1_0,     PIO1_1,  PIO1_2,  PIO1_3,  PIO1_4,  PIO1_5,  PIO1_6,  PIO1_7,
    PIO1_8,     PIO1_9,  PIO1_10, PIO1_11, PIO1_12, PIO1_13, PIO1_14, PIO1_15,
    PIO1_16,    PIO1_17, PIO1_18, PIO1_19, PIO1_20, PIO1_21, PIO1_22, PIO1_23,
    PIO1_24,    PIO1_25, PIO1_26, PIO2_17, PIO1_28, PIO1_29, PIO1_30, PIO1_31,
    PIO2_0,     PIO2_1,  PIO2_2,  PIO2_3,  PIO2_4,  PIO2_5,  PIO2_6,  PIO2_7,
    PIO2_8,     PIO2_9,  PIO2_10, PIO2_11, PIO2_12, PIO2_13, PIO2_14, PIO2_15,
    PIO2_16,    PIO2_17, PIO2_18, PIO2_19, PIO2_20, PIO2_21, PIO2_22, PIO2_23,
    PIO2_24,    PIO2_25, PIO2_26, PIO2_27, PIO2_28, PIO2_29, PIO2_30, PIO2_31,
    PIO3_0,     PIO3_1,  PIO3_2,  PIO3_3,  PIO3_4,  PIO3_5,  PIO3_6,  PIO3_7,
    PIO3_8,     PIO3_9,  PIO3_10, PIO3_11, PIO3_12, PIO3_13, PIO3_14, PIO3_15,
    PIO3_16,    PIO3_17, PIO3_18, PIO3_19, PIO3_20, PIO3_21, PIO3_22, PIO3_23,
    PIO3_24,    PIO3_25, PIO3_26, PIO3_27, PIO3_28, PIO3_29, PIO3_30, PIO3_31,
    PIO4_0,     PIO4_1,  PIO4_2,  PIO4_3,  PIO4_4,  PIO4_5,  PIO4_6,  PIO4_7,
    PIO4_8,     PIO4_9,  PIO4_10, PIO4_11, PIO4_12, PIO4_13, PIO4_14, PIO4_15,
    PIO4_16,    PIO4_17, PIO4_18, PIO4_19, PIO4_20, PIO4_21, PIO4_22, PIO4_23,
    PIO4_24,    PIO4_25, PIO4_26, PIO4_27, PIO4_28, PIO4_29, PIO4_30, PIO4_31,
    PIO5_0,     PIO5_1,  PIO5_2,  PIO5_3,  PIO5_4,  PIO5_5,  PIO5_6,  PIO5_7,
    PIO5_8,     PIO5_9,  PIO5_10, PIO5_11, PIO5_12, PIO5_13, PIO5_14, PIO5_15,
    PIO5_16,    PIO5_17, PIO5_18, PIO5_19, PIO5_20, PIO5_21, PIO5_22, PIO5_23,
    PIO5_24,    PIO5_25, PIO5_26, PIO5_27, PIO5_28, PIO5_29, PIO5_30, PIO5_31,
    PIO6_0,     PIO6_1,  PIO6_2,  PIO6_3,  PIO6_4,  PIO6_5,  PIO6_6,  PIO6_7,
    PIO6_8,     PIO6_9,  PIO6_10, PIO6_11, PIO6_12, PIO6_13, PIO6_14, PIO6_15,
    PIO6_16,    PIO6_17, PIO6_18, PIO6_19, PIO6_20, PIO6_21, PIO6_22, PIO6_23,
    PIO6_24,    PIO6_25, PIO6_26, PIO6_27, PIO6_28, PIO6_29, PIO6_30, PIO6_31,
    PIO7_0,     PIO7_1,  PIO7_2,  PIO7_3,  PIO7_4,  PIO7_5,  PIO7_6,  PIO7_7,
    PIO7_8,     PIO7_9,  PIO7_10, PIO7_11, PIO7_12, PIO7_13, PIO7_14, PIO7_15,
    PIO7_16,    PIO7_17, PIO7_18, PIO7_19, PIO7_20, PIO7_21, PIO7_22, PIO7_23,
    PIO7_24,    PIO7_25, PIO7_26, PIO7_27, PIO7_28, PIO7_29, PIO7_30, PIO7_31,
    //only for service calls
    PIN_DEFAULT,
    PIN_UNUSED
} PIN;

#define LPC_GPIO_MODE_MASK                      0x7ff
#define LPC_GPIO_MODE_OUT                       (1 << 11)

#define GPIO_PORT(pin)                          ((pin) >> 5)
#define GPIO_PIN(pin)                           ((pin) & 0x1f)
#define GPIO_MAKE_PIN(port, pin)                (((port) << 5)  + (pin))

//------------------------------------------------ Timer ---------------------------------------------------------------------

typedef enum {
    TC16B0 = 0,
    TC16B1,
    TC32B0,
    TC32B1,
    TIMER_MAX
} TIMER;

#define TIMER_CHANNEL0                               0
#define TIMER_CHANNEL1                               1
#define TIMER_CHANNEL2                               2
#define TIMER_CHANNEL3                               3
#define TIMER_CHANNEL_INVALID                        0xff

#define TIMER_MODE_CHANNEL_POS                       16
#define TIMER_MODE_CHANNEL_MASK                      (3 << TIMER_MODE_CHANNEL_POS)

#define TIMER_MODE_CHANNEL0                          (TIMER_CHANNEL0 << TIMER_CHANNEL_POS)
#define TIMER_MODE_CHANNEL1                          (TIMER_CHANNEL1 << TIMER_CHANNEL_POS)
#define TIMER_MODE_CHANNEL2                          (TIMER_CHANNEL2 << TIMER_CHANNEL_POS)
#define TIMER_MODE_CHANNEL3                          (TIMER_CHANNEL3 << TIMER_CHANNEL_POS)

//------------------------------------------------ Power ---------------------------------------------------------------------
typedef enum {
    LPC_POWER_GET_SYSTEM_CLOCK = IPC_USER,
    LPC_POWER_UPDATE_CLOCK,
    LPC_POWER_GET_RESET_REASON,
    LPC_POWER_USB_ON,
    LPC_POWER_USB_OFF
} LPC_POWER_IPCS;

typedef enum {
    RESET_REASON_POWERON = 0,
    RESET_REASON_EXTERNAL,
    RESET_REASON_WATCHDOG,
    RESET_REASON_BROWNOUT,
    RESET_REASON_SOFTWARE,
    RESET_REASON_UNKNOWN
} RESET_REASON;

#define PLL_LOCK_TIMEOUT                            10000

//------------------------------------------------- UART ---------------------------------------------------------------------

typedef enum {
    UART_0 = 0,
    UART_1,
    UART_2,
    UART_3,
    UART_4,
    UART_MAX
}UART_PORT;

//-------------------------------------------------- I2C ---------------------------------------------------------------------

typedef enum {
    I2C_0,
    I2C_1
} I2C_PORT;

#define I2C_MASTER                  (1 << 16)
#define I2C_SLAVE                   (0 << 16)

#define I2C_NORMAL_SPEED            (0 << 17)
#define I2C_FAST_SPEED              (1 << 17)

//size of address. If 0, no Rs condition will be used. MSB goes first
#define I2C_ADDR_SIZE_POS           0
#define I2C_ADDR_SIZE_MASK          (0xf << 0)

//size of len. Used for some smartcards IO
#define I2C_LEN_SIZE_POS            8
#define I2C_LEN_SIZE_MASK           (0xf << 8)

#endif // LPC_DRIVER_H
