/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "stm32_gpio.h"
#include "../../userspace/error.h"
#include "../sys_call.h"
#include "sys_config.h"
#include "../../userspace/lib/stdlib.h"
#include "../../userspace/lib/stdio.h"
#include <string.h>

#define GPIO_PORT(pin)                                          (pin / 16)
#define GPIO_PIN(pin)                                           (pin & 15)

typedef GPIO_TypeDef* GPIO_TypeDef_P;
extern const GPIO_TypeDef_P GPIO[];

//it's what defined in STM32F1xxx.h. Real count may be less.
#if defined(STM32F1)
#define GPIO_COUNT                                              7
const GPIO_TypeDef_P GPIO[GPIO_COUNT] =							{GPIOA, GPIOB, GPIOC, GPIOD, GPIOE, GPIOF, GPIOG};
static const unsigned int GPIO_POWER_PINS[GPIO_COUNT] =         {2, 3, 4, 5, 6, 7, 8};
#define GPIO_POWER_PORT                                         RCC->APB2ENR
#elif defined(STM32F2)
#define GPIO_COUNT                                              9
const GPIO_TypeDef_P GPIO[GPIO_COUNT] =							{GPIOA, GPIOB, GPIOC, GPIOD, GPIOE, GPIOF, GPIOG, GPIOH, GPIOI};
static const unsigned int GPIO_POWER_PINS[GPIO_COUNT] =         {0, 1, 2, 3, 4, 5, 6, 7, 8};
#define GPIO_POWER_PORT                                         RCC->AHB1ENR
#elif defined(STM32F4)
#define GPIO_COUNT                                              11
const GPIO_TypeDef_P GPIO[GPIO_COUNT] =							{GPIOA, GPIOB, GPIOC, GPIOD, GPIOE, GPIOF, GPIOG, GPIOH, GPIOI, GPIOJ, GPIOK};
static const unsigned int GPIO_POWER_PINS[GPIO_COUNT] =         {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
#define GPIO_POWER_PORT                                         RCC->AHB1ENR
#endif

#if defined(STM32F1)
#define GPIO_CR(pin)                                            (*((unsigned int*)((unsigned int)(GPIO[GPIO_PORT(pin)]) + 4 * ((GPIO_PIN(pin)) / 8))))
#define GPIO_CR_SET(pin, mode)                                  GPIO_CR(pin) &= ~(0xful << ((GPIO_PIN(pin) % 8) * 4ul)); \
                                                                GPIO_CR(pin) |= ((unsigned int)mode << ((GPIO_PIN(pin) % 8) * 4ul))

#define GPIO_ODR_SET(pin, mode)                                 GPIO[GPIO_PORT(pin)]->ODR &= ~(1 << GPIO_PIN(pin)); \
                                                                GPIO[GPIO_PORT(pin)]->ODR |= mode << GPIO_PIN(pin)

void gpio_enable_pin_system(CORE* core, PIN pin, GPIO_MODE mode, bool pullup)
{
    if (core->used_pins[GPIO_PORT(pin)]++ == 0)
        GPIO_POWER_PORT |= 1 << GPIO_POWER_PINS[GPIO_PORT(pin)];
    GPIO_CR_SET(pin, mode);
    GPIO_ODR_SET(pin, pullup);
}


#elif defined(STM32F2) || defined(STM32F4)
#define GPIO_SET_MODE(pin, mode)                                GPIO[GPIO_PORT(pin)]->MODER &= (3 << (GPIO_PIN(pin) * 2)); \
                                                                GPIO[GPIO_PORT(pin)]->MODER |= (mode << (GPIO_PIN(pin) * 2))

#define GPIO_SET_OT(pin, mode)                                  GPIO[GPIO_PORT(pin)]->OTYPER &= (1 << GPIO_PIN(pin)); \
                                                                GPIO[GPIO_PORT(pin)]->OTYPER |= (mode << GPIO_PIN(pin))

#define GPIO_SET_SPEED(pin, mode)                               GPIO[GPIO_PORT(pin)]->OSPEEDR &= (3 << (GPIO_PIN(pin) * 2)); \
                                                                GPIO[GPIO_PORT(pin)]->OSPEEDR |= (mode << (GPIO_PIN(pin) * 2))

#define GPIO_SET_PUPD(pin, mode)                                GPIO[GPIO_PORT(pin)]->PUPDR &= (3 << (GPIO_PIN(pin) * 2)); \
                                                                GPIO[GPIO_PORT(pin)]->PUPDR |= (mode << (GPIO_PIN(pin) * 2))

#define GPIO_AFR(pin)                                           (*((unsigned int*)((unsigned int)(GPIO[GPIO_PORT(pin)]) + 0x20 + 4 * ((GPIO_PIN(pin)) / 8))))
#define GPIO_AFR_SET(pin, mode)                                 GPIO_AFR(pin) &= ~(0xful << ((GPIO_PIN(pin) % 8) * 4ul)); \
                                                                GPIO_AFR(pin) |= ((unsigned int)mode << ((GPIO_PIN(pin) % 8) * 4ul))

void gpio_enable_pin_system(CORE* core, PIN pin, unsigned int mode, AF af)
{
    if (core->used_pins[GPIO_PORT(pin)]++ == 0)
        GPIO_POWER_PORT |= 1 << GPIO_POWER_PINS[GPIO_PORT(pin)];
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
#endif
}

void gpio_disable_pin(CORE* core, PIN pin)
{
#if defined(STM32F1)
    GPIO_CR_SET(pin, PIN_MODE_IN_FLOAT);
#elif defined(STM32F2)
    GPIO_SET_MODE(pin, GPIO_MODE_INPUT >> 0);
    GPIO_SET_SPEED(pin, GPIO_SPEED_LOW >> 3);
    GPIO_SET_PUPD(pin,  GPIO_PUPD_NO_PULLUP >> 5);
    GPIO_AFR_SET(pin, AF0);
#endif
    if (--core->used_pins[GPIO_PORT(pin)] == 0)
        GPIO_POWER_PORT &= ~(1 << GPIO_POWER_PINS[GPIO_PORT(pin)]);
}

void gpio_set_pin(PIN pin, bool set)
{
#if defined(STM32F1)
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
    core->used_pins[0] += 3;
    core->used_pins[2] += 2;
    gpio_disable_pin(core, A13);
    gpio_disable_pin(core, A14);
    gpio_disable_pin(core, A15);
    gpio_disable_pin(core, B3);
    gpio_disable_pin(core, B4);

#if defined(STM32F1)
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;
	AFIO->MAPR |= AFIO_MAPR_SWJ_CFG_DISABLE;
#endif
}

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

void stm32_gpio_init(CORE* core)
{
    core->used_pins = (int*)malloc(sizeof(int) * GPIO_COUNT);
    memset(core->used_pins, 0, sizeof(int) * GPIO_COUNT);
}
