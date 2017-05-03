/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef LPC_DRIVER_H
#define LPC_DRIVER_H

#include "lpc.h"
#include "lpc_config.h"
#include "../process.h"
#include "../power.h"

extern const REX __LPC_CORE;

//------------------------------------------------- GPIO ---------------------------------------------------------------------
typedef enum {
    PIO0_0 = 0, PIO0_1,  PIO0_2,  PIO0_3,  PIO0_4,  PIO0_5,  PIO0_6,  PIO0_7,
    PIO0_8,     PIO0_9,  PIO0_10, PIO0_11, PIO0_12, PIO0_13, PIO0_14, PIO0_15,
    PIO0_16,    PIO0_17, PIO0_18, PIO0_19, PIO0_20, PIO0_21, PIO0_22, PIO0_23,
    PIO0_24,    PIO0_25, PIO0_26, PIO0_27, PIO0_28, PIO0_29, PIO0_30, PIO0_31,
    PIO1_0,     PIO1_1,  PIO1_2,  PIO1_3,  PIO1_4,  PIO1_5,  PIO1_6,  PIO1_7,
    PIO1_8,     PIO1_9,  PIO1_10, PIO1_11, PIO1_12, PIO1_13, PIO1_14, PIO1_15,
    PIO1_16,    PIO1_17, PIO1_18, PIO1_19, PIO1_20, PIO1_21, PIO1_22, PIO1_23,
    PIO1_24,    PIO1_25, PIO1_26, PIO1_27, PIO1_28, PIO1_29, PIO1_30, PIO1_31,
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
    PIO_MAX
} GPIO;

#if defined(LPC11Uxx)
#define PIN_MAX                                 PIO2_0

#else
typedef enum {
    P0_0 = 0, P0_1,  P0_2,  P0_3,  P0_4,  P0_5,  P0_6,  P0_7,
    P0_8,     P0_9,  P0_10, P0_11, P0_12, P0_13, P0_14, P0_15,
    P0_16,    P0_17, P0_18, P0_19, P0_20, P0_21, P0_22, P0_23,
    P0_24,    P0_25, P0_26, P0_27, P0_28, P0_29, P0_30, P0_31,
    P1_0,     P1_1,  P1_2,  P1_3,  P1_4,  P1_5,  P1_6,  P1_7,
    P1_8,     P1_9,  P1_10, P1_11, P1_12, P1_13, P1_14, P1_15,
    P1_16,    P1_17, P1_18, P1_19, P1_20, P1_21, P1_22, P1_23,
    P1_24,    P1_25, P1_26, P1_27, P1_28, P1_29, P1_30, P1_31,
    P2_0,     P2_1,  P2_2,  P2_3,  P2_4,  P2_5,  P2_6,  P2_7,
    P2_8,     P2_9,  P2_10, P2_11, P2_12, P2_13, P2_14, P2_15,
    P2_16,    P2_17, P2_18, P2_19, P2_20, P2_21, P2_22, P2_23,
    P2_24,    P2_25, P2_26, P2_27, P2_28, P2_29, P2_30, P2_31,
    P3_0,     P3_1,  P3_2,  P3_3,  P3_4,  P3_5,  P3_6,  P3_7,
    P3_8,     P3_9,  P3_10, P3_11, P3_12, P3_13, P3_14, P3_15,
    P3_16,    P3_17, P3_18, P3_19, P3_20, P3_21, P3_22, P3_23,
    P3_24,    P3_25, P3_26, P3_27, P3_28, P3_29, P3_30, P3_31,
    P4_0,     P4_1,  P4_2,  P4_3,  P4_4,  P4_5,  P4_6,  P4_7,
    P4_8,     P4_9,  P4_10, P4_11, P4_12, P4_13, P4_14, P4_15,
    P4_16,    P4_17, P4_18, P4_19, P4_20, P4_21, P4_22, P4_23,
    P4_24,    P4_25, P4_26, P4_27, P4_28, P4_29, P4_30, P4_31,
    P5_0,     P5_1,  P5_2,  P5_3,  P5_4,  P5_5,  P5_6,  P5_7,
    P5_8,     P5_9,  P5_10, P5_11, P5_12, P5_13, P5_14, P5_15,
    P5_16,    P5_17, P5_18, P5_19, P5_20, P5_21, P5_22, P5_23,
    P5_24,    P5_25, P5_26, P5_27, P5_28, P5_29, P5_30, P5_31,
    P6_0,     P6_1,  P6_2,  P6_3,  P6_4,  P6_5,  P6_6,  P6_7,
    P6_8,     P6_9,  P6_10, P6_11, P6_12, P6_13, P6_14, P6_15,
    P6_16,    P6_17, P6_18, P6_19, P6_20, P6_21, P6_22, P6_23,
    P6_24,    P6_25, P6_26, P6_27, P6_28, P6_29, P6_30, P6_31,
    P7_0,     P7_1,  P7_2,  P7_3,  P7_4,  P7_5,  P7_6,  P7_7,
    P7_8,     P7_9,  P7_10, P7_11, P7_12, P7_13, P7_14, P7_15,
    P7_16,    P7_17, P7_18, P7_19, P7_20, P7_21, P7_22, P7_23,
    P7_24,    P7_25, P7_26, P7_27, P7_28, P7_29, P7_30, P7_31,
    P8_0,     P8_1,  P8_2,  P8_3,  P8_4,  P8_5,  P8_6,  P8_7,
    P8_8,     P8_9,  P8_10, P8_11, P8_12, P8_13, P8_14, P8_15,
    P8_16,    P8_17, P8_18, P8_19, P8_20, P8_21, P8_22, P8_23,
    P8_24,    P8_25, P8_26, P8_27, P8_28, P8_29, P8_30, P8_31,
    P9_0,     P9_1,  P9_2,  P9_3,  P9_4,  P9_5,  P9_6,  P9_7,
    P9_8,     P9_9,  P9_10, P9_11, P9_12, P9_13, P9_14, P9_15,
    P9_16,    P9_17, P9_18, P9_19, P9_20, P9_21, P9_22, P9_23,
    P9_24,    P9_25, P9_26, P9_27, P9_28, P9_29, P9_30, P9_31,
    PA_0,     PA_1,  PA_2,  PA_3,  PA_4,  PA_5,  PA_6,  PA_7,
    PA_8,     PA_9,  PA_10, PA_11, PA_12, PA_13, PA_14, PA_15,
    PA_16,    PA_17, PA_18, PA_19, PA_20, PA_21, PA_22, PA_23,
    PA_24,    PA_25, PA_26, PA_27, PA_28, PA_29, PA_30, PA_31,
    PB_0,     PB_1,  PB_2,  PB_3,  PB_4,  PB_5,  PB_6,  PB_7,
    PB_8,     PB_9,  PB_10, PB_11, PB_12, PB_13, PB_14, PB_15,
    PB_16,    PB_17, PB_18, PB_19, PB_20, PB_21, PB_22, PB_23,
    PB_24,    PB_25, PB_26, PB_27, PB_28, PB_29, PB_30, PB_31,
    PC_0,     PC_1,  PC_2,  PC_3,  PC_4,  PC_5,  PC_6,  PC_7,
    PC_8,     PC_9,  PC_10, PC_11, PC_12, PC_13, PC_14, PC_15,
    PC_16,    PC_17, PC_18, PC_19, PC_20, PC_21, PC_22, PC_23,
    PC_24,    PC_25, PC_26, PC_27, PC_28, PC_29, PC_30, PC_31,
    PD_0,     PD_1,  PD_2,  PD_3,  PD_4,  PD_5,  PD_6,  PD_7,
    PD_8,     PD_9,  PD_10, PD_11, PD_12, PD_13, PD_14, PD_15,
    PD_16,    PD_17, PD_18, PD_19, PD_20, PD_21, PD_22, PD_23,
    PD_24,    PD_25, PD_26, PD_27, PD_28, PD_29, PD_30, PD_31,
    PE_0,     PE_1,  PE_2,  PE_3,  PE_4,  PE_5,  PE_6,  PE_7,
    PE_8,     PE_9,  PE_10, PE_11, PE_12, PE_13, PE_14, PE_15,
    PE_16,    PE_17, PE_18, PE_19, PE_20, PE_21, PE_22, PE_23,
    PE_24,    PE_25, PE_26, PE_27, PE_28, PE_29, PE_30, PE_31,
    PF_0,     PF_1,  PF_2,  PF_3,  PF_4,  PF_5,  PF_6,  PF_7,
    PF_8,     PF_9,  PF_10, PF_11, PF_12, PF_13, PF_14, PF_15,
    PF_16,    PF_17, PF_18, PF_19, PF_20, PF_21, PF_22, PF_23,
    PF_24,    PF_25, PF_26, PF_27, PF_28, PF_29, PF_30, PF_31,
    CLK_0 = 0x300, CLK_1, CLK_2, CLK_3,
    PIN_MAX
} PIN;
#endif

#define GPIO_PORT(pin)                          ((pin) >> 5)
#define GPIO_PIN(pin)                           ((pin) & 0x1f)
#define GPIO_MAKE_PIN(port, pin)                (((port) << 5)  + (pin))


//------------------------------------------------ Timer ---------------------------------------------------------------------

typedef enum {
#ifdef LPC11Uxx
    TC16B0 = 0,
    TC16B1,
    TC32B0,
    TC32B1,
#else //LPC18xx
    TIMER0,
    TIMER1,
    TIMER2,
    TIMER3,
    SCT,
    PWM0,
    PWM1,
    PWM2,
#endif //LPC11uxx
    TIMER_MAX
} TIMER;

typedef enum {
    TIMER_CHANNEL0 = 0,
    TIMER_CHANNEL1,
    TIMER_CHANNEL2,
    TIMER_CHANNEL3,
    TIMER_CHANNEL4,
    TIMER_CHANNEL5,
    TIMER_CHANNEL6,
    TIMER_CHANNEL7,
    TIMER_CHANNEL8,
    TIMER_CHANNEL9,
    TIMER_CHANNEL10,
    TIMER_CHANNEL11,
    TIMER_CHANNEL12,
    TIMER_CHANNEL13,
    TIMER_CHANNEL14,
    TIMER_CHANNEL15,
} TIMER_CHANNEL;

#define TIMER_CHANNEL_INVALID                        0xff

#define TIMER_MODE_CHANNEL_POS                       16
#define TIMER_MODE_CHANNEL_MASK                      (0xf << TIMER_MODE_CHANNEL_POS)

//------------------------------------------------ Power ---------------------------------------------------------------------
typedef enum {
    LPC_POWER_GET_RESET_REASON = POWER_MAX,
    LPC_POWER_MAX
} LPC_POWER_IPCS;

typedef enum {
    RESET_REASON_POWERON = 0,
    RESET_REASON_EXTERNAL,
    RESET_REASON_WATCHDOG,
    RESET_REASON_BROWNOUT,
    RESET_REASON_SOFTWARE,
    RESET_REASON_UNKNOWN
} RESET_REASON;

//-------------------------------------------------- I2C ---------------------------------------------------------------------
typedef enum {
    I2C_0,
    I2C_1
} I2C_PORT;

//------------------------------------------------- UART ---------------------------------------------------------------------
typedef enum {
    UART_0 = 0,
    UART_1,
    UART_2,
    UART_3,
    UART_4,
    UART_5,
    UART_6,
    UART_7,
    UART_MAX
}UART_PORT;

#endif // LPC_DRIVER_H
