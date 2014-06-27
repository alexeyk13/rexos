/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef HW_CONFIG_STM32_F2
#define HW_CONFIG_STM32_F2

//--------------------- RCC hw config -----------------------------------------
//choose one of for pll-q tuning
//for USB-FS
//#define PLL_Q_48MHZ
//for SDIO 25MHZ
//#define PLL_Q_25MHZ
#define PLL_Q_DONT_CARE

//if system stucks during flashing, turn this on, than back off for speed
//#define UNSTUCK_FLASH

//--------------------- USART hw config -----------------------------------------
//each bit for port
#define USART_RX_DISABLE_MASK_DEF				(1 << 0)
//#define USART_RX_DISABLE_MASK_DEF				(0)
#define USART_TX_DISABLE_MASK_DEF				(0)
//USART1
/*
#define USART1_TX_PIN								GPIO_A9
#define USART1_RX_PIN								GPIO_A10
*/

#define USART1_TX_PIN								GPIO_B6
#define USART1_RX_PIN								GPIO_B7

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

//--------------------- I2C hw config -----------------------------------------
//I2C1
#define I2C1_SCL_PIN									GPIO_B6
#define I2C1_SDA_PIN									GPIO_B7

/*
#define I2C1_SCL_PIN									GPIO_B8
#define I2C1_SDA_PIN									GPIO_B9
*/

//I2C2
#define I2C2_SCL_PIN									GPIO_B10
#define I2C2_SDA_PIN									GPIO_B11

/*
#define I2C2_SCL_PIN									GPIO_F0
#define I2C2_SDA_PIN									GPIO_F1
*/

/*
#define I2C2_SCL_PIN									GPIO_H4
#define I2C2_SDA_PIN									GPIO_H5
*/

//I2C3
#define I2C3_SCL_PIN									GPIO_A8
#define I2C3_SDA_PIN									GPIO_C9

/*
#define I2C3_SCL_PIN									GPIO_H7
#define I2C3_SDA_PIN									GPIO_H8
*/
//--------------------- USB hw config -----------------------------------------
//pin remaps for ULPI

#define HS_NXT_PIN									GPIO_C3
#define HS_DIR_PIN									GPIO_C2

/*
#define HS_NXT_PIN									GPIO_H4
#define HS_DIR_PIN									GPIO_I11
*/

#define USB_DMA_ENABLED

//ST device-library specific
#define USB_ADDRESS_POST_PROCESSING				0

#ifdef USB_DMA_ENABLED
#define USB_DATA_ALIGN								8
#else
#define USB_DATA_ALIGN								4
#endif

//due to bug in STM, core reset may halt system if USB is not plugged
//usually core reset only needed on dynamic core changing
#define USB_STM_RESET_CORE							0
//--------------------- SDIO hw config ---------------------------------------
#define SDIO_DMA_ENABLED
#define SDIO_DMA_STREAM								DMA_STREAM_3
//#define SDIO_DMA_STREAM								DMA_STREAM_6

//--------------------- hw rand enable ---------------------------------------
#define HW_RAND

#endif // HW_CONFIG_STM32_F2

