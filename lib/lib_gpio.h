/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef STM32_BITBANG_H
#define STM32_BITBANG_H

/*
        STM32 BitBang module. Set of helpers to use GPIO as bitbang. Generally made for soft extmem manager for МЭЛТ display
*/

#include "../../../userspace/sys.h"
#include "../../../userspace/cc_macro.h"
#include "stm32_config.h"
#include "stm32_gpio.h"

#if defined(STM32F1)
#define BITBANG_OUT                 0x3
#define BITBANG_IN                  0x4
#elif defined(STM32F2) || defined(STM32F4) || defined(STM32L0)
#define BITBANG_OUT                 0x1
#define BITBANG_IN                  0x0
#endif

__STATIC_INLINE void stm32_bitbang_enable_pin(PIN pin)
{
#if defined(STM32F1)
    ack(object_get(SYS_OBJ_CORE), STM32_GPIO_ENABLE_PIN_SYSTEM, pin, GPIO_MODE_OUTPUT_PUSH_PULL_50MHZ, false);
#elif defined(STM32F2) || defined(STM32F4) || defined(STM32L0)
    ack(object_get(SYS_OBJ_CORE), STM32_GPIO_ENABLE_PIN_SYSTEM, pin, GPIO_MODE_OUTPUT | GPIO_OT_PUSH_PULL | GPIO_SPEED_HIGH | GPIO_PUPD_NO_PULLUP, AF0);
#endif
}

__STATIC_INLINE void stm32_bitbang_enable_mask(GPIO_PORT_NUM port, unsigned int mask)
{
    unsigned int bit;
    unsigned int cur = mask;
    while (cur)
    {
        bit = 31 - __builtin_clz(cur);
        cur &= ~(1 << bit);
        stm32_bitbang_enable_pin(GPIO_MAKE_PIN(port, bit));
    }
}

__STATIC_INLINE void stm32_bitbang_set_pin(PIN pin)
{
#if defined(STM32F1) || defined (STM32L0)
    GPIO[GPIO_PORT(pin)]->BSRR = 1 << GPIO_PIN(pin);
#elif defined(STM32F2) || defined(STM32F4)
    GPIO[GPIO_PORT(pin)]->BSRRL = 1 << GPIO_PIN(pin);
#endif
}

__STATIC_INLINE void stm32_bitbang_set_mask(GPIO_PORT_NUM port, unsigned int mask)
{
#if defined(STM32F1) || defined (STM32L0)
    GPIO[port]->BSRR = mask;
#elif defined(STM32F2) || defined(STM32F4)
    GPIO[port]->BSRRL = mask;
#endif
}

__STATIC_INLINE void stm32_bitbang_reset_pin(PIN pin)
{
#if defined(STM32F1) || defined (STM32L0)
    GPIO[GPIO_PORT(pin)]->BRR = 1 << GPIO_PIN(pin);
#elif defined(STM32F2) || defined(STM32F4)
    GPIO[GPIO_PORT(pin)]->BSRRH = 1 << GPIO_PIN(pin);
#endif
}

__STATIC_INLINE void stm32_bitbang_reset_mask(GPIO_PORT_NUM port, unsigned int mask)
{
#if defined(STM32F1) || defined (STM32L0)
    GPIO[port]->BRR = mask;
#elif defined(STM32F2) || defined(STM32F4)
    GPIO[port]->BSRRH = mask;
#endif
}

__STATIC_INLINE int stm32_bitbang_get_pin(PIN pin)
{
    return (GPIO[GPIO_PORT(pin)]->IDR >> GPIO_PIN(pin)) & 1;
}

__STATIC_INLINE int stm32_bitbang_get_mask(GPIO_PORT_NUM port, unsigned int mask)
{
    return GPIO[port]->IDR & mask;
}

__STATIC_INLINE void stm32_bitbang_set_data_out(GPIO_PORT_NUM port)
{

#if defined(STM32F1)
    GPIO[port]->CRL = 0x33333333;
#elif defined(STM32F2) || defined(STM32F4) || defined(STM32L0)
    GPIO[port]->MODER |= 0x5555;
#endif
}

__STATIC_INLINE void stm32_bitbang_set_data_in(GPIO_PORT_NUM port)
{

#if defined(STM32F1)
    GPIO[port]->CRL = 0x44444444;
#elif defined(STM32F2) || defined(STM32F4) || defined(STM32L0)
    GPIO[port]->MODER &= ~0xffff;
#endif
}

#endif // STM32_BITBANG_H
