/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "stm32_pin.h"
#include "../../userspace/stm32/stm32_driver.h"
#include "stm32_exo_private.h"
#include "sys_config.h"
#include <string.h>

//ALL STM32 has ports A, B, C
#ifndef GPIOD
#define GPIOD                                                   0
#endif
#ifndef GPIOE
#define GPIOE                                                   0
#endif
#ifndef GPIOF
#define GPIOF                                                   0
#endif

#if defined(STM32F1)
const GPIO_TypeDef_P GPIO[GPIO_COUNT] =							{GPIOA, GPIOB, GPIOC, GPIOD, GPIOE, GPIOF, GPIOG};
static const unsigned int GPIO_POWER_PINS[GPIO_COUNT] =         {2, 3, 4, 5, 6, 7, 8};
#define GPIO_POWER_PORT                                         RCC->APB2ENR
#elif defined(STM32F0)
const GPIO_TypeDef_P GPIO[GPIO_COUNT] =							{GPIOA, GPIOB, GPIOC, GPIOD, GPIOE, GPIOF};
static const unsigned int GPIO_POWER_PINS[GPIO_COUNT] =         {17, 18, 19, 20, 21, 22};
#define GPIO_POWER_PORT                                         RCC->AHBENR
#elif defined(STM32F2)
const GPIO_TypeDef_P GPIO[GPIO_COUNT] =							{GPIOA, GPIOB, GPIOC, GPIOD, GPIOE, GPIOF, GPIOG, GPIOH, GPIOI};
static const unsigned int GPIO_POWER_PINS[GPIO_COUNT] =         {0, 1, 2, 3, 4, 5, 6, 7, 8};
#define GPIO_POWER_PORT                                         RCC->AHB1ENR
#elif defined(STM32F4)
const GPIO_TypeDef_P GPIO[GPIO_COUNT] =							{GPIOA, GPIOB, GPIOC, GPIOD, GPIOE, GPIOF, GPIOG, GPIOH, GPIOI, GPIOJ, GPIOK};
static const unsigned int GPIO_POWER_PINS[GPIO_COUNT] =         {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
#define GPIO_POWER_PORT                                         RCC->AHB1ENR
#elif defined(STM32L0)
const GPIO_TypeDef_P GPIO[8] =                                  {GPIOA, GPIOB, GPIOC, GPIOD, 0, 0, 0, GPIOH};
#elif defined(STM32L1)
const GPIO_TypeDef_P GPIO[4] =                                  {GPIOA, GPIOB, GPIOC, GPIOD};
static const unsigned int GPIO_POWER_PINS[GPIO_COUNT] =         {0, 1, 2, 3};
#define GPIO_POWER_PORT                                         RCC->AHBENR
#endif

#if defined(STM32F1)
#define GPIO_CR(pin)                                            (*((unsigned int*)((unsigned int)(GPIO[GPIO_PORT(pin)]) + 4 * ((GPIO_PIN(pin)) / 8))))
#define GPIO_CR_SET(pin, mode)                                  GPIO_CR(pin) &= ~(0xful << ((GPIO_PIN(pin) % 8) * 4ul)); \
                                                                GPIO_CR(pin) |= ((unsigned int)mode << ((GPIO_PIN(pin) % 8) * 4ul))

#define GPIO_ODR_SET(pin, mode)                                 GPIO[GPIO_PORT(pin)]->ODR &= ~(1 << GPIO_PIN(pin)); \
                                                                GPIO[GPIO_PORT(pin)]->ODR |= mode << GPIO_PIN(pin)

void stm32_gpio_enable_pin(GPIO_DRV* gpio, PIN pin, STM32_GPIO_MODE mode, bool pullup)
{
    if (gpio->used_pins[GPIO_PORT(pin)]++ == 0)
        GPIO_POWER_PORT |= 1 << GPIO_POWER_PINS[GPIO_PORT(pin)];
    GPIO_CR_SET(pin, mode);
    GPIO_ODR_SET(pin, pullup);
}


#else
#define GPIO_SET_MODE(pin, mode)                                GPIO[GPIO_PORT(pin)]->MODER &= ~(3 << (GPIO_PIN(pin) * 2)); \
                                                                GPIO[GPIO_PORT(pin)]->MODER |= ((mode) << (GPIO_PIN(pin) * 2))

#define GPIO_SET_OT(pin, mode)                                  GPIO[GPIO_PORT(pin)]->OTYPER &= ~(1 << GPIO_PIN(pin)); \
                                                                GPIO[GPIO_PORT(pin)]->OTYPER |= ((mode) << GPIO_PIN(pin))

#define GPIO_SET_SPEED(pin, mode)                               GPIO[GPIO_PORT(pin)]->OSPEEDR &= ~(3 << (GPIO_PIN(pin) * 2)); \
                                                                GPIO[GPIO_PORT(pin)]->OSPEEDR |= ((mode) << (GPIO_PIN(pin) * 2))

#define GPIO_SET_PUPD(pin, mode)                                GPIO[GPIO_PORT(pin)]->PUPDR &= ~(3 << (GPIO_PIN(pin) * 2)); \
                                                                GPIO[GPIO_PORT(pin)]->PUPDR |= ((mode) << (GPIO_PIN(pin) * 2))

#define GPIO_AFR(pin)                                           (*((unsigned int*)((unsigned int)(GPIO[GPIO_PORT(pin)]) + 0x20 + 4 * ((GPIO_PIN(pin)) / 8))))
#define GPIO_AFR_SET(pin, mode)                                 GPIO_AFR(pin) &= ~(0xful << ((GPIO_PIN(pin) % 8) * 4ul)); \
                                                                GPIO_AFR(pin) |= ((unsigned int)(mode) << ((GPIO_PIN(pin) % 8) * 4ul))

void stm32_gpio_enable_pin(GPIO_DRV* gpio, PIN pin, unsigned int mode, AF af)
{
#if defined(STM32L0)
    if (GPIO_PORT(pin) >= GPIO_COUNT || (gpio->used_pins[GPIO_PORT(pin)]++ == 0))
        RCC->IOPENR |= 1 << GPIO_PORT(pin);
#else
    if (gpio->used_pins[GPIO_PORT(pin)]++ == 0)
        GPIO_POWER_PORT |= 1 << GPIO_POWER_PINS[GPIO_PORT(pin)];
#endif
    GPIO_SET_MODE(pin, (mode >> 0) & 3);
    GPIO_SET_OT(pin, (mode >> 2) & 1);
    GPIO_SET_SPEED(pin, (mode >> 3) & 3);
    GPIO_SET_PUPD(pin, (mode >> 5) & 3);
    GPIO_AFR_SET(pin, af);
}
#endif

void stm32_gpio_enable_exti(GPIO_DRV* gpio, PIN pin, unsigned int flags)
{
#if defined(STM32F1)
    AFIO->EXTICR[GPIO_PIN(pin) / 4] &= ~(0xful << (uint32_t)(GPIO_PIN(pin) & 3ul));
    AFIO->EXTICR[GPIO_PIN(pin) / 4] |= ((uint32_t)GPIO_PORT(pin) << (uint32_t)(GPIO_PIN(pin) & 3ul));
#else
    SYSCFG->EXTICR[GPIO_PIN(pin) / 4] &= ~(0xful << (uint32_t)(GPIO_PIN(pin) & 3ul));
    SYSCFG->EXTICR[GPIO_PIN(pin) / 4] |= ((uint32_t)GPIO_PORT(pin) << (uint32_t)(GPIO_PIN(pin) & 3ul));
#endif
    EXTI->RTSR &= ~(1ul << GPIO_PIN(pin));
    EXTI->FTSR &= ~(1ul << GPIO_PIN(pin));
    if (flags & EXTI_FLAGS_RISING)
        EXTI->RTSR |= 1ul << GPIO_PIN(pin);
    if (flags & EXTI_FLAGS_FALLING)
        EXTI->FTSR |= 1ul << GPIO_PIN(pin);
    EXTI->IMR |= 1ul << GPIO_PIN(pin);
    EXTI->EMR |= 1ul << GPIO_PIN(pin);
}

void stm32_gpio_disable_pin(GPIO_DRV* gpio, PIN pin)
{
#if defined(STM32F1)
    GPIO_CR_SET(pin, GPIO_MODE_IN_FLOAT);
#else
    GPIO_SET_MODE(pin, STM32_GPIO_MODE_INPUT >> 0);
#if defined(STM32L0)
    GPIO_SET_SPEED(pin, GPIO_SPEED_VERY_LOW >> 3);
#else
    GPIO_SET_SPEED(pin, GPIO_SPEED_LOW >> 3);
#endif
    GPIO_SET_PUPD(pin,  GPIO_PUPD_NO_PULLUP >> 5);
    GPIO_AFR_SET(pin, AF0);
#endif
#if defined(STM32L0)
    if (GPIO_PORT(pin) >= GPIO_COUNT || (--gpio->used_pins[GPIO_PORT(pin)] == 0))
        RCC->IOPENR &= ~(1 << GPIO_PORT(pin));
#else
    if (--gpio->used_pins[GPIO_PORT(pin)] == 0)
        GPIO_POWER_PORT &= ~(1 << GPIO_POWER_PINS[GPIO_PORT(pin)]);
#endif
}

void stm32_gpio_disable_exti(GPIO_DRV* gpio, PIN pin)
{
    EXTI->IMR &= ~(1ul << GPIO_PIN(pin));
    EXTI->EMR &= ~(1ul << GPIO_PIN(pin));

    EXTI->RTSR &= ~(1ul << GPIO_PIN(pin));
    EXTI->FTSR &= ~(1ul << GPIO_PIN(pin));
}

void stm32_pin_init(EXO* exo)
{
    memset(&exo->gpio, 0, sizeof (GPIO_DRV));
}

void stm32_pin_request(EXO* exo, IPC* ipc)
{
    switch (HAL_ITEM(ipc->cmd))
    {
    case IPC_CLOSE:
        stm32_gpio_disable_pin(&exo->gpio, (PIN)ipc->param1);
        break;
    case IPC_OPEN:
#if defined(STM32F1)
        stm32_gpio_enable_pin(&exo->gpio, (PIN)ipc->param1, (STM32_GPIO_MODE)ipc->param2, ipc->param3);
#else
        stm32_gpio_enable_pin(&exo->gpio, (PIN)ipc->param1, ipc->param2, (AF)ipc->param3);
#endif
        break;

    case STM32_GPIO_ENABLE_EXTI:
        stm32_gpio_enable_exti(&exo->gpio, (PIN)ipc->param1, ipc->param2);
        break;
    case STM32_GPIO_DISABLE_EXTI:
        stm32_gpio_disable_exti(&exo->gpio, (PIN)ipc->param1);
        break;
    default:
        error(ERROR_NOT_SUPPORTED);
        break;
    }
}
