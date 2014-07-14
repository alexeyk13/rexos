/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "stm32_gpio.h"
#include "../../userspace/error.h"
#include "../../userspace/process.h"
#include "../sys_call.h"
#include "sys_config.h"
#include "../../userspace/lib/stdio.h"

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

static void gpio_enable_pin_system(int* used_pins, PIN pin, GPIO_MODE mode, bool pullup)
{
    if (used_pins[GPIO_PORT(pin)]++ == 0)
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

static void gpio_enable_pin_system(int* used_pins, PIN pin, unsigned int mode, AF af)
{
    if (used_pins[GPIO_PORT(pin)]++ == 0)
        GPIO_POWER_PORT |= 1 << GPIO_POWER_PINS[GPIO_PORT(pin)];
    GPIO_SET_MODE(pin, (mode >> 0) & 3);
    GPIO_SET_OT(pin, (mode >> 2) & 1);
    GPIO_SET_SPEED(pin, (mode >> 3) & 3);
    GPIO_SET_PUPD(pin, (mode >> 5) & 3);
    GPIO_AFR_SET(pin, af);
}
#endif

static inline void gpio_enable_pin(int* used_pins, PIN pin, PIN_MODE mode)
{
#if defined(STM32F1)
    switch (mode)
    {
    case PIN_MODE_OUT:
        gpio_enable_pin_system(used_pins, pin, GPIO_MODE_OUTPUT_PUSH_PULL_2MHZ, false);
        break;
    case PIN_MODE_IN_FLOAT:
        gpio_enable_pin_system(used_pins, pin, GPIO_MODE_INPUT_FLOAT, false);
        break;
    case PIN_MODE_IN_PULLUP:
        gpio_enable_pin_system(used_pins, pin, GPIO_MODE_INPUT_PULL, true);
        break;
    case PIN_MODE_IN_PULLDOWN:
        gpio_enable_pin_system(used_pins, pin, GPIO_MODE_INPUT_PULL, false);
        break;
    }
#elif defined(STM32F2) || defined(STM32F4)
    switch (mode)
    {
    case PIN_MODE_OUT:
        gpio_enable_pin_system(used_pins, pin, GPIO_MODE_OUTPUT | GPIO_OT_PUSH_PULL | GPIO_SPEED_LOW | GPIO_PUPD_NO_PULLUP, AF0);
        break;
    case PIN_MODE_IN_FLOAT:
        gpio_enable_pin_system(used_pins, pin, GPIO_MODE_INPUT | GPIO_SPEED_LOW | GPIO_PUPD_NO_PULLUP, AF0);
        break;
    case PIN_MODE_IN_PULLUP:
        gpio_enable_pin_system(used_pins, pin, GPIO_MODE_INPUT | GPIO_SPEED_LOW | GPIO_PUPD_PULLUP, AF0);
        break;
    case PIN_MODE_IN_PULLDOWN:
        gpio_enable_pin_system(used_pins, pin, GPIO_MODE_INPUT | GPIO_SPEED_LOW | GPIO_PUPD_PULLDOWN, AF0);
        break;
    }
#endif
}

static void gpio_disable_pin(int* used_pins, PIN pin)
{
#if defined(STM32F1)
    GPIO_CR_SET(pin, PIN_MODE_IN_FLOAT);
#elif defined(STM32F2)
    GPIO_SET_MODE(pin, GPIO_MODE_INPUT >> 0);
    GPIO_SET_SPEED(pin, GPIO_SPEED_LOW >> 3);
    GPIO_SET_PUPD(pin,  GPIO_PUPD_NO_PULLUP >> 5);
    GPIO_AFR_SET(pin, AF0);
#endif
    if (--used_pins[GPIO_PORT(pin)] == 0)
        GPIO_POWER_PORT &= ~(1 << GPIO_POWER_PINS[GPIO_PORT(pin)]);
}

static inline void gpio_set_pin(PIN pin, bool set)
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

static inline bool gpio_get_pin(PIN pin)
{
    return (GPIO[GPIO_PORT(pin)]->IDR >> GPIO_PIN(pin)) & 1;
}

static inline void gpio_disable_jtag(int* used_pins)
{
    used_pins[0] += 3;
    used_pins[2] += 2;
    gpio_disable_pin(used_pins, A13);
    gpio_disable_pin(used_pins, A14);
    gpio_disable_pin(used_pins, A15);
    gpio_disable_pin(used_pins, B3);
    gpio_disable_pin(used_pins, B4);

#if defined(STM32F1)
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;
	AFIO->MAPR |= AFIO_MAPR_SWJ_CFG_DISABLE;
#endif
}

static inline void stm32_gpio_info(int* used_pins)
{
    int i;
    bool empty = true;
    printf("STM32 gpio driver info\n\r\n\r");
    printf("Total ports count: %d\n\r", GPIO_COUNT);
    printf("Active ports: ");
    for (i = 0; i < GPIO_COUNT; ++i)
    {
        if (!empty && used_pins[i])
            printf(", ");
        if (used_pins[i])
        {
            printf("%c(%d)", 'A' + i, used_pins[i]);
            empty = false;
        }
    }
    printf("\n\r\n\r");
}

void stm32_gpio()
{
    IPC ipc;
    int used_pins[GPIO_COUNT] = {0};
    sys_ack(SYS_SET_OBJECT, SYS_OBJECT_GPIO, 0, 0);
    for (;;)
    {
        ipc_read_ms(&ipc, 0, 0);
        switch (ipc.cmd)
        {
        case IPC_PING:
            ipc_post(&ipc);
            break;
        case IPC_CALL_ERROR:
            break;
        case SYS_SET_STDIO:
            open_stdout();
            ipc_post(&ipc);
            break;
#if (SYS_INFO)
        case SYS_GET_INFO:
            stm32_gpio_info(used_pins);
            ipc_post(&ipc);
            break;
#endif
        case IPC_ENABLE_PIN:
            gpio_enable_pin(used_pins, (PIN)ipc.param1, (PIN_MODE)ipc.param2);
            ipc_post(&ipc);
            break;
        case IPC_ENABLE_PIN_SYSTEM:
#if defined(STM32F1)
            gpio_enable_pin_system(used_pins, (PIN)ipc.param1, (GPIO_MODE)ipc.param2, ipc.param3);
#elif defined(STM32F2) || defined(STM32F4)
            gpio_enable_pin_system(used_pins, (PIN)ipc.param1, ipc.param2, (AF)ipc.param3);
#endif
            ipc_post(&ipc);
            break;
        case IPC_DISABLE_PIN:
            gpio_disable_pin(used_pins, (PIN)ipc.param1);
            ipc_post(&ipc);
            break;
        case IPC_SET_PIN:
            gpio_set_pin((PIN)ipc.param1, (bool)ipc.param2);
            ipc_post(&ipc);
            break;
        case IPC_GET_PIN:
            ipc.param1 = gpio_get_pin((PIN)ipc.param1);
            ipc_post(&ipc);
            break;
        case IPC_DISABLE_JTAG:
            gpio_disable_jtag(used_pins);
            ipc_post(&ipc);
            break;
        default:
            ipc_post_error(ipc.process, ERROR_NOT_SUPPORTED);
            break;
        }
    }
}
