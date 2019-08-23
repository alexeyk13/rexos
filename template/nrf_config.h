/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: RL (jam_roma@yahoo.com)
*/

#ifndef NRF_CONFIG_H_
#define NRF_CONFIG_H_

//---------------------- fast drivers definitions -----------------------------------
#define NRF_ADC_DRIVER                          1
#define NRF_WDT_DRIVER                          0
#define NRF_FLASH_DRIVER                        1
#define NRF_SPI_DRIVER                          0
#define NRF_UART_DRIVER                         1
#define NRF_RTC_DRIVER                          1
#define NRF_TIMER_DRIVER                        1
#define NRF_RF_DRIVER                           1
#define NRF_RNG_DRIVER                          1
#define NRF_TEMP_DRIVER                         1
#define NRF_AES_ECB_DRIVER                      0
//------------------------------ CORE ------------------------------------------------


//------------------------------ PINBOARD --------------------------------------------
// If pin connected to VCC - set HIGH_LO_ACTIVE flag to 0
// If pin connected to GND - set HIGH_LO_ACTIVE flag to 1
#define PINBOARD_HIGH_LOW_ACTIVE                1

//------------------------------- UART -----------------------------------------------
//disable for some memory saving if not blocking IO is required
#define UART_IO_MODE_SUPPORT                    0
//default values for IO mode
#define UART_CHAR_TIMEOUT_US                    10000000
#define UART_INTERLEAVED_TIMEOUT_US             10000
//size of every uart internal buf. Increasing this you will get less irq ans ipc calls, but faster processing
#define UART_BUF_SIZE                           16
//generally UART is used as stdout/stdio, so fine-tuning is required only on hi load
#define UART_STREAM_SIZE                        32
//------------------------------ POWER -----------------------------------------------
#define HFCLK                                   16000000
/* Low Frequency OSC. 0 - not use LFCLK */
#define LFCLK                                   32768
#define LFCLK_SOURCE                            CLOCK_LFCLKSRC_SRC_Synth

//save few bytes here
#define NRF_DECODE_RESET                        1
/* keep SRAM during system OFF */
#define NRF_SRAM_RETENTION_ENABLE               0
/* POWER CONFIG for SRAM blocks */
#define NRF_SRAM_POWER_CONFIG                   0

#if (NRF_SRAM_POWER_CONFIG)
/* Count of RAM block should be checked with NUMRAMBLOCK register */
/* RAM0 is used for kernel always. Don't change this settings */
#define NRF_RAM0_ENABLE                         1
/* user data SRAM pool */
#define NRF_RAM1_ENABLE                         1
/* for NRF52 */
#define NRF_RAM2_ENABLE                         0
#define NRF_RAM3_ENABLE                         0
#define NRF_RAM4_ENABLE                         0
#define NRF_RAM5_ENABLE                         0
#define NRF_RAM6_ENABLE                         0
#define NRF_RAM7_ENABLE                         0
#endif // NRF_SRAM_POWER_CONFIG
//------------------------------ TIMER -----------------------------------------------
#if (NRF_RTC_DRIVER)
#define KERNEL_SECOND_RTC                       0
#endif //

//Don't change this if you are not sure. Unused if RTC is configured
#if (KERNEL_SECOND_RTC)
#define SECOND_RTC                              RTC_0
#define SECOND_RTC_CHANNEL                      RTC_CC0
#else
#define SECOND_CHANNEL                          TIMER_CC1
#endif // KERNEL_SECOND_RTC
// Use another channel for HPET
#define HPET_TIMER                              TIMER_0
#define HPET_CHANNEL                            TIMER_CC0
//------------------------------- ADC ------------------------------------------------
/* We can measure different voltage range on the input with adjusting the prescaling of the ADC */
/* With e.g. 1/3 prescaling and VBG reference we have range 0V - 3.6V on the input */
#define NRF_ADC_INPUT                           ADC_CONFIG_INPSEL_AnalogInputOneThirdPrescaling
#define NRF_ADC_REFERENCE                       ADC_CONFIG_REFSEL_VBG
#define NRF_ADC_RESOLUTION                      ADC_CONFIG_RES_10bit
//------------------------------- WDT ------------------------------------------------
#define WDT_TIMEOUT_S                           10
//------------------------------ FLASH -----------------------------------------------
//protect user code if VFS is allocated on flash. Must be page-aligned (512)
#define NRF_FLASH_USER_CODE_SIZE                (128 * 1024)
//------------------------------ DEBUG -----------------------------------------------
#define DBG_CONSOLE                                 UART_0
#define DBG_CONSOLE_BAUD                            115200
#define DBG_CONSOLE_TX_PIN                          P20

#define TEST_ROUNDS                                 10000
#endif /* NRF_CONFIG_H_ */
