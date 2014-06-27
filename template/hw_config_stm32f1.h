/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef HW_CONFIG_STM32_F1
#define HW_CONFIG_STM32_F1

//--------------------- USART hw config -----------------------------------------
//each bit for port
//#define USART_RX_DISABLE_MASK_DEF				(1 << UART_1)
#define USART_RX_DISABLE_MASK_DEF				(0)
#define USART_TX_DISABLE_MASK_DEF				(0)

#define USART_REMAP_MASK							(0)
//#define USART_REMAP_MASK							(1 << UART_1)

//USART1
#define USART1_TX_PIN								GPIO_A9
#define USART1_RX_PIN								GPIO_A10

/*
#define USART1_TX_PIN								GPIO_B6
#define USART1_RX_PIN								GPIO_B7
*/

//USART2
#define USART2_TX_PIN								GPIO_A2
#define USART2_RX_PIN								GPIO_A3

/*
#define USART2_TX_PIN								GPIO_D5
#define USART2_RX_PIN								GPIO_D6
*/

//USART3
#define USART3_TX_PIN								GPIO_B10
#define USART3_RX_PIN								GPIO_B11

/*
#define USART3_TX_PIN								GPIO_C10
#define USART3_RX_PIN								GPIO_C11
*/

/*
#define USART3_TX_PIN								GPIO_D8
#define USART3_RX_PIN								GPIO_D9
*/

//UART4
#define UART4_TX_PIN									GPIO_A0
#define UART4_RX_PIN									GPIO_A1

/*
#define UART4_TX_PIN									GPIO_C10
#define UART4_RX_PIN									GPIO_C11
*/

//USART6
#define USART6_TX_PIN								GPIO_C6
#define USART6_RX_PIN								GPIO_C7

/*
#define USART6_TX_PIN								GPIO_G9
#define USART6_RX_PIN								GPIO_G14
*/


#endif // HW_CONFIG_STM32_F1

