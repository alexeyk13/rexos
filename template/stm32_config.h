/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef STM32_CONFIG_H
#define STM32_CONFIG_H

//------------------------------ POWER -----------------------------------------------
//0 meaning HSI. If not defined, 25MHz will be defined by default by ST lib
#define HSE_VALUE                               25000000

//STM32F1
#define PLL_MUL                                 7
#define PLL_DIV                                 2
//STM32F10X_CL only
// use PLL2 as clock source for main PLL. Set to 0 to disable
#define PLL2_DIV                                10
#define PLL2_MUL                                8

//STM32F2, STM32F4
#define PLL_M                                   0
#define PLL_N                                   0
#define PLL_P                                   0
//------------------------------ UART ------------------------------------------------
#define UART1_TX_PORT                           GPIOA
#define UART1_TX_PIN                            9


#endif // STM32_CONFIG_H
