/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef STM32_GPIO_H
#define STM32_GPIO_H

#include "../../userspace/process.h"
#include "../../userspace/core/stm32.h"

typedef enum {
    PIN_MODE_OUT = 0,
    PIN_MODE_IN_FLOAT,
    PIN_MODE_IN_PULLUP,
    PIN_MODE_IN_PULLDOWN
} PIN_MODE;

typedef enum {
    A0 = 0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15,
    B0,     B1, B2, B3, B4, B5, B6, B7, B8, B9, B10, B11, B12, B13, B14, B15,
    C0,     C1, C2, C3, C4, C5, C6, C7, C8, C9, C10, C11, C12, C13, C14, C15,
    D0,     D1, D2, D3, D4, D5, D6, D7, D8, D9, D10, D11, D12, D13, D14, D15,
    E0,     E1, E2, E3, E4, E5, E6, E7, E8, E9, E10, E11, E12, E13, E14, E15,
    F0,     F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15,
    G0,     G1, G2, G3, G4, G5, G6, G7, G8, G9, G10, G11, G12, G13, G14, G15,
    H0,     H1, H2, H3, H4, H5, H6, H7, H8, H9, H10, H11, H12, H13, H14, H15,
    I0,     I1, I2, I3, I4, I5, I6, I7, I8, I9, I10, I11, I12, I13, I14, I15,
    J0,     J1, J2, J3, J4, J5, J6, J7, J8, J9, J10, J11, J12, J13, J14, J15,
    K0,     K1, K2, K3, K4, K5, K6, K7, K8, K9, K10, K11, K12, K13, K14, K15
} PIN;

#define GPIO_PORT(pin)                                          (pin / 16)
#define GPIO_PIN(pin)                                           (pin & 15)

typedef GPIO_TypeDef* GPIO_TypeDef_P;
extern const GPIO_TypeDef_P GPIO[];

//stm32 specific, for internal use for drivers
void gpio_disable_jtag();

#if defined(STM32F1)

typedef enum {
    GPIO_MODE_INPUT_ANALOG = 0x0,
    GPIO_MODE_INPUT_FLOAT = 0x4,
    GPIO_MODE_INPUT_PULL = 0x8,
    GPIO_MODE_OUTPUT_PUSH_PULL_10MHZ = 0x1,
    GPIO_MODE_OUTPUT_PUSH_PULL_2MHZ = 0x2,
    GPIO_MODE_OUTPUT_PUSH_PULL_50MHZ = 0x3,
    GPIO_MODE_OUTPUT_OPEN_DRAIN_10MHZ = 0x5,
    GPIO_MODE_OUTPUT_OPEN_DRAIN_2MHZ = 0x6,
    GPIO_MODE_OUTPUT_OPEN_DRAIN_50MHZ = 0x7,
    GPIO_MODE_OUTPUT_AF_PUSH_PULL_10MHZ = 0x9,
    GPIO_MODE_OUTPUT_AF_PUSH_PULL_2MHZ = 0xa,
    GPIO_MODE_OUTPUT_AF_PUSH_PULL_50MHZ = 0xb,
    GPIO_MODE_OUTPUT_AF_OPEN_DRAIN_10MHZ = 0xd,
    GPIO_MODE_OUTPUT_AF_OPEN_DRAIN_2MHZ = 0xe,
    GPIO_MODE_OUTPUT_AF_OPEN_DRAIN_50MHZ = 0xf
}GPIO_MODE;

void afio_remap();
void afio_unmap();

void gpio_enable_pin_system(PIN pin, GPIO_MODE mode, bool pullup);
#endif

void gpio_enable_pin(PIN pin, PIN_MODE mode);
void gpio_disable_pin(PIN pin);
void gpio_set_pin(PIN pin, bool set);
bool gpio_get_pin(PIN pin);

#endif // STM32_GPIO_H
