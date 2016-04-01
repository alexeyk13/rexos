/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2016, Alexey Kramarenko
    All rights reserved.
*/

#include "../gpio.h"
#include "stm32_driver.h"

void gpio_enable_pin(unsigned int pin, GPIO_MODE mode)
{
#if defined(STM32F1)
    switch (mode)
    {
    case GPIO_MODE_OUT:
        ack(object_get(SYS_OBJ_CORE), HAL_REQ(HAL_PIN, STM32_GPIO_ENABLE_PIN), pin, STM32_GPIO_MODE_OUTPUT_PUSH_PULL_50MHZ, false);
        break;
    case GPIO_MODE_IN_FLOAT:
        ack(object_get(SYS_OBJ_CORE), HAL_REQ(HAL_PIN, STM32_GPIO_ENABLE_PIN), pin, STM32_GPIO_MODE_INPUT_FLOAT, false);
        break;
    case GPIO_MODE_IN_PULLUP:
        ack(object_get(SYS_OBJ_CORE), HAL_REQ(HAL_PIN, STM32_GPIO_ENABLE_PIN), pin, STM32_GPIO_MODE_INPUT_PULL, true);
        break;
    case GPIO_MODE_IN_PULLDOWN:
        ack(object_get(SYS_OBJ_CORE), HAL_REQ(HAL_PIN, STM32_GPIO_ENABLE_PIN), pin, STM32_GPIO_MODE_INPUT_PULL, false);
        break;
    }
#elif defined(STM32F2) || defined(STM32F4) || defined(STM32L0)
    switch (mode)
    {
    case GPIO_MODE_OUT:
        ack(object_get(SYS_OBJ_CORE), HAL_REQ(HAL_PIN, STM32_GPIO_ENABLE_PIN), pin, STM32_GPIO_MODE_OUTPUT | GPIO_OT_PUSH_PULL | GPIO_SPEED_HIGH | GPIO_PUPD_NO_PULLUP, AF0);
        break;
    case GPIO_MODE_IN_FLOAT:
        ack(object_get(SYS_OBJ_CORE), HAL_REQ(HAL_PIN, STM32_GPIO_ENABLE_PIN), pin, STM32_GPIO_MODE_INPUT | GPIO_SPEED_HIGH | GPIO_PUPD_NO_PULLUP, AF0);
        break;
    case GPIO_MODE_IN_PULLUP:
        ack(object_get(SYS_OBJ_CORE), HAL_REQ(HAL_PIN, STM32_GPIO_ENABLE_PIN), pin, STM32_GPIO_MODE_INPUT | GPIO_SPEED_HIGH | GPIO_PUPD_PULLUP, AF0);
        break;
    case GPIO_MODE_IN_PULLDOWN:
        ack(object_get(SYS_OBJ_CORE), HAL_REQ(HAL_PIN, STM32_GPIO_ENABLE_PIN), pin, STM32_GPIO_MODE_INPUT | GPIO_SPEED_HIGH | GPIO_PUPD_PULLDOWN, AF0);
        break;
    }
#endif
}

void gpio_enable_mask(unsigned int port, GPIO_MODE mode, unsigned int mask)
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

void gpio_disable_pin(unsigned int pin)
{
    ack(object_get(SYS_OBJ_CORE), HAL_REQ(HAL_PIN, STM32_GPIO_DISABLE_PIN), pin, 0, 0);
}

void gpio_disable_mask(unsigned int port, unsigned int mask)
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

void gpio_set_pin(unsigned int pin)
{
#if defined(STM32F1) || defined (STM32L0)
    GPIO[GPIO_PORT(pin)]->BSRR = 1 << GPIO_PIN(pin);
#elif defined(STM32F2) || defined(STM32F4)
    GPIO[GPIO_PORT(pin)]->BSRRL = 1 << GPIO_PIN(pin);
#endif
}

void gpio_set_mask(unsigned int port, unsigned int mask)
{
#if defined(STM32F1) || defined (STM32L0)
    GPIO[port]->BSRR = mask;
#elif defined(STM32F2) || defined(STM32F4)
    GPIO[port]->BSRRL = mask;
#endif
}

void gpio_reset_pin(unsigned int pin)
{
#if defined(STM32F1) || defined (STM32L0)
    GPIO[GPIO_PORT(pin)]->BRR = 1 << GPIO_PIN(pin);
#elif defined(STM32F2) || defined(STM32F4)
    GPIO[GPIO_PORT(pin)]->BSRRH = 1 << GPIO_PIN(pin);
#endif
}

void gpio_reset_mask(unsigned int port, unsigned int mask)
{
#if defined(STM32F1) || defined (STM32L0)
    GPIO[port]->BRR = mask;
#elif defined(STM32F2) || defined(STM32F4)
    GPIO[port]->BSRRH = mask;
#endif
}

bool gpio_get_pin(unsigned int pin)
{
    return (GPIO[GPIO_PORT(pin)]->IDR >> GPIO_PIN(pin)) & 1;
}

unsigned int gpio_get_mask(unsigned port, unsigned int mask)
{
    return GPIO[port]->IDR & mask;
}

void gpio_set_data_out(unsigned int port, unsigned int wide)
{
#if defined(STM32F1)
    unsigned int mask = (1 << ((((wide - 1) & 7) + 1) << 2)) - 1;
    GPIO[port]->CRL &= ~mask;
    GPIO[port]->CRL |= (0x33333333 & mask);
    if (wide > 8)
    {
        mask = (1 << ((wide - 8) << 2)) - 1;
        GPIO[port]->CRH &= ~mask;
        GPIO[port]->CRH |= (0x33333333 & mask);
    }
#elif defined(STM32F2) || defined(STM32F4) || defined(STM32L0)
    unsigned int mask = (1 << (wide << 1)) - 1;
    GPIO[port]->MODER &= ~mask;
    GPIO[port]->MODER |= (0x55555555 & mask);
#endif
}

void gpio_set_data_in(unsigned int port, unsigned int wide)
{
#if defined(STM32F1)
    unsigned int mask = (1 << ((((wide - 1) & 7) + 1) << 2)) - 1;
    GPIO[port]->CRL &= ~mask;
    GPIO[port]->CRL |= (0x44444444 & mask);
    if (wide > 8)
    {
        mask = (1 << ((wide - 8) << 2)) - 1;
        GPIO[port]->CRH &= ~mask;
        GPIO[port]->CRH |= (0x44444444 & mask);
    }
#elif defined(STM32F2) || defined(STM32F4) || defined(STM32L0)
    unsigned int mask = (1 << (wide << 1)) - 1;
    GPIO[port]->MODER &= ~mask;
#endif
}
