/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: Alexey E. Kramarenko (alexeyk13@yandex.ru)
*/

#ifndef STM32_CONFIG_H
#define STM32_CONFIG_H

//---------------------- fast drivers definitions -----------------------------------
#define STM32_ADC_DRIVER                        0
#define STM32_DAC_DRIVER                        0
#define STM32_WDT_DRIVER                        0
#define STM32_EEP_DRIVER                        0
#define STM32_FLASH_DRIVER                      0
#define STM32_I2C_DRIVER                        0
#define STM32_UART_DRIVER                       1
#define STM32_RTC_DRIVER                        0
#define STM32_USB_DRIVER                        1
#define STM32_CAN_DRIVER                        0
#define STM32_ETH_DRIVER                        0
#define STM32_SDMMC_DRIVER                      0
//------------------------------ CORE ------------------------------------------------
//disable only for power saving if no EXTI or remap is used
#define SYSCFG_ENABLED                          1
//stupid F1 series only. Remap mask. See datasheet
#define STM32F1_MAPR                            (AFIO_MAPR_USART2_REMAP)
#define IO_DATA_ALIGN                           4  // 32 for stm32H7 if use DMA
//------------------------------- UART -----------------------------------------------
//in any half duplex mode
#define STM32_UART_DISABLE_ECHO                 0
//------------------------------ POWER -----------------------------------------------
//save few bytes here
#define STM32_DECODE_RESET                      0
//0 meaning HSI. If not defined, 25MHz will be defined by default by ST lib
#define HSE_VALUE                               0
#define HSE_BYPASS                              0
//0 meaning HSE
#define LSE_VALUE                               0
//STM32L0
#define MSI_RANGE                               6

//STM32F1, F0
#define PLL_MUL                                 12
//STM32F1
#define PLL_DIV                                 2
//STM32L0
//#define PLL_MUL                                 12
//#define PLL_DIV                                 3
//STM32F10X_CL only
// use PLL2 as clock source for main PLL. Set to 0 to disable
#define PLL2_DIV                                0
#define PLL2_MUL                                0

//STM32F2, STM32F4, STM32H7    PLL clk = (PLL_N + 1) * ((clock source)/PLL_M) / ( PLL_P + 1)
#define PLL_M                                   1 // 2M <= ((clock source)/PLL_M) <= 16M
#define PLL_N                                   59
#define PLL_P                                   0
#define PLL_Q                                   9 

//---- only for  STM32H7
#define PLL2_N                                   49
#define PLL2_P                                   20
#define PLL2_Q                                   20
#define PLL2_R                                   3  // 100M

#define PLL3_N                                   59
#define PLL3_P                                   0
#define PLL3_Q                                   9
#define PLL3_R                                   9

//STM32H7  LDO voltage   0    1    2   3
//         MAX CLOCK    480  400  300 200
#define VOS_VALUE                               0  // for rev.Y VOS0 prohibit see errata
#define EXT_VCORE                               0  // STM32H7: 0 - use internal LDO  1 - external supply

#define USB_CLOCK_SRC                           USB_CLOCK_SRC_PLL1_Q          // 48M for USB FS, 60M for USB HS
#define SDMMC_CLOCK_SRC                         SDMMC_CLOCK_SRC_PLL2_R        // max 250 MHz


#define STANDBY_WKUP                            0
//------------------------------ SDMMC -----------------------------------------------
#define STM32_SDMMC_MAX_CLOCK                   10000000

//------------------------------ TIMER -----------------------------------------------
#define HPET_TIMER                              TIM_14
//only required if no STM32_RTC_DRIVER is set
#define SECOND_PULSE_TIMER                      TIM_15
//disable to save few bytes
#define TIMER_IO                                1
//------------------------------- ADC ------------------------------------------------
//In L0 series - select HSI16 as clock source
#define STM32_ADC_ASYNCRONOUS_CLOCK             0
// Avg Slope, refer to datasheet
#define AVG_SLOPE                               4300
// temp at 25C in mV, refer to datasheet
#define V25_MV                                  1400

//------------------------------- DAC ------------------------------------------------
#define DAC_BOFF                                0

//DAC streaming support with DMA. Can be disabled for flash saving
#define DAC_STREAM                              0
//For F1 only
#define DAC_DUAL_CHANNEL                        0
//Decrease for memory saving, increase if you have data underflow.
//Generally you will need fifo enough to hold samples at least for 30-50us, depending on core frequency.
//Size in samples
#define DAC_DMA_FIFO_SIZE                       128
//Turn on for DAC data underflow debug.
#define DAC_DEBUG                               0

//------------------------------- USB ------------------------------------------------
//Maximum packet size for USB.
//Must be 8 for Low-speed devices
//Full speed: 64 if no isochronous transfers, else  1024
//High speed(STM32F2+): 64 if no high-speed bulk transfers, 512 in other case. 1024 in case of isochronous or high-speed interrupts
#define STM32_USB_MPS                           64
//Sizeof USB process stack. Remember, that process itself requires around 512 bytes
#define STM32_USB_PROCESS_SIZE                  600
//------------------------------- WDT ------------------------------------------------
//if set by STM32 Option Bits, WDT is started by hardware on power-up
#define HARDWARE_WATCHDOG                       0
//------------------------------ FLASH -----------------------------------------------
//protect user code if VFS is allocated on flash. Must be sector aligned
#define STM32_FLASH_USER_CODE_SIZE              (32 * 1024)
//------------------------------- I2C ------------------------------------------------
#define I2C_DEBUG                               0

#endif // STM32_CONFIG_H
