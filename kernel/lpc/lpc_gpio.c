/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "../../userspace/lpc/lpc_driver.h"
#include "../../userspace/gpio.h"
#include "../../userspace/pin.h"
#include "../kerror.h"
#include "lpc_pin.h"
#include "sys_config.h"

#if defined(LPC11Uxx)
#define PIN_RAW(gpio)                   (gpio)

#define GPIO_I2C_MASK                   ((1 << PIO0_4)  | (1 << PIO0_5))
#define GPIO_AD_MASK                    ((1 << PIO0_11) | (1 << PIO0_12) | (1 << PIO0_16) | (1 << PIO0_22) | (1 << PIO0_23))
#define GPIO_MODE1_MASK                 ((1 << PIO0_0)  | (1 << PIO0_10) | (1 << PIO0_11) | (1 << PIO0_12) | (1 << PIO0_13) | (1 << PIO0_14) | (1 << PIO0_15))

#else

#define LPC_GPIO                        LPC_GPIO_PORT

static const uint16_t __GPIO_PIN[PIO_MAX] =
                                       {
                                        //PIO0
                                        P0_0,    P0_1,    P1_15,   P1_16,   P1_0,    P6_6,    P3_6,    P2_7,
                                        P1_1,    P1_2,    P1_3,    P1_4,    P1_17,   P1_18,   P2_10,   P1_20,
                                        PIO_MAX, PIO_MAX, PIO_MAX, PIO_MAX, PIO_MAX, PIO_MAX, PIO_MAX, PIO_MAX,
                                        PIO_MAX, PIO_MAX, PIO_MAX, PIO_MAX, PIO_MAX, PIO_MAX, PIO_MAX, PIO_MAX,
                                        //PIO1
                                        P1_7,    P1_8,    P1_9,    P1_10,   P1_11,   P1_12,   P1_13,   P1_14,
                                        P1_5,    P1_6,    P1_3,    P1_4,    P2_12,   P2_13,   P3_4,    P3_5,
                                        PIO_MAX, PIO_MAX, PIO_MAX, PIO_MAX, PIO_MAX, PIO_MAX, PIO_MAX, PIO_MAX,
                                        PIO_MAX, PIO_MAX, PIO_MAX, PIO_MAX, PIO_MAX, PIO_MAX, PIO_MAX, PIO_MAX,
                                        //PIO2
                                        P4_0,    P4_1,    P4_2,    P4_3,    P4_4,    P4_5,    P4_6,    P5_7,
                                        P6_12,   P5_0,    P5_1,    P5_2,    P5_3,    P5_4,    P5_5,    P5_6,
                                        PIO_MAX, PIO_MAX, PIO_MAX, PIO_MAX, PIO_MAX, PIO_MAX, PIO_MAX, PIO_MAX,
                                        PIO_MAX, PIO_MAX, PIO_MAX, PIO_MAX, PIO_MAX, PIO_MAX, PIO_MAX, PIO_MAX,
                                        //PIO3
                                        P6_1,    P6_2,    P6_3,    P6_4,    P6_5,    P6_9,    P6_10,   P6_11,
                                        P7_0,    P7_1,    P7_2,    P7_3,    P7_4,    P7_5,    P7_6,    P7_7,
                                        PIO_MAX, PIO_MAX, PIO_MAX, PIO_MAX, PIO_MAX, PIO_MAX, PIO_MAX, PIO_MAX,
                                        PIO_MAX, PIO_MAX, PIO_MAX, PIO_MAX, PIO_MAX, PIO_MAX, PIO_MAX, PIO_MAX,
                                        //PIO4
                                        P8_0,    P8_1,    P8_2,    P8_3,    P8_4,    P8_5,    P8_6,    P8_7,
                                        PA_1,    PA_2,    PA_3,    P9_6,    P9_0,    P9_1,    P9_2,    P9_3,
                                        PIO_MAX, PIO_MAX, PIO_MAX, PIO_MAX, PIO_MAX, PIO_MAX, PIO_MAX, PIO_MAX,
                                        PIO_MAX, PIO_MAX, PIO_MAX, PIO_MAX, PIO_MAX, PIO_MAX, PIO_MAX, PIO_MAX,
                                        //PIO5
                                        P2_0,    P2_1,    P2_2,    P2_3,    P2_4,    P2_5,    P2_6,    P2_8,
                                        P3_1,    P3_2,    P3_7,    P3_8,    P4_8,    P4_9,    P4_10,   P6_7,
                                        P6_8,    P9_4,    P9_5,    PA_4,    PB_0,    PB_1,    PB_2,    PB_3,
                                        PB_4,    PB_5,    PB_6,    PIO_MAX, PIO_MAX, PIO_MAX, PIO_MAX, PIO_MAX,
                                        //PIO6
                                        PC_1,    PC_2,    PC_3,    PC_4,    PC_5,    PC_6,    PC_7,    PC_8,
                                        PC_9,    PC_10,   PC_11,   PC_12,   PC_13,   PC_14,   PD_0,    PD_1,
                                        PD_2,    PD_3,    PD_4,    PD_5,    PD_6,    PD_7,    PD_8,    PD_9,
                                        PD_10,   PD_11,   PD_12,   PD_13,   PD_14,   PD_15,   PD_16,   PIO_MAX,
                                        //PIO7
                                        PE_0,    PE_1,    PE_2,    PE_3,    PE_4,    PE_5,    PE_6,    PE_7,
                                        PE_8,    PE_9,    PE_10,   PE_11,   PE_12,   PE_13,   PE_14,   PE_15,
                                        PF_1,    PF_2,    PF_3,    PF_5,    PF_6,    PF_7,    PF_8,    PF_9,
                                        PF_10,   PF_11,   PIO_MAX, PIO_MAX, PIO_MAX, PIO_MAX, PIO_MAX, PIO_MAX
                                       };

#define PIN_RAW(gpio)                   (__GPIO_PIN[(gpio)])
#endif

void lpc_gpio_enable_pin(GPIO gpio, GPIO_MODE mode)
{
    unsigned int param = 0;
#ifdef LPC11Uxx
    if (GPIO_PORT(gpio) == 0)
    {
        if ((1 << gpio) & GPIO_I2C_MASK)
            param = IOCON_PIO_I2CMODE_GPIO;
        else if ((1 << gpio) & GPIO_AD_MASK)
            param = IOCON_PIO_ADMODE;
        if ((1 << gpio) & GPIO_MODE1_MASK)
            param |= (1 << 0);
    }
    switch (mode)
    {
    case GPIO_MODE_IN_PULLUP:
        param |= IOCON_PIO_MODE_PULL_UP | IOCON_PIO_HYS;
        break;
    case GPIO_MODE_IN_PULLDOWN:
        param |= IOCON_PIO_MODE_PULL_DOWN | IOCON_PIO_HYS;
        break;
    case GPIO_MODE_IN_FLOAT:
        param |= IOCON_PIO_HYS;
        break;
    default:
        break;
    }
#else //LPC18xx

    if (GPIO_PORT(gpio) >= 5)
        param = (4 << 0);

    switch (mode)
    {
    case GPIO_MODE_IN_PULLUP:
        param |= SCU_SFS_EZI | SCU_SFS_ZIF;
        break;
    case GPIO_MODE_IN_PULLDOWN:
        param |= SCU_SFS_EPUN | SCU_SFS_EPD | SCU_SFS_EZI | SCU_SFS_ZIF;
        break;
    case GPIO_MODE_IN_FLOAT:
        param |= SCU_SFS_EPUN | SCU_SFS_EZI | SCU_SFS_ZIF;
        break;
    default:
        break;
    }
#endif //LPC11Uxx

    lpc_pin_enable(PIN_RAW(gpio), param);
    if (mode == GPIO_MODE_OUT)
        LPC_GPIO->DIR[GPIO_PORT(gpio)] |= 1 << GPIO_PIN(gpio);
    else
        LPC_GPIO->DIR[GPIO_PORT(gpio)] &= ~(1 << GPIO_PIN(gpio));
}

static inline void lpc_gpio_enable_mask(unsigned int port, GPIO_MODE mode, unsigned int mask)
{
    unsigned int bit;
    unsigned int cur = mask;
    while (cur)
    {
        bit = 31 - __builtin_clz(cur);
        cur &= ~(1 << bit);
        gpio_enable_pin(GPIO_MAKE_PIN(port, bit), mode);
    }
}

void lpc_gpio_disable_pin(GPIO gpio)
{
    LPC_GPIO->DIR[GPIO_PORT(gpio)] &= ~(1 << GPIO_PIN(gpio));
    lpc_pin_disable(PIN_RAW(gpio));
}

static inline void lpc_gpio_disable_mask(unsigned int port, unsigned int mask)
{
    unsigned int bit;
    unsigned int cur = mask;
    while (cur)
    {
        bit = 31 - __builtin_clz(cur);
        cur &= ~(1 << bit);
        gpio_disable_pin(GPIO_MAKE_PIN(port, bit));
    }
}

void lpc_gpio_set_pin(GPIO gpio)
{
    LPC_GPIO->B[gpio] = 1;
}

static inline void lpc_gpio_set_mask(unsigned int port, unsigned int mask)
{
    LPC_GPIO->SET[port] = mask;
}

void lpc_gpio_reset_pin(GPIO gpio)
{
    LPC_GPIO->B[gpio] = 0;
}

static inline void lpc_gpio_reset_mask(unsigned int port, unsigned int mask)
{
    LPC_GPIO->CLR[port] = mask;
}

static inline bool lpc_gpio_get_pin(GPIO gpio)
{
    return LPC_GPIO->B[gpio] & 1;
}

static inline unsigned int lpc_gpio_get_mask(unsigned port, unsigned int mask)
{
    return LPC_GPIO->PIN[port] & mask;
}

static inline void lpc_gpio_set_data_out(unsigned int port, unsigned int wide)
{
    LPC_GPIO->DIR[port] |= (1 << wide) - 1;
}

static inline void lpc_gpio_set_data_in(unsigned int port, unsigned int wide)
{
    LPC_GPIO->DIR[port] &= ~((1 << wide) - 1);
}

void lpc_gpio_request(IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_GPIO_ENABLE_PIN:
        lpc_gpio_enable_pin((GPIO)ipc->param1, (GPIO_MODE)ipc->param2);
        break;
    case IPC_GPIO_DISABLE_PIN:
        lpc_gpio_disable_pin((GPIO)ipc->param1);
        break;
    case IPC_GPIO_SET_PIN:
        lpc_gpio_set_pin((GPIO)ipc->param1);
        break;
    case IPC_GPIO_SET_MASK:
        lpc_gpio_set_mask(ipc->param1, ipc->param2);
        break;
    case IPC_GPIO_RESET_PIN:
        lpc_gpio_reset_pin((GPIO)ipc->param1);
        break;
    case IPC_GPIO_RESET_MASK:
        lpc_gpio_reset_mask(ipc->param1, ipc->param2);
        break;
    case IPC_GPIO_GET_PIN:
        ipc->param2 = lpc_gpio_get_pin((GPIO)ipc->param1);
        break;
    case IPC_GPIO_GET_MASK:
        ipc->param2 = lpc_gpio_get_mask(ipc->param1, ipc->param2);
        break;
    case IPC_GPIO_SET_DATA_OUT:
        lpc_gpio_set_data_out(ipc->param1, ipc->param2);
        break;
    case IPC_GPIO_SET_DATA_IN:
        lpc_gpio_set_data_in(ipc->param1, ipc->param2);
        break;
    default:
        kerror(ERROR_NOT_SUPPORTED);
        break;
    }
}
