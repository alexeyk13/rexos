/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef LPC_CONFIG_H
#define LPC_CONFIG_H

//------------------------------------- CORE ----------------------------------------------
//Sizeof CORE process stack. Adjust, if monolith UART/USB/Analog/etc is used
#define LPC_CORE_STACK_SIZE                 580
#define LPC_DRIVERS_IPC_COUNT               3

//UART driver is monolith. Enable for size, disable for perfomance
#define MONOLITH_UART                       1
#define MONOLITH_I2C                        1
#define MONOLITH_USB                        1
//------------------------------------- power ---------------------------------------------
#define HSE_VALUE                           12000000
#define RTC_VALUE                           0
#define HSE_BYPASS                          0
#define PLL_M                               4
#define PLL_P                               2

//Use main or dedicated usb pll for USBCLK
#define USB_DEDICATED_PLL                   0
#define USBPLL_M                            4
#define USBPLL_P                            2

//------------------------------------- timer ---------------------------------------------
//Second pulse generation timer. Use 32 bit for fine tune.
#define SECOND_TIMER                        TC32B0
//Don't change this if you are not sure. Can't be channel 0, cause channel 0 is used for
//second pulse timer. Unused if RTC is configured
#define HPET_CHANNEL                        TIMER_CHANNEL1

//------------------------------------- EEPROM --------------------------------------------
#define LPC_EEPROM_DRIVER                   1

//increase for perfomance, decrease for memory saving. Must be 4 bytes aligned.
#define LPC_EEPROM_BUF_SIZE                 20
//-------------------------------------- GPIO ---------------------------------------------
#define GPIO_HYSTERESIS                     1
//GPIO pin constants for specific module. If not used can save 32 bytes per block
#define GPIO_UART                           1
#define GPIO_I2C                            1
//SCLK/RTS for PIO0_17
#define GPIO_PIO0_17_SCLK                   1
//TXD/DTR for PIO1_13
#define GPIO_PIO1_13_TXD                    1
//RXD/DSR for PIO1_14
#define GPIO_PIO1_14_RXD                    1

//-------------------------------------- UART ---------------------------------------------
//Use UART as default stdio
#define UART_STDIO                          1
//PIN_UNUSED can be also set.
#define UART_STDIO_PORT                     UART_0
#define UART_STDIO_TX                       PIO0_19
#define UART_STDIO_RX                       PIN_UNUSED
#define UART_STDIO_BAUD                     115200
#define UART_STDIO_DATA_BITS                8
#define UART_STDIO_PARITY                   'N'
#define UART_STDIO_STOP_BITS                1

#define LPC_UART_STACK_SIZE                 512
//size of every uart internal tx buf. Increasing this you will get less irq ans ipc calls, but faster processing
#define UART_TX_BUF_SIZE                    16

//-------------------------------------- USB ----------------------------------------------
//Maximum packet size for USB.
//Full speed: 64 if no isochronous transfers, else max 1024
//must be 64 byte align for buffer allocation
#define LPC_USB_MPS                         64
//Sizeof USB process stack. Remember, that process itself requires around 512 bytes
#define LPC_USB_PROCESS_SIZE                550

//Control pullap of DP line through SCONNECT pin
#define USB_SOFT_CONNECT                    1
//-------------------------------------- I2C ----------------------------------------------
#define LPC_I2C_PROCESS_SIZE                480

#endif //LPC_CONFIG_H
