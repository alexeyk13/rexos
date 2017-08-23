/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef LPC_CONFIG_H
#define LPC_CONFIG_H

//---------------------- fast drivers definitions -----------------------------------
#define LPC_I2C_DRIVER                      0
#define LPC_SDMMC_DRIVER                    0
#define LPC_EEPROM_DRIVER                   0
#define LPC_UART_DRIVER                     1
#define LPC_USB_DRIVER                      1
#define LPC_FLASH_DRIVER                    0
#define LPC_ETH_DRIVER                      1

//------------------------------------- power ---------------------------------------------
//save few bytes here
#define LPC_DECODE_RESET                    0

#define HSE_VALUE                           12000000
#define LSE_VALUE                           0
#define HSE_BYPASS                          0
#define PLL_M                               15
#define PLL_N                               1
//P == 1 means bypass post divider
#define PLL_P                               1

//M3 clock divider for HI/LO power modes. Base clock is PLL1 out.
//HI divider generally operates on PLL1 out.
#define POWER_HI_DIVIDER                    1
//MID divider is used when HI clock > 110MHz for step-up clock adjustment, required by datasheet
//MID divider must be in range 90MHz<=MID<=110MHz. If HI clock is less or equal 110MHz, mid divider
//can be disabled by setting to 0.
#define POWER_MID_DIVIDER                   2
#define POWER_LO_DIVIDER                    6

//------------------------------------- timer ---------------------------------------------
//Second pulse generation timer. Use 32 bit for fine tune.
#define SECOND_TIMER                        TIMER0
//Don't change this if you are not sure. Unused if RTC is configured
#define SECOND_CHANNEL                      TIMER_CHANNEL0
#define HPET_CHANNEL                        TIMER_CHANNEL1

//-------------------------------------- USB ----------------------------------------------
//Maximum packet size for USB.
//Full speed: 64 if no isochronous transfers, else max 1024
//must be 64 byte align for buffer allocation
#define LPC_USB_MPS                         512

//indirect values for LPC18xx. Read details from datasheet
#define USBPLL_M                            0x06167FFA
//not used in LPC18xx
#define USBPLL_P                            0x0

//LPC11uxx: Control pullap of DP line through SCONNECT pin. Only for full-speed devices.
#define USB_SOFT_CONNECT                    1
//Use main or dedicated usb pll for USBCLK (only for LPC11Uxx)
#define USB_DEDICATED_PLL                   0

//LPC18xx: IDIV_A is used for clock divide for USB1 (if internal PHY is used). Output must be 60 MHz
#define IDIV_A                              3
#define USB1_ULPI                           0
//enable if both USB_0 and USB_1 used same time
#define LPC_USB_USE_BOTH                    0
//-------------------------------------- I2C ----------------------------------------------
//in some application slave devices may hang bus for infinite time.
//Set this value greater than 0 to solve problem. Soft timers is required.
#define LPC_I2C_TIMEOUT_MS                  3000

//-------------------------------------- SDMMC ------------------------------------------------
//each is 7 KB
#define LPC_SDMMC_DESCR_COUNT               9

//-------------------------------------- ETH ----------------------------------------------
//MII/RMII
#define LPC_ETH_MII                         0

#endif //LPC_CONFIG_H
