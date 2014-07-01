/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef GPIO_STM32_H
#define GPIO_STM32_H

/*
	stm32fxxx-specific hardware configuration
 */

#include "dev.h"
#include "gpio.h"

typedef enum {
#if defined(STM32F1)
	AFIO_MODE_ANALOG = 0x0,
	AFIO_MODE_PUSH_PULL = 0xb,
	AFIO_MODE_OD = 0xf
#elif defined(STM32F2)
	AFIO_MODE_SYS = 0,
	AFIO_MODE_TIM1_2,
	AFIO_MODE_TIM3_4_5,
	AFIO_MODE_TIM8_9_10_11,
	AFIO_MODE_I2C,
	AFIO_MODE_SPI1_2_I2S2,
	AFIO_MODE_SPI3_I2S3,
	AFIO_MODE_USART1_2_3,
	AFIO_MODE_UART_4_5_USART_6,
	AFIO_MODE_CAN1_2_TIM12_13_14,
	AFIO_MODE_OTG,
	AFIO_MODE_ETH,
	AFIO_MODE_FSMC_SDIO_OTG_FS,
	AFIO_MODE_DCMI,
	AFIO_MODE_14,
	AFIO_MODE_EVENTOUT,
	AFIO_MODE_ANALOG = 0x10,
	AFIO_MODE_FSMC_SDIO_OTG_FS_PULL_UP = 0x1c
#endif
}AFIO_MODE;

typedef GPIO_TypeDef* GPIO_TypeDef_P;
extern const GPIO_TypeDef_P GPIO[];

//stm32 specific, for internal use for drivers
void gpio_enable_afio(GPIO_CLASS pin, AFIO_MODE mode);
void gpio_disable_jtag();

#if defined(STM32F1)
void afio_remap();
void afio_unmap();
#endif

#endif // GPIO_STM32_H
