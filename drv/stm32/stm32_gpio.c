/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "stm32_gpio.h"
#include "../../userspace/stm32_driver.h"
#include "stm32_core_private.h"
#include "sys_config.h"
#include <string.h>

#if defined(STM32F1)
const GPIO_TypeDef_P GPIO[GPIO_COUNT] =							{GPIOA, GPIOB, GPIOC, GPIOD, GPIOE, GPIOF, GPIOG};
static const unsigned int GPIO_POWER_PINS[GPIO_COUNT] =         {2, 3, 4, 5, 6, 7, 8};
#define GPIO_POWER_PORT                                         RCC->APB2ENR
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
#endif

#if defined(STM32F1)
#define GPIO_CR(pin)                                            (*((unsigned int*)((unsigned int)(GPIO[GPIO_PORT(pin)]) + 4 * ((GPIO_PIN(pin)) / 8))))
#define GPIO_CR_SET(pin, mode)                                  GPIO_CR(pin) &= ~(0xful << ((GPIO_PIN(pin) % 8) * 4ul)); \
                                                                GPIO_CR(pin) |= ((unsigned int)mode << ((GPIO_PIN(pin) % 8) * 4ul))

#define GPIO_ODR_SET(pin, mode)                                 GPIO[GPIO_PORT(pin)]->ODR &= ~(1 << GPIO_PIN(pin)); \
                                                                GPIO[GPIO_PORT(pin)]->ODR |= mode << GPIO_PIN(pin)

void stm32_gpio_enable_afio(GPIO_DRV* gpio)
{
    if (gpio->used_afio++ == 0)
        RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;
}

void stm32_gpio_disable_afio(GPIO_DRV* gpio)
{
    if (--gpio->used_afio == 0)
        RCC->APB2ENR &= ~RCC_APB2ENR_AFIOEN;
}

void stm32_gpio_enable_pin(GPIO_DRV* gpio, PIN pin, STM32_GPIO_MODE mode, bool pullup)
{
    if (gpio->used_pins[GPIO_PORT(pin)]++ == 0)
        GPIO_POWER_PORT |= 1 << GPIO_POWER_PINS[GPIO_PORT(pin)];
    GPIO_CR_SET(pin, mode);
    GPIO_ODR_SET(pin, pullup);
}


#elif defined(STM32F2) || defined(STM32F4) || defined(STM32L0)
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

#if defined(STM32L0)
void stm32_gpio_enable_syscfg(GPIO_DRV* gpio)
{
    if (gpio->used_syscfg++ == 0)
        RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
}

void stm32_gpio_disable_syscfg(GPIO_DRV* gpio)
{
    if (--gpio->used_syscfg == 0)
        RCC->APB2ENR &= ~RCC_APB2ENR_SYSCFGEN;
}
#endif

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
    switch (flags & EXTI_FLAGS_PULL_MASK)
    {
#if defined(STM32F1)
    case EXTI_FLAGS_PULLUP:
        stm32_gpio_enable_pin(gpio, pin, STM32_GPIO_MODE_INPUT_PULL, false);
        break;
    case EXTI_FLAGS_PULLDOWN:
        stm32_gpio_enable_pin(gpio, pin, STM32_GPIO_MODE_INPUT_PULL, true);
        break;
    case 0:
        stm32_gpio_enable_pin(gpio, pin, STM32_GPIO_MODE_INPUT_FLOAT, false);
#elif  defined(STM32F2) || defined(STM32F4)
    case EXTI_FLAGS_PULLUP:
        stm32_gpio_enable_pin(gpio, pin, STM32_GPIO_MODE_INPUT | GPIO_SPEED_LOW | GPIO_PUPD_PULLUP, AF0);
        break;
    case EXTI_FLAGS_PULLDOWN:
        stm32_gpio_enable_pin(gpio, pin, STM32_GPIO_MODE_INPUT | GPIO_SPEED_LOW | GPIO_PUPD_PULLDOWN, AF0);
        break;
    case 0:
        stm32_gpio_enable_pin(gpio, pin, STM32_GPIO_MODE_INPUT | GPIO_SPEED_LOW | GPIO_PUPD_NO_PULLUP, AF0);
#elif  defined(STM32L0)
    case EXTI_FLAGS_PULLUP:
        stm32_gpio_enable_pin(gpio, pin, STM32_GPIO_MODE_INPUT | GPIO_SPEED_VERY_LOW | GPIO_PUPD_PULLUP, AF0);
        break;
    case EXTI_FLAGS_PULLDOWN:
        stm32_gpio_enable_pin(gpio, pin, STM32_GPIO_MODE_INPUT | GPIO_SPEED_VERY_LOW | GPIO_PUPD_PULLDOWN, AF0);
        break;
    case 0:
        stm32_gpio_enable_pin(gpio, pin, STM32_GPIO_MODE_INPUT | GPIO_SPEED_VERY_LOW | GPIO_PUPD_NO_PULLUP, AF0);
#endif
    }

#if defined(STM32F1) || defined(STM32L0)
#if defined(STM32F1)
    stm32_gpio_enable_afio(gpio);
    AFIO->EXTICR[GPIO_PIN(pin) / 4] &= ~(0xful << (uint32_t)(GPIO_PIN(pin) & 3ul));
    AFIO->EXTICR[GPIO_PIN(pin) / 4] |= ((uint32_t)GPIO_PORT(pin) << (uint32_t)(GPIO_PIN(pin) & 3ul));
#elif defined(STM32L0)
    stm32_gpio_enable_syscfg(gpio);
    SYSCFG->EXTICR[GPIO_PIN(pin) / 4] &= ~(0xful << (uint32_t)(GPIO_PIN(pin) & 3ul));
    SYSCFG->EXTICR[GPIO_PIN(pin) / 4] |= ((uint32_t)GPIO_PORT(pin) << (uint32_t)(GPIO_PIN(pin) & 3ul));
#endif
    EXTI->RTSR &= ~(1ul << GPIO_PIN(pin));
    EXTI->FTSR &= ~(1ul << GPIO_PIN(pin));
    if (flags & EXTI_FLAGS_RISING)
        EXTI->RTSR |= 1ul << GPIO_PIN(pin);
    if (flags & EXTI_FLAGS_FALLING)
        EXTI->FTSR |= 1ul << GPIO_PIN(pin);
#endif
}

void stm32_gpio_disable_pin(GPIO_DRV* gpio, PIN pin)
{
#if defined(STM32F1)
    GPIO_CR_SET(pin, GPIO_MODE_IN_FLOAT);
#elif defined(STM32F2) || defined(STM32F4) || defined(STM3L0)
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
#if defined(STM32F1) || defined(STM32L0)
    EXTI->IMR &= ~(1ul << GPIO_PIN(pin));
    EXTI->EMR &= ~(1ul << GPIO_PIN(pin));

    EXTI->RTSR &= ~(1ul << GPIO_PIN(pin));
    EXTI->FTSR &= ~(1ul << GPIO_PIN(pin));
#if defined(STM32F1)
    stm32_gpio_disable_afio(gpio);
#elif defined(STM32L0)
    stm32_gpio_disable_syscfg(gpio);
#endif
#endif
    stm32_gpio_disable_pin(gpio, pin);
}

void stm32_gpio_disable_jtag(GPIO_DRV* gpio)
{
#if defined(STM32F1) || defined(STM32F2) || defined(STM32F4)
    gpio->used_pins[0] += 3;
    gpio->used_pins[1] += 2;
    stm32_gpio_disable_pin(gpio, A13);
    stm32_gpio_disable_pin(gpio, A14);
    stm32_gpio_disable_pin(gpio, A15);
    stm32_gpio_disable_pin(gpio, B3);
    stm32_gpio_disable_pin(gpio, B4);

#if defined(STM32F1)
    stm32_gpio_enable_afio(gpio);
	AFIO->MAPR |= AFIO_MAPR_SWJ_CFG_DISABLE;
#endif
#elif defined(STM32L0)
    gpio->used_pins[0] += 2;
    stm32_gpio_disable_pin(gpio, A13);
    stm32_gpio_disable_pin(gpio, A14);
#endif
}

void stm32_gpio_init(CORE* core)
{
    memset(&core->gpio, 0, sizeof (GPIO_DRV));
}

bool stm32_gpio_request(CORE* core, IPC* ipc)
{
    bool need_post = false;
    switch (ipc->cmd)
    {
    case STM32_GPIO_DISABLE_PIN:
        stm32_gpio_disable_pin(&core->gpio, (PIN)ipc->param1);
        need_post = true;
        break;
    case STM32_GPIO_ENABLE_PIN:
#if defined(STM32F1)
        stm32_gpio_enable_pin(&core->gpio, (PIN)ipc->param1, (STM32_GPIO_MODE)ipc->param2, ipc->param3);
#elif defined(STM32F2) || defined(STM32F4) || defined(STM32L0)
        stm32_gpio_enable_pin(&core->gpio, (PIN)ipc->param1, ipc->param2, (AF)ipc->param3);
#endif
        need_post = true;
        break;
    case STM32_GPIO_ENABLE_EXTI:
        stm32_gpio_enable_exti(&core->gpio, (PIN)ipc->param1, ipc->param2);
        need_post = true;
        break;
    case STM32_GPIO_DISABLE_EXTI:
        stm32_gpio_disable_exti(&core->gpio, (PIN)ipc->param1);
        need_post = true;
        break;
    case STM32_GPIO_DISABLE_JTAG:
        stm32_gpio_disable_jtag(&core->gpio);
        need_post = true;
        break;
#if defined(STM32F1)
    case STM32_GPIO_ENABLE_AFIO:
        stm32_gpio_enable_afio(&core->gpio);
        need_post = true;
        break;
    case STM32_GPIO_DISABLE_AFIO:
        stm32_gpio_disable_afio(&core->gpio);
        need_post = true;
        break;
#elif defined(STM32L0)
    case STM32_GPIO_ENABLE_SYSCFG:
        stm32_gpio_enable_syscfg(&core->gpio);
        need_post = true;
        break;
    case STM32_GPIO_DISABLE_SYSCFG:
        stm32_gpio_disable_syscfg(&core->gpio);
        need_post = true;
        break;
#endif
    default:
        error(ERROR_NOT_SUPPORTED);
        need_post = true;
        break;
    }
    return need_post;
}

