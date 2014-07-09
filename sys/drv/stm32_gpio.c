/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "stm32_gpio.h"
#include "../../userspace/error.h"
#include "../../userspace/process.h"
#include "../sys_call.h"

void stm32_gpio();

const REX __STM32_GPIO = {
    //name
    "STM32 gpio",
    //size
    512,
    //priority - driver priority
    91,
    //flags
    PROCESS_FLAGS_ACTIVE,
    //ipc size
    10,
    //function
    stm32_gpio
};

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

void gpio_enable_pin_system(PIN pin, GPIO_MODE mode, bool pullup)
{
    //    if (__KERNEL->used_pins[GPIO_PORT(pin)]++ == 0)
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

void gpio_enable_pin_system(PIN pin, unsigned int mode, AF af)
{
    GPIO_SET_MODE(pin, (mode >> 0) & 3);
    GPIO_SET_OT(pin, (mode >> 2) & 1);
    GPIO_SET_SPEED(pin, (mode >> 3) & 3);
    GPIO_SET_PUPD(pin, (mode >> 5) & 3);
    GPIO_AFR_SET(pin, af);
}
#endif

void gpio_enable_pin(PIN pin, PIN_MODE mode)
{
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
#elif defined(STM32F2) || defined(STM32F4)
    switch (mode)
    {
    case PIN_MODE_OUT:
        gpio_enable_pin_system(pin, GPIO_MODE_OUTPUT | GPIO_OT_PUSH_PULL | GPIO_SPEED_LOW | GPIO_PUPD_NO_PULLUP, AF0);
        break;
    case PIN_MODE_IN_FLOAT:
        gpio_enable_pin_system(pin, GPIO_MODE_INPUT | GPIO_SPEED_LOW | GPIO_PUPD_NO_PULLUP, AF0);
        break;
    case PIN_MODE_IN_PULLUP:
        gpio_enable_pin_system(pin, GPIO_MODE_INPUT | GPIO_SPEED_LOW | GPIO_PUPD_PULLUP, AF0);
        break;
    case PIN_MODE_IN_PULLDOWN:
        gpio_enable_pin_system(pin, GPIO_MODE_INPUT | GPIO_SPEED_LOW | GPIO_PUPD_PULLDOWN, AF0);
        break;
    }
#endif
}

void gpio_disable_pin(PIN pin)
{
#if defined(STM32F1)
    GPIO_CR_SET(pin, PIN_MODE_IN_FLOAT);
#elif defined(STM32F2)
    GPIO_SET_MODE(pin, GPIO_MODE_INPUT >> 0);
    GPIO_SET_SPEED(pin, GPIO_SPEED_LOW >> 3);
    GPIO_SET_PUPD(pin,  GPIO_PUPD_NO_PULLUP >> 5);
    GPIO_AFR_SET(pin, AF0);
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

void gpio_disable_jtag()
{
#if defined(STM32F1)
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;
	AFIO->MAPR |= AFIO_MAPR_SWJ_CFG_DISABLE;
#elif defined(STM32F2)
    //TODO
    gpio_enable_pin(A13, PIN_MODE_IN);
    gpio_enable_pin(A14, PIN_MODE_IN);
    gpio_enable_pin(A15, PIN_MODE_IN);
    gpio_enable_pin(B3, PIN_MODE_IN);
    gpio_enable_pin(B4, PIN_MODE_IN);
#endif
}

void stm32_gpio()
{
    IPC ipc;
    sys_post(SYS_SET_GPIO, 0, 0, 0);
    for (;;)
    {
        ipc_wait_peek_ms(&ipc, 0, 0);
        switch (ipc.cmd)
        {
        case IPC_PING:
            ipc.cmd = IPC_PONG;
            ipc_post(&ipc);
            break;
        case SYS_SET_STDOUT:
            __HEAP->stdout = (STDOUT)ipc.param1;
            __HEAP->stdout_param = (void*)ipc.param2;
            break;
#if (SYS_DEBUG)
//        case SYS_GET_INFO:
//            stm32_power_info();
//            break;
#endif
        case IPC_ENABLE_PIN:
            gpio_enable_pin((PIN)ipc.param1, (PIN_MODE)ipc.param2);
            break;
        case IPC_ENABLE_PIN_SYSTEM:
            //TODO
//            gpio_enable_pin((PIN)ipc.param1, (PIN_MODE)ipc.param2);
            break;
        case IPC_DISABLE_PIN:
            gpio_disable_pin((PIN)ipc.param1);
            break;
        case IPC_SET_PIN:
            gpio_set_pin((PIN)ipc.param1, (bool)ipc.param2);
            break;
        case IPC_GET_PIN:
            ipc.param1 = gpio_get_pin((PIN)ipc.param1);
            ipc_post(&ipc);
            break;
        case IPC_DISABLE_JTAG:
            gpio_disable_jtag();
            break;
        default:
            break;
        }
    }
}
