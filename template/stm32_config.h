/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef STM32_CONFIG_H
#define STM32_CONFIG_H

//------------------------------ POWER -----------------------------------------------
//0 meaning HSI. If not defined, 25MHz will be defined by default by ST lib
#define HSE_VALUE                               24000000
#define LSE_VALUE                               32768

//STM32F1
#define PLL_MUL                                 6
#define PLL_DIV                                 2
//STM32F10X_CL only
// use PLL2 as clock source for main PLL. Set to 0 to disable
#define PLL2_DIV                                0
#define PLL2_MUL                                0

//STM32F2, STM32F4
#define PLL_M                                   0
#define PLL_N                                   0
#define PLL_P                                   0
//------------------------------ UART ------------------------------------------------
//Use UART as default stdio
#define UART_STDIO                              1
//PIN_DEFAULT and PIN_UNUSED can be also set.
#define UART_STDIO_PORT                         UART_2
#define UART_STDIO_TX                           D5
#define UART_STDIO_RX                           D6
#define UART_STDIO_BAUD                         115200
#define UART_STDIO_DATA_BITS                    8
#define UART_STDIO_PARITY                       'N'
#define UART_STDIO_STOP_BITS                    1

//size of UART process. You will need to increase this, if you have many uarts opened at same time
#define UART_PROCESS_SIZE                       1024
//size of every uart internal tx buf. Increasing this you will get less irq ans ipc calls, but faster processing
//remember, that process itself requires around 256 bytes
#define UART_TX_BUF_SIZE                        32
//------------------------------ TIMER -----------------------------------------------
#define HPET_TIMER                               TIM_2
#define TIMER_SOFT_RTC                           0
#define SECOND_PULSE_TIMER                       TIM_3
//------------------------------- ADC ------------------------------------------------
// Avg Slope, refer to datasheet
#define AVG_SLOPE                                4300
// temp at 25C in mV, refer to datasheet
#define V25_MV                                   1400
// Vref in mV, set to 3300, if used internal
#define ADC_VREF                                 3300
//------------------------------- USB ------------------------------------------------
//Maximum packet size for USB.
//Must be 8 for Low-speed devices
//Full speed: 64 if no isochronous transfers, else  1024
//High speed(STM32F2+): 64 if no high-speed bulk transfers, 512 in other case. 1024 in case of isochronous or high-speed interrupts
#define STM32_USB_MPS                            64


#endif // STM32_CONFIG_H
