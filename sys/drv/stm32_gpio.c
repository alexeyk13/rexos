/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "stm32_gpio.h"
#include "sys_config.h"
#if (SYS_INFO)
#include "../../userspace/lib/stdlib.h"
#include "../../userspace/lib/stdio.h"
#endif
#include <string.h>

#define GPIO_PORT(pin)                                          (pin / 16)
#define GPIO_PIN(pin)                                           (pin & 15)

typedef GPIO_TypeDef* GPIO_TypeDef_P;
const GPIO_TypeDef_P GPIO[];

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

void gpio_enable_afio(CORE* core)
{
    if (core->used_afio++ == 0)
        RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;
}

void gpio_disable_afio(CORE* core)
{
    if (--core->used_afio == 0)
        RCC->APB2ENR &= ~RCC_APB2ENR_AFIOEN;
}

void gpio_enable_pin_system(CORE* core, PIN pin, GPIO_MODE mode, bool pullup)
{
    if (core->used_pins[GPIO_PORT(pin)]++ == 0)
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
void gpio_enable_syscfg(CORE* core)
{
    if (core->used_syscfg++ == 0)
        RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
}

void gpio_disable_syscfg(CORE* core)
{
    if (--core->used_syscfg == 0)
        RCC->APB2ENR &= ~RCC_APB2ENR_SYSCFGEN;
}
#endif

void gpio_enable_pin_system(CORE* core, PIN pin, unsigned int mode, AF af)
{
#if defined(STM32L0)
    if (GPIO_PORT(pin) >= GPIO_COUNT || (core->used_pins[GPIO_PORT(pin)]++ == 0))
        RCC->IOPENR |= 1 << GPIO_PORT(pin);
#else
    if (core->used_pins[GPIO_PORT(pin)]++ == 0)
        GPIO_POWER_PORT |= 1 << GPIO_POWER_PINS[GPIO_PORT(pin)];
#endif
    GPIO_SET_MODE(pin, (mode >> 0) & 3);
    GPIO_SET_OT(pin, (mode >> 2) & 1);
    GPIO_SET_SPEED(pin, (mode >> 3) & 3);
    GPIO_SET_PUPD(pin, (mode >> 5) & 3);
    GPIO_AFR_SET(pin, af);
}
#endif

void gpio_enable_pin(CORE* core, PIN pin, PIN_MODE mode)
{
#if defined(STM32F1)
    switch (mode)
    {
    case PIN_MODE_OUT:
        gpio_enable_pin_system(core, pin, GPIO_MODE_OUTPUT_PUSH_PULL_2MHZ, false);
        break;
    case PIN_MODE_IN_FLOAT:
        gpio_enable_pin_system(core, pin, GPIO_MODE_INPUT_FLOAT, false);
        break;
    case PIN_MODE_IN_PULLUP:
        gpio_enable_pin_system(core, pin, GPIO_MODE_INPUT_PULL, true);
        break;
    case PIN_MODE_IN_PULLDOWN:
        gpio_enable_pin_system(core, pin, GPIO_MODE_INPUT_PULL, false);
        break;
    }
#elif defined(STM32F2) || defined(STM32F4)
    switch (mode)
    {
    case PIN_MODE_OUT:
        gpio_enable_pin_system(core, pin, GPIO_MODE_OUTPUT | GPIO_OT_PUSH_PULL | GPIO_SPEED_LOW | GPIO_PUPD_NO_PULLUP, AF0);
        break;
    case PIN_MODE_IN_FLOAT:
        gpio_enable_pin_system(core, pin, GPIO_MODE_INPUT | GPIO_SPEED_LOW | GPIO_PUPD_NO_PULLUP, AF0);
        break;
    case PIN_MODE_IN_PULLUP:
        gpio_enable_pin_system(core, pin, GPIO_MODE_INPUT | GPIO_SPEED_LOW | GPIO_PUPD_PULLUP, AF0);
        break;
    case PIN_MODE_IN_PULLDOWN:
        gpio_enable_pin_system(core, pin, GPIO_MODE_INPUT | GPIO_SPEED_LOW | GPIO_PUPD_PULLDOWN, AF0);
        break;
    }
#elif defined(STM32L0)
    switch (mode)
    {
    case PIN_MODE_OUT:
        gpio_enable_pin_system(core, pin, GPIO_MODE_OUTPUT | GPIO_OT_PUSH_PULL | GPIO_SPEED_VERY_LOW | GPIO_PUPD_NO_PULLUP, AF0);
        break;
    case PIN_MODE_IN_FLOAT:
        gpio_enable_pin_system(core, pin, GPIO_MODE_INPUT | GPIO_SPEED_VERY_LOW | GPIO_PUPD_NO_PULLUP, AF0);
        break;
    case PIN_MODE_IN_PULLUP:
        gpio_enable_pin_system(core, pin, GPIO_MODE_INPUT | GPIO_SPEED_VERY_LOW | GPIO_PUPD_PULLUP, AF0);
        break;
    case PIN_MODE_IN_PULLDOWN:
        gpio_enable_pin_system(core, pin, GPIO_MODE_INPUT | GPIO_SPEED_VERY_LOW | GPIO_PUPD_PULLDOWN, AF0);
        break;
    }
#endif
}

void gpio_enable_exti(CORE* core, PIN pin, unsigned int flags)
{
    switch (flags & EXTI_FLAGS_PULL_MASK)
    {
    case 0:
        gpio_enable_pin(core, pin, PIN_MODE_IN_FLOAT);
        break;
    case EXTI_FLAGS_PULLUP:
        gpio_enable_pin(core, pin, PIN_MODE_IN_PULLUP);
        break;
    case EXTI_FLAGS_PULLDOWN:
        gpio_enable_pin(core, pin, PIN_MODE_IN_PULLDOWN);
        break;
    }
#if defined(STM32F1) || defined(STM32L0)
#if defined(STM32F1)
    gpio_enable_afio(core);
    AFIO->EXTICR[GPIO_PIN(pin) / 4] &= ~(0xful << (uint32_t)(GPIO_PIN(pin) & 3ul));
    AFIO->EXTICR[GPIO_PIN(pin) / 4] |= ((uint32_t)GPIO_PORT(pin) << (uint32_t)(GPIO_PIN(pin) & 3ul));
#elif defined(STM32L0)
    gpio_enable_syscfg(core);
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

void gpio_disable_exti(CORE* core, PIN pin)
{
#if defined(STM32F1) || defined(STM32L0)
    EXTI->IMR &= ~(1ul << GPIO_PIN(pin));
    EXTI->EMR &= ~(1ul << GPIO_PIN(pin));

    EXTI->RTSR &= ~(1ul << GPIO_PIN(pin));
    EXTI->FTSR &= ~(1ul << GPIO_PIN(pin));
#if defined(STM32F1)
    gpio_disable_afio(core);
#elif defined(STM32L0)
    gpio_disable_syscfg(core);
#endif
#endif
    gpio_disable_pin(core, pin);
}

void gpio_disable_pin(CORE* core, PIN pin)
{
#if defined(STM32F1)
    GPIO_CR_SET(pin, PIN_MODE_IN_FLOAT);
#elif defined(STM32F2) || defined(STM32F4) || defined(STM3L0)
    GPIO_SET_MODE(pin, GPIO_MODE_INPUT >> 0);
#if defined(STM32L0)
    GPIO_SET_SPEED(pin, GPIO_SPEED_VERY_LOW >> 3);
#else
    GPIO_SET_SPEED(pin, GPIO_SPEED_LOW >> 3);
#endif
    GPIO_SET_PUPD(pin,  GPIO_PUPD_NO_PULLUP >> 5);
    GPIO_AFR_SET(pin, AF0);
#endif
#if defined(STM32L0)
    if (GPIO_PORT(pin) >= GPIO_COUNT || (--core->used_pins[GPIO_PORT(pin)] == 0))
        RCC->IOPENR &= ~(1 << GPIO_PORT(pin));
#else
    if (--core->used_pins[GPIO_PORT(pin)] == 0)
        GPIO_POWER_PORT &= ~(1 << GPIO_POWER_PINS[GPIO_PORT(pin)]);
#endif
}

void gpio_set_pin(PIN pin, bool set)
{
#if defined(STM32F1) || defined (STM32L0)
	if (set)
        GPIO[GPIO_PORT(pin)]->BSRR = 1 << GPIO_PIN(pin);
	else
        GPIO[GPIO_PORT(pin)]->BRR = 1 << GPIO_PIN(pin);
#elif defined(STM32F2) || defined(STM32F4)
	if (set)
        GPIO[GPIO_PORT(pin)]->BSRRL = 1 << GPIO_PIN(pin);
	else
        GPIO[GPIO_PORT(pin)]->BSRRH = 1 << GPIO_PIN(pin);
#endif
}

bool gpio_get_pin(PIN pin)
{
    return (GPIO[GPIO_PORT(pin)]->IDR >> GPIO_PIN(pin)) & 1;
}

void gpio_disable_jtag(CORE *core)
{
#if defined(STM32F1) || defined(STM32F2) || defined(STM32F4)
    core->used_pins[0] += 3;
    core->used_pins[1] += 2;
    gpio_disable_pin(core, A13);
    gpio_disable_pin(core, A14);
    gpio_disable_pin(core, A15);
    gpio_disable_pin(core, B3);
    gpio_disable_pin(core, B4);

#if defined(STM32F1)
    gpio_enable_afio(core);
	AFIO->MAPR |= AFIO_MAPR_SWJ_CFG_DISABLE;
#endif
#elif defined(STM32L0)
    core->used_pins[0] += 2;
    gpio_disable_pin(core, A13);
    gpio_disable_pin(core, A14);
#endif
}

#if (SYS_INFO)
void stm32_gpio_info(CORE* core)
{
    int i;
    bool empty = true;
    printf("Total GPIO count: %d\n\r", GPIO_COUNT);
    printf("Active GPIO: ");
    for (i = 0; i < GPIO_COUNT; ++i)
    {
        if (!empty && core->used_pins[i])
            printf(", ");
        if (core->used_pins[i])
        {
            printf("%c(%d)", 'A' + i, core->used_pins[i]);
            empty = false;
        }
    }
    printf("\n\r");
}
#endif

void stm32_gpio_init(CORE* core)
{
    memset(core->used_pins, 0, sizeof(int) * GPIO_COUNT);
#if defined(STM32F1)
    core->used_afio = 0;
#elif defined(STM32L0)
    core->used_syscfg = 0;
#endif
}
