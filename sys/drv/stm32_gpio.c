/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "arch.h"
#include "stm32_gpio.h"
#include "error.h"
#include "../../../kernel/kernel.h"

#define PORT(pin)																																(pin / 32)
#define PIN(pin)																																(pin & 31)


#if defined(STM32F1)
const char PIN_MODE_VALUE[] =																												{0x3, 0x4, 0x7, 0x8, 0x8};
#if defined(STM32F10X_LD) || defined(STM32F10X_LD_VL)
const uint32_t RCC_GPIO[] =																												{RCC_APB2ENR_IOPAEN, RCC_APB2ENR_IOPBEN, RCC_APB2ENR_IOPCEN,
																																					 RCC_APB2ENR_IOPDEN};
const GPIO_TypeDef_P GPIO[] =																												{GPIOA, GPIOB, GPIOC, GPIOD};
#elif defined(STM32F10X_MD) || defined(STM32F10X_MD_VL) || defined(STM32F10X_CL)
const uint32_t RCC_GPIO[] =																												{RCC_APB2ENR_IOPAEN, RCC_APB2ENR_IOPBEN, RCC_APB2ENR_IOPCEN,
																																					 RCC_APB2ENR_IOPDEN, RCC_APB2ENR_IOPEEN};
const GPIO_TypeDef_P GPIO[] =																												{GPIOA, GPIOB, GPIOC, GPIOD, GPIOE};
#else
const uint32_t RCC_GPIO[] =																												{RCC_APB2ENR_IOPAEN, RCC_APB2ENR_IOPBEN, RCC_APB2ENR_IOPCEN,
																																					 RCC_APB2ENR_IOPDEN, RCC_APB2ENR_IOPEEN, RCC_APB2ENR_IOPFEN
																																					 RCC_APB2ENR_IOPGEN};
const GPIO_TypeDef_P GPIO[] =																												{GPIOA, GPIOB, GPIOC, GPIOD, GPIOE, GPIOF, GPIOG};
#endif //STM32F1
#define RCC_PORT																																RCC->APB2ENR
#elif defined(STM32F2)
const uint32_t RCC_GPIO[] =																												{RCC_AHB1ENR_GPIOAEN, RCC_AHB1ENR_GPIOBEN, RCC_AHB1ENR_GPIOCEN,
																																					 RCC_AHB1ENR_GPIODEN, RCC_AHB1ENR_GPIOEEN, RCC_AHB1ENR_GPIOFEN,
																																					 RCC_AHB1ENR_GPIOGEN, RCC_AHB1ENR_GPIOHEN, RCC_AHB1ENR_GPIOIEN};
const GPIO_TypeDef_P GPIO[] =																												{GPIOA, GPIOB, GPIOC, GPIOD, GPIOE, GPIOF, GPIOG, GPIOH, GPIOI};
#define RCC_PORT																																RCC->AHB1ENR
#endif


const IRQn_Type EXTI_SINGLE_VECTORS[] =																								{EXTI0_IRQn, EXTI1_IRQn, EXTI2_IRQn, EXTI3_IRQn, EXTI4_IRQn};

//port is not decoded on STM32f2xx: only one line can be set for port
void EXTI0_IRQHandler (void)
{
	if (EXTI->PR & (1 << 0))
	{
		EXTI->PR |= (1 << 0);
        if (__KERNEL->exti_handlers[0] != NULL)
            __KERNEL->exti_handlers[0](GPIO_A0);
	}
}

void EXTI1_IRQHandler (void)
{
	if (EXTI->PR & (1 << 1))
	{
		EXTI->PR |= (1 << 1);
        if (__KERNEL->exti_handlers[1] != NULL)
            __KERNEL->exti_handlers[1](GPIO_A1);
	}
}

void EXTI2_IRQHandler (void)
{
	if (EXTI->PR & (1 << 2))
	{
		EXTI->PR |= (1 << 2);
        if (__KERNEL->exti_handlers[2] != NULL)
            __KERNEL->exti_handlers[2](GPIO_A2);
	}
}

void EXTI3_IRQHandler (void)
{
	if (EXTI->PR & (1 << 3))
	{
		EXTI->PR |= (1 << 3);
        if (__KERNEL->exti_handlers[3] != NULL)
            __KERNEL->exti_handlers[3](GPIO_A3);
	}
}

void EXTI4_IRQHandler (void)
{
	if (EXTI->PR & (1 << 4))
	{
		EXTI->PR |= (1 << 4);
        if (__KERNEL->exti_handlers[4] != NULL)
            __KERNEL->exti_handlers[4](GPIO_A4);
	}
}

void EXTI9_5_IRQHandler(void)
{
	int i;
	for (i = 5; i <= 9; ++i)
	{
		if (EXTI->PR & (1 << i))
		{
			EXTI->PR |= (1 << i);
            if (__KERNEL->exti_handlers[i] != NULL)
                __KERNEL->exti_handlers[i](GPIO_A0 + i);
		}
	}
}

void EXTI15_10_IRQHandler(void)
{
	int i;
	for (i = 10; i <= 15; ++i)
	{
		if (EXTI->PR & (1 << i))
		{
			EXTI->PR |= (1 << i);
            if (__KERNEL->exti_handlers[i] != NULL)
                __KERNEL->exti_handlers[i](GPIO_A0 + i);
		}
	}
}

void gpio_enable_pin_power(GPIO_CLASS pin)
{
//    if (__KERNEL->used_pins[PORT(pin)]++ == 0)
		RCC_PORT |= RCC_GPIO[PORT(pin)];
}

void gpio_enable_pin(GPIO_CLASS pin, PIN_MODE mode)
{
	gpio_enable_pin_power(pin);

#if defined(STM32F1)
	if (PIN(pin) >= 8)
	{
		GPIO[PORT(pin)]->CRH &= ~(0xful << ((PIN(pin) - 8) * 4ul));
		GPIO[PORT(pin)]->CRH |= ((uint32_t)PIN_MODE_VALUE[(int)mode] << ((PIN(pin) - 8) * 4ul));
	}
	else
	{
		GPIO[PORT(pin)]->CRL &= ~(0xful << (PIN(pin) * 4ul));
		GPIO[PORT(pin)]->CRL |= ((uint32_t)PIN_MODE_VALUE[(int)mode] << (PIN(pin) * 4ul));
	}
	if (mode == PIN_MODE_IN_PULLUP)
		GPIO[PORT(pin)]->ODR |= 1 << PIN(pin);
	else
		GPIO[PORT(pin)]->ODR &= ~(1 << PIN(pin));
#elif defined(STM32F2)
	//in/out
	GPIO[PORT(pin)]->MODER &= ~(3 << (PIN(pin) * 2));
	switch (mode)
	{
	case PIN_MODE_OUT:
	case PIN_MODE_OUT_OD:
		//speed 100mhz
		GPIO[PORT(pin)]->OSPEEDR |= (3 << (PIN(pin) * 2));
		GPIO[PORT(pin)]->MODER |= (1 << (PIN(pin) * 2));
		break;
	default:
		break;
	}

	//out pp/od
	switch (mode)
	{
	case PIN_MODE_OUT:
		GPIO[PORT(pin)]->OTYPER &= ~(1 << PIN(pin));
		break;
	case PIN_MODE_OUT_OD:
		GPIO[PORT(pin)]->OTYPER |= 1 << PIN(pin);
		break;
	default:
		break;
	}

	//pullup/pulldown
	GPIO[PORT(pin)]->PUPDR &= ~(3 << (PIN(pin) * 2));
	switch (mode)
	{
	case PIN_MODE_IN_PULLUP:
		GPIO[PORT(pin)]->PUPDR |= (1 << (PIN(pin) * 2));
		break;
	case PIN_MODE_IN_PULLDOWN:
		GPIO[PORT(pin)]->PUPDR |= (2 << (PIN(pin) * 2));
		break;
	default:
		break;
	}
#endif
}

void gpio_disable_pin(GPIO_CLASS pin)
{
#if defined(STM32F1)
	if (PIN(pin) >= 8)
	{
		GPIO[PORT(pin)]->CRH &= ~(0xful << ((PIN(pin) - 8) * 4ul));
		GPIO[PORT(pin)]->CRH |= ((uint32_t)PIN_MODE_VALUE[(int)PIN_MODE_IN] << ((PIN(pin) - 8) * 4ul));
	}
	else
	{
		GPIO[PORT(pin)]->CRL &= ~(0xful << (PIN(pin) * 4ul));
		GPIO[PORT(pin)]->CRL |= ((uint32_t)PIN_MODE_VALUE[(int)PIN_MODE_IN] << (PIN(pin) * 4ul));
	}
	GPIO[PORT(pin)]->ODR &= ~(1 << PIN(pin));
#elif defined(STM32F2)
	GPIO[PORT(pin)]->MODER &= ~(3 << (PIN(pin) * 2));
	GPIO[PORT(pin)]->PUPDR &= ~(3 << (PIN(pin) * 2));
	GPIO[PORT(pin)]->OSPEEDR |= (3 << (PIN(pin) * 2));
#endif
    if (--__KERNEL->used_pins[PORT(pin)] == 0)
		RCC_PORT &= ~RCC_GPIO[PORT(pin)];
}

void gpio_enable_afio(GPIO_CLASS pin, AFIO_MODE mode)
{
	gpio_enable_pin_power(pin);

#if defined(STM32F1)
	if (PIN(pin) >= 8)
	{
		GPIO[PORT(pin)]->CRH &= ~(0xful << ((PIN(pin) - 8) * 4ul));
		GPIO[PORT(pin)]->CRH |= ((uint32_t)mode << ((PIN(pin) - 8) * 4ul));
	}
	else
	{
		GPIO[PORT(pin)]->CRL &= ~(0xful << (PIN(pin) * 4ul));
		GPIO[PORT(pin)]->CRL |= ((uint32_t)mode << (PIN(pin) * 4ul));
	}
	GPIO[PORT(pin)]->ODR &= ~(1 << PIN(pin));
#elif defined(STM32F2)
	GPIO[PORT(pin)]->AFR[PIN(pin) >= 8 ? 1 : 0] &= ~(0xful << ((PIN(pin) & 0x7ul) * 4ul));
	GPIO[PORT(pin)]->AFR[PIN(pin) >= 8 ? 1 : 0] |= ((uint32_t)(mode & 0xf) << ((PIN(pin) & 0x7ul) * 4ul));

	if (mode == AFIO_MODE_ANALOG)
	{
		GPIO[PORT(pin)]->MODER |= (3 << (PIN(pin) * 2));
	}
	else
	{
		GPIO[PORT(pin)]->MODER &= ~(3 << (PIN(pin) * 2));
		GPIO[PORT(pin)]->MODER |= (2 << (PIN(pin) * 2));
	}
	GPIO[PORT(pin)]->OSPEEDR |= (3 << (PIN(pin) * 2));
	GPIO[PORT(pin)]->OTYPER &= ~(1 << PIN(pin));

	//pull up/down
	GPIO[PORT(pin)]->PUPDR &= ~(3 << (PIN(pin) * 2));
	switch (mode)
	{
	case AFIO_MODE_FSMC_SDIO_OTG_FS_PULL_UP:
		GPIO[PORT(pin)]->PUPDR |= (1 << (PIN(pin) * 2));
		break;
	default:
		break;
	}
#endif
}

void gpio_set_pin(GPIO_CLASS pin, bool set)
{
#if defined(STM32F1)
	if (set)
		GPIO[PORT(pin)]->BSRR = 1 << PIN(pin);
	else
		GPIO[PORT(pin)]->BRR = 1 << PIN(pin);
#elif defined(STM32F2)
	if (set)
		GPIO[PORT(pin)]->BSRRL = 1 << PIN(pin);
	else
		GPIO[PORT(pin)]->BSRRH = 1 << PIN(pin);
#endif
}

bool gpio_get_pin(GPIO_CLASS pin)
{
	return GPIO[PORT(pin)]->IDR & (1 << PIN(pin)) ? true : false;
}

bool gpio_get_out_pin(GPIO_CLASS pin)
{
	return GPIO[PORT(pin)]->ODR & (1 << PIN(pin)) ? true : false;
}

void gpio_disable_jtag()
{
#if defined(STM32F1)
	afio_remap();
	AFIO->MAPR |= AFIO_MAPR_SWJ_CFG_DISABLE;
#elif defined(STM32F2)
	gpio_enable_pin(GPIO_A13, PIN_MODE_IN);
	gpio_enable_pin(GPIO_A14, PIN_MODE_IN);
	gpio_enable_pin(GPIO_A15, PIN_MODE_IN);
	gpio_enable_pin(GPIO_B3, PIN_MODE_IN);
	gpio_enable_pin(GPIO_B4, PIN_MODE_IN);
#endif
}

#if defined(STM32F1)
void afio_remap()
{
    if (__KERNEL->afio_remap_count++ == 0)
		RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;
}

void afio_unmap()
{
    if (--__KERNEL->afio_remap_count == 0)
		RCC->APB2ENR &= ~RCC_APB2ENR_AFIOEN;
}

#endif

void gpio_exti_enable(EXTI_CLASS exti, EXTI_MODE mode, EXTI_HANDLER callback, int priority)
{
	if (PIN(exti) < EXTI_LINES_COUNT)
	{
        __KERNEL->exti_handlers[PIN(exti)] = callback;
#if defined(STM32F1)
		gpio_enable_pin(exti, PIN_MODE_IN);
		AFIO->EXTICR[PIN(exti) / 4] &= ~(0xful << (uint32_t)(PIN(exti) & 3ul));
		AFIO->EXTICR[PIN(exti) / 4] |= ((uint32_t)PORT(exti) << (uint32_t)(PIN(exti) & 3ul));

		EXTI->IMR |= 1ul << PIN(exti);
		EXTI->EMR |= 1ul << PIN(exti);

		EXTI->RTSR &= ~(1ul << PIN(exti));
		EXTI->FTSR &= ~(1ul << PIN(exti));
		if (mode & EXTI_MODE_RISING)
			EXTI->RTSR |= (1ul << PIN(exti));
		if (mode & EXTI_MODE_FALLING)
			EXTI->FTSR |= (1ul << PIN(exti));
#elif defined(STM32F2)
		if (_syscfg_count++ == 0)
			RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
		//connect to event
		SYSCFG->EXTICR[PIN(exti) / 4] &= ~(0xf << ((PIN(exti) & 3) * 4));
		SYSCFG->EXTICR[PIN(exti) / 4] |= PORT(exti) << ((PIN(exti) & 3) * 4);

		// Configure EXTI Line
		if (mode == EXTI_MODE_RISING || mode == EXTI_MODE_BOTH)
			EXTI->RTSR |= 1 << PIN(exti);
		else
			EXTI->RTSR &= ~(1 << PIN(exti));
		if (mode == EXTI_MODE_FALLING || mode == EXTI_MODE_BOTH)
			EXTI->FTSR |= 1 << PIN(exti);
		else
			EXTI->FTSR &= ~(1 << PIN(exti));

		EXTI->IMR |= 1 << PIN(exti);
		EXTI->EMR |= 1 << PIN(exti);
#endif
		//enable irq, if needed
		if (PIN(exti) <= 4)
		{
			NVIC_EnableIRQ(EXTI_SINGLE_VECTORS[PIN(exti)]);
			NVIC_SetPriority(EXTI_SINGLE_VECTORS[PIN(exti)], priority);
		}
		else if (PIN(exti) <= 10)
		{
            if (__KERNEL->exti_5_9_active++ == 0)
			{
				NVIC_EnableIRQ(EXTI9_5_IRQn);
				NVIC_SetPriority(EXTI9_5_IRQn, priority);
			}
		}
		else // if (PIN(exti) <= 15)
		{
            if (__KERNEL->exti_10_15_active++ == 0)
			{
				NVIC_EnableIRQ(EXTI15_10_IRQn);
				NVIC_SetPriority(EXTI15_10_IRQn, priority);
			}
		}
	}
	else
        error(ERROR_NOT_SUPPORTED);
}

void gpio_exti_disable(EXTI_CLASS exti)
{
	if (PIN(exti) < EXTI_LINES_COUNT)
	{
		//disable irq, if needed
		if (PIN(exti)  <= 4)
			NVIC_DisableIRQ(EXTI_SINGLE_VECTORS[PIN(exti)]);
		else if (PIN(exti) <= 10)
		{
            if (--__KERNEL->exti_5_9_active == 0)
				NVIC_DisableIRQ(EXTI9_5_IRQn);
		}
		else // if (PIN(exti) <= 15)
		{
            if (--__KERNEL->exti_10_15_active == 0)
				NVIC_DisableIRQ(EXTI15_10_IRQn);
		}

#if defined(STM32F1)
		EXTI->IMR &= ~(1ul << PIN(exti));
		EXTI->EMR &= ~(1ul << PIN(exti));

		EXTI->RTSR &= ~(1ul << PIN(exti));
		EXTI->FTSR &= ~(1ul << PIN(exti));
#elif defined(STM32F2)
		if (--_syscfg_count == 0)
			RCC->APB2ENR &= ~RCC_APB2ENR_SYSCFGEN;

		EXTI->IMR &= ~(1 << PIN(exti));
		EXTI->EMR &= ~(1 << PIN(exti));
#endif

        __KERNEL->exti_handlers[PIN(exti)] = NULL;
	}
	else
        error(ERROR_NOT_SUPPORTED);
}
