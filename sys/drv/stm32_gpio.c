/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "stm32_gpio.h"
#include "error.h"

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

void gpio_enable_pin_power(PIN pin)
{
//    if (__KERNEL->used_pins[GPIO_PORT(pin)]++ == 0)
        GPIO_POWER_PORT |= 1 << GPIO_POWER_PINS[GPIO_PORT(pin)];
}

#if defined(STM32F1)
#define GPIO_CR(pin)                                            (*((unsigned int*)((unsigned int)(GPIO[GPIO_PORT(pin)]) + 4 * ((GPIO_PIN(pin)) / 8))))
#define GPIO_CR_SET(pin, mode)                                  GPIO_CR(pin) &= ~(0xful << ((GPIO_PIN(pin) % 8) * 4ul)); \
                                                                GPIO_CR(pin) |= ((unsigned int)mode << ((GPIO_PIN(pin) % 8) * 4ul))

#define GPIO_ODR_SET(pin, mode)                                 GPIO[GPIO_PORT(pin)]->ODR &= ~(1 << GPIO_PIN(pin)); \
                                                                GPIO[GPIO_PORT(pin)]->ODR |= mode << GPIO_PIN(pin)

void gpio_enable_pin_system(PIN pin, GPIO_MODE mode, bool pullup)
{
    gpio_enable_pin_power(pin);
    GPIO_CR_SET(pin, mode);
    GPIO_ODR_SET(pin, pullup);
}

#endif


void gpio_enable_pin(PIN pin, PIN_MODE mode)
{
	gpio_enable_pin_power(pin);

#if defined(STM32F1)
    switch (mode)
    {
    case PIN_MODE_OUT:
        gpio_enable_pin_system(pin, GPIO_MODE_OUTPUT_PUSH_PULL_2MHZ, false);
        break;
    case PIN_MODE_IN_FLOAT:
        gpio_enable_pin_system(pin, GPIO_MODE_INPUT_FLOAT, false);
        break;
    case PIN_MODE_IN_PULLUP:
        gpio_enable_pin_system(pin, GPIO_MODE_INPUT_PULL, true);
        break;
    case PIN_MODE_IN_PULLDOWN:
        gpio_enable_pin_system(pin, GPIO_MODE_INPUT_PULL, false);
        break;
    }
#elif defined(STM32F2)
	//in/out
    GPIO[GPIO_PORT(pin)]->MODER &= ~(3 << (GPIO_PIN(pin) * 2));
	switch (mode)
	{
	case PIN_MODE_OUT:
		//speed 100mhz
        GPIO[GPIO_PORT(pin)]->OSPEEDR |= (3 << (GPIO_PIN(pin) * 2));
        GPIO[GPIO_PORT(pin)]->MODER |= (1 << (GPIO_PIN(pin) * 2));
		break;
	default:
		break;
	}

	//out pp/od
	switch (mode)
	{
	case PIN_MODE_OUT:
        GPIO[GPIO_PORT(pin)]->OTYPER &= ~(1 << GPIO_PIN(pin));
		break;
	case PIN_MODE_OUT_OD:
        GPIO[GPIO_PORT(pin)]->OTYPER |= 1 << GPIO_PIN(pin);
		break;
	default:
		break;
	}

	//pullup/pulldown
    GPIO[GPIO_PORT(pin)]->PUPDR &= ~(3 << (GPIO_PIN(pin) * 2));
	switch (mode)
	{
	case PIN_MODE_IN_PULLUP:
        GPIO[GPIO_PORT(pin)]->PUPDR |= (1 << (GPIO_PIN(pin) * 2));
		break;
	case PIN_MODE_IN_PULLDOWN:
        GPIO[GPIO_PORT(pin)]->PUPDR |= (2 << (GPIO_PIN(pin) * 2));
		break;
	default:
		break;
	}
#endif
}

void gpio_disable_pin(PIN pin)
{
#if defined(STM32F1)
    GPIO_CR_SET(pin, PIN_MODE_IN_FLOAT);
#elif defined(STM32F2)
    GPIO[GPIO_PORT(pin)]->MODER &= ~(3 << (GPIO_PIN(pin) * 2));
    GPIO[GPIO_PORT(pin)]->PUPDR &= ~(3 << (GPIO_PIN(pin) * 2));
    GPIO[GPIO_PORT(pin)]->OSPEEDR |= (3 << (GPIO_PIN(pin) * 2));
#endif
///    if (--__KERNEL->used_pins[GPIO_PORT(pin)] == 0)
        GPIO_POWER_PORT &= ~(1 << GPIO_POWER_PINS[GPIO_PORT(pin)]);
}

void gpio_set_pin(PIN pin, bool set)
{
#if defined(STM32F1)
	if (set)
        GPIO[GPIO_PORT(pin)]->BSRR = 1 << GPIO_PIN(pin);
	else
        GPIO[GPIO_PORT(pin)]->BRR = 1 << GPIO_PIN(pin);
#elif defined(STM32F2)
	if (set)
        GPIO[GPIO_PORT(pin)]->BSRRL = 1 << GPIO_PIN(pin);
	else
        GPIO[GPIO_PORT(pin)]->BSRRH = 1 << GPIO_PIN(pin);
#endif
}

bool gpio_get_pin(PIN pin)
{
    return GPIO[GPIO_PORT(pin)]->IDR & (1 << GPIO_PIN(pin)) ? true : false;
}

bool gpio_get_out_pin(PIN pin)
{
    return GPIO[GPIO_PORT(pin)]->ODR & (1 << GPIO_PIN(pin)) ? true : false;
}

void gpio_disable_jtag()
{
#if defined(STM32F1)
	afio_remap();
	AFIO->MAPR |= AFIO_MAPR_SWJ_CFG_DISABLE;
#elif defined(STM32F2)
    gpio_enable_pin(A13, PIN_MODE_IN);
    gpio_enable_pin(A14, PIN_MODE_IN);
    gpio_enable_pin(A15, PIN_MODE_IN);
    gpio_enable_pin(B3, PIN_MODE_IN);
    gpio_enable_pin(B4, PIN_MODE_IN);
#endif
}

#if defined(STM32F1)
void afio_remap()
{
///    if (__KERNEL->afio_remap_count++ == 0)
		RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;
}

void afio_unmap()
{
///    if (--__KERNEL->afio_remap_count == 0)
		RCC->APB2ENR &= ~RCC_APB2ENR_AFIOEN;
}

#endif
