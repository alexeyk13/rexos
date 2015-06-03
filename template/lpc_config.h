/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef LPC_CONFIG_H
#define LPC_CONFIG_H

//------------------------------------- CORE ----------------------------------------------
//Sizeof CORE process stack. Adjust, if monolith UART/USB/Analog/etc is used
#define LPC_CORE_PROCESS_SIZE               780
#define LPC_DRIVERS_IPC_COUNT               10

//UART driver is monolith. Enable for size, disable for perfomance
#define MONOLITH_UART                       1
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
//Don't change this if you are not sure. Unused if RTC is configured
#define SECOND_CHANNEL                      TIMER_CHANNEL0
#define HPET_CHANNEL                        TIMER_CHANNEL1

//------------------------------------- EEPROM --------------------------------------------
#define LPC_EEPROM_DRIVER                   1

//increase for perfomance, decrease for memory saving. Must be 4 bytes aligned.
#define LPC_EEPROM_BUF_SIZE                 20
//-------------------------------------- GPIO ---------------------------------------------
#define GPIO_HYSTERESIS                     1

//-------------------------------------- UART ---------------------------------------------
#define UART_STREAM_SIZE                    32

#define LPC_UART_PROCESS_SIZE               512
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
#define LPC_I2C_DRIVER                      1
#define LPC_I2C_PROCESS_SIZE                480

//in some application slave devices may hang bus for infinite time.
//Set this value greater than 0 to solve problem. Soft timers is required.
#define LPC_I2C_TIMEOUT_MS                  3000

#endif //LPC_CONFIG_H
