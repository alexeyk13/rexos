/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef STM32_CONFIG_H
#define STM32_CONFIG_H

//------------------------------ CORE ------------------------------------------------
//Sizeof CORE process stack. Adjust, if monolith UART/USB/Analog/etc is used
#define STM32_CORE_PROCESS_SIZE                 1024

#define STM32_DRIVERS_IPC_COUNT                 5
//UART driver is monolith. Enable for size, disable for perfomance
#define MONOLITH_UART                           1
#define MONOLITH_USB                            1

//------------------------------ POWER -----------------------------------------------
//save few bytes here
#define STM32_DECODE_RESET                      0
//0 meaning HSI. If not defined, 25MHz will be defined by default by ST lib
#define HSE_VALUE                               8000000
#define HSE_BYPASS                              0
//0 meaning HSE
#define LSE_VALUE                               32768
//STM32L0
#define MSI_RANGE                               6

//STM32F1
//#define PLL_MUL                                 6
//#define PLL_DIV                                 2
//STM32L0
#define PLL_MUL                                 12
#define PLL_DIV                                 3
//STM32F10X_CL only
// use PLL2 as clock source for main PLL. Set to 0 to disable
#define PLL2_DIV                                0
#define PLL2_MUL                                0

//STM32F2, STM32F4
#define PLL_M                                   0
#define PLL_N                                   0
#define PLL_P                                   0

#define STANDBY_WKUP                            STANDBY_WKUP_PIN2
//disable only for power saving if no EXTI or remap is used
#define SYSCFG_ENABLED                          1
//------------------------------ UART ------------------------------------------------
//size of every uart internal tx buf. Increasing this you will get less irq ans ipc calls, but faster processing
//remember, that process itself requires around 256 bytes
#define UART_TX_BUF_SIZE                        16
//generally UART is used as stdout/stdio, so fine-tuning is required only on hi load
#define UART_STREAM_SIZE                        32
//Sizeof UART process. Remember, that process itself requires around 450 bytes. Only for stand-alone UART driver
#define STM32_UART_PROCESS_SIZE                 410 + (50 + UART_TX_BUF_SIZE) * 1
//------------------------------ TIMER -----------------------------------------------
#define HPET_TIMER                              TIM_22
//only required if no STM32_RTC_DRIVER is set
#define SECOND_PULSE_TIMER                      TIM_2
//disable to save few bytes
#define TIMER_IO                                0
//------------------------------- ADC ------------------------------------------------
#define STM32_ADC_DRIVER                        1
//In L0 series - select HSI16 as clock source
#define STM32_ADC_ASYNCRONOUS_CLOCK             0
// Avg Slope, refer to datasheet
#define AVG_SLOPE                               4300
// temp at 25C in mV, refer to datasheet
#define V25_MV                                  1400

//------------------------------- DAC ------------------------------------------------
#define STM32_DAC_DRIVER                        1
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
//WDT module enable
#define STM32_WDT_DRIVER                        0
//------------------------------- RTC ------------------------------------------------
#define STM32_RTC_DRIVER                        1
//------------------------------- EEP ------------------------------------------------
#define STM32_EEP_DRIVER                        1
//increase for perfomance, decrease for memory saving. Must be 4 bytes aligned
#define STM32_EEPROM_BUF_SIZE                   20
//------------------------------- ETH ------------------------------------------------
#define STM32_ETH_PROCESS_SIZE                  512
#define STM32_ETH_IPC_COUNT                     5

//enable Pulse per second signal
#define STM32_ETH_PPS_OUT_ENABLE                0
//pin remapping for 105/107
#define STM32_ETH_REMAP                         0


#endif // STM32_CONFIG_H
