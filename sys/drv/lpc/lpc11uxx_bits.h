/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef LPC11UXX_BITS_H
#define LPC11UXX_BITS_H

/******************************************************************************/
/*                                                                            */
/*                            SYSCON                                          */
/*                                                                            */
/******************************************************************************/

/**********  Bit definition for SYSMEMREMAP register  *************************/
#define SYSCON_SYSMEMREMAP_BOOTLOADER                  (0 << 0)
#define SYSCON_SYSMEMREMAP_USER_RAM                    (1 << 0)
#define SYSCON_SYSMEMREMAP_USER_FLASH                  (2 << 0)

/**********  Bit definition for PRESETCTRL register  **************************/
#define SYSCON_PRESETCTRL_SSP0_RST_N_POS               0
#define SYSCON_PRESETCTRL_I2C0_RST_N_POS               1
#define SYSCON_PRESETCTRL_SSP1_RST_N_POS               2
#define SYSCON_PRESETCTRL_I2C1_RST_N_POS               3
#define SYSCON_PRESETCTRL_FRG_RST_N_POS                4
#define SYSCON_PRESETCTRL_USART1_RST_N_POS             5
#define SYSCON_PRESETCTRL_USART2_RST_N_POS             6
#define SYSCON_PRESETCTRL_USART3_RST_N_POS             7
#define SYSCON_PRESETCTRL_USART4_RST_N_POS             8
#define SYSCON_PRESETCTRL_SCT0_RST_N_POS               9
#define SYSCON_PRESETCTRL_SCT1_RST_N_POS               10

/**********  Bit definition for SYSPLLCTRL register  **************************/
#define SYSCON_SYSPLLCTRL_MSEL_POS                     0
#define SYSCON_SYSPLLCTRL_MSEL_MASK                    (0x1f << 0)

#define SYSCON_SYSPLLCTRL_PSEL_POS                     5
#define SYSCON_SYSPLLCTRL_PSEL_MASK                    (3 << 5)

/**********  Bit definition for SYSPLLSTAT register  **************************/
#define SYSCON_SYSPLLSTAT_LOCK                         (1 << 0)

/**********  Bit definition for USBPLLCTRL register  **************************/
#define SYSCON_USBPLLCTRL_MSEL_POS                     0
#define SYSCON_USBPLLCTRL_PSEL_POS                     5

/**********  Bit definition for USBPLLSTAT register  **************************/
#define SYSCON_USBPLLSTAT_LOCK                         (1 << 0)

/**********  Bit definition for RTCOSCCTRL register  **************************/
#define SYSCON_RTCOSCCTRL_RTCOSCEN                     (1 << 0)

/**********  Bit definition for SYSOSCCTRL register  **************************/
#define SYSCON_SYSOSCCTRL_BYPASS                       (1 << 0)
#define SYSCON_SYSOSCCTRL_FREQRANGE                    (1 << 1)

/**********  Bit definition for WDTOSCCTRL register  **************************/
#define SYSCON_WDTOSCCTRL_DIVSEL_POS                   0
#define SYSCON_WDTOSCCTRL_DIVSEL_MASK                  (0x1f << 0)

#define SYSCON_WDTOSCCTRL_FREQSEL_POS                  5
#define SYSCON_WDTOSCCTRL_FREQSEL_MASK                 (0xf << 5)

/**********  Bit definition for SYSRSTSTAT register  **************************/
#define SYSCON_SYSRSTSTAT_POR                          (1 << 0)
#define SYSCON_SYSRSTSTAT_EXTRST                       (1 << 1)
#define SYSCON_SYSRSTSTAT_WDT                          (1 << 2)
#define SYSCON_SYSRSTSTAT_BOD                          (1 << 3)
#define SYSCON_SYSRSTSTAT_SYSRST                       (1 << 4)

/**********  Bit definition for SYSPLLCLKSEL register  ***********************/
#define SYSCON_SYSPLLCLKSEL_IRC                        (0 << 0)
#define SYSCON_SYSPLLCLKSEL_SYSOSC                     (1 << 0)
#define SYSCON_SYSPLLCLKSEL_RTCOSC                     (3 << 0)

/**********  Bit definition for SYSPLLCLKUEN register  ***********************/
#define SYSCON_SYSPLLCLKUEN_ENA                        (1 << 0)

/**********  Bit definition for USBPLLCLKSEL register  ***********************/
#define SYSCON_USBPLLCLKSEL_IRC                        (0 << 0)
#define SYSCON_USBPLLCLKSEL_SYSOSC                     (1 << 0)

/**********  Bit definition for USBPLLCLKUEN register  ***********************/
#define SYSCON_USBPLLUEN_ENA                           (1 << 0)

/**********  Bit definition for MAINCLKSEL register  *************************/
#define SYSCON_MAINCLKSEL_IRC                          (0 << 0)
#define SYSCON_MAINCLKSEL_PLLSRC                       (1 << 0)
#define SYSCON_MAINCLKSEL_WDT                          (2 << 0)
#define SYSCON_MAINCLKSEL_PLL                          (3 << 0)

/**********  Bit definition for MAINCLKUEN register  *************************/
#define SYSCON_MAINCLKUEN_ENA                          (1 << 0)

/**********  Bit definition for SYSAHBCLKCTRL register  **********************/
#define SYSCON_SYSAHBCLKCTRL_SYS_POS                   0
#define SYSCON_SYSAHBCLKCTRL_ROM_POS                   1
#define SYSCON_SYSAHBCLKCTRL_RAM0_POS                  2
#define SYSCON_SYSAHBCLKCTRL_FLASHREG_POS              3
#define SYSCON_SYSAHBCLKCTRL_FLASHARRAY_POS            4
#define SYSCON_SYSAHBCLKCTRL_I2C0_POS                  5
#define SYSCON_SYSAHBCLKCTRL_GPIO_POS                  6
#define SYSCON_SYSAHBCLKCTRL_CT16B0_POS                7
#define SYSCON_SYSAHBCLKCTRL_CT16B1_POS                8
#define SYSCON_SYSAHBCLKCTRL_CT32B0_POS                9
#define SYSCON_SYSAHBCLKCTRL_CT32B1_POS                10
#define SYSCON_SYSAHBCLKCTRL_SSP0_POS                  11
#define SYSCON_SYSAHBCLKCTRL_USART0_POS                12
#define SYSCON_SYSAHBCLKCTRL_ADC_POS                   13
#define SYSCON_SYSAHBCLKCTRL_USB_POS                   14
#define SYSCON_SYSAHBCLKCTRL_WWDT_POS                  15
#define SYSCON_SYSAHBCLKCTRL_IOCON_POS                 16
#define SYSCON_SYSAHBCLKCTRL_SSP1_POS                  18
#define SYSCON_SYSAHBCLKCTRL_PINT_POS                  19
#define SYSCON_SYSAHBCLKCTRL_USART1_POS                20
#define SYSCON_SYSAHBCLKCTRL_USART2_POS                21
#define SYSCON_SYSAHBCLKCTRL_USART3_4_POS              22
#define SYSCON_SYSAHBCLKCTRL_GROUP0INT_POS             23
#define SYSCON_SYSAHBCLKCTRL_GROUP1INT_POS             24
#define SYSCON_SYSAHBCLKCTRL_I2C1_POS                  25
#define SYSCON_SYSAHBCLKCTRL_RAM1_POS                  26
#define SYSCON_SYSAHBCLKCTRL_USBRAM_POS                27
#define SYSCON_SYSAHBCLKCTRL_CRC_POS                   28
#define SYSCON_SYSAHBCLKCTRL_DMA_POS                   29
#define SYSCON_SYSAHBCLKCTRL_RTC_POS                   30
#define SYSCON_SYSAHBCLKCTRL_SCT0_1_POS                31

/**********  Bit definition for USBCLKSEL register  **************************/
#define SYSCON_USBCLKSEL_PLL                           (0 << 0)
#define SYSCON_USBCLKSEL_MAIN                          (1 << 0)

/**********  Bit definition for USBCLKUEN register  **************************/
#define SYSCON_USBCLKUEN_ENA                           (1 << 0)

/**********  Bit definition for CLKOUTSEL register  **************************/
#define SYSCON_CLKOUTSEL_IRC                           (0 << 0)
#define SYSCON_CLKOUTSEL_SYSOSC                        (1 << 0)
#define SYSCON_CLKOUTSEL_WDT                           (2 << 0)
#define SYSCON_CLKOUTSEL_MAIN                          (3 << 0)

/**********  Bit definition for CLKOUTUEN register  **************************/
#define SYSCON_CLKOUT_ENA                              (1 << 0)

/**********  Bit definition for EXTTRACECMD register  ************************/
#define SYSCON_EXTTRACECMD_START                       (1 << 0)
#define SYSCON_EXTTRACECMD_STOP                        (1 << 1)

/**********  Bit definition for BODCTR register  *****************************/
#define SYSCON_BODCTR_BODRSTLEV_POS                    0
#define SYSCON_BODCTR_BODRSTLEV_MASK                   (3 << 0)
#define SYSCON_BODCTR_BODRSTLEV_LEVEL0                 (0 << 0)
#define SYSCON_BODCTR_BODRSTLEV_LEVEL1                 (1 << 0)
#define SYSCON_BODCTR_BODRSTLEV_LEVEL2                 (2 << 0)
#define SYSCON_BODCTR_BODRSTLEV_LEVEL3                 (3 << 0)

#define SYSCON_BODCTR_BODINTVAL_POS                    2
#define SYSCON_BODCTR_BODINTVAL_MASK                   (3 << 2)
#define SYSCON_BODCTR_BODINTVAL_LEVEL2                 (2 << 2)
#define SYSCON_BODCTR_BODINTVAL_LEVEL3                 (3 << 2)

#define SYSCON_BODCTR_BODRSTENA                        (1 << 4)

/**********  Bit definition for NSISRC register  *****************************/
#define SYSCON_NMISRC_IRQN_POS                         0
#define SYSCON_NMISRC_IRQN_MASK                        (0xf << 0)

#define SYSCON_NMISRC_NMIEN                            (1 << 31)

/**********  Bit definition for USBCLKCTRL register  *************************/
#define SYSCON_USBCLKCTRL_APCLK                        (1 << 0)
#define SYSCON_USBCLKCTRL_POL_CLK                      (1 << 1)

/**********  Bit definition for USBCLKST register  ***************************/
#define SYSCON_USBCLKST_NEED_CLKST                     (1 << 0)

/**********  Bit definition for STARTERP1 register  **************************/
#define SYSCON_STARTERP1_RTCINT                        (1 << 12)
#define SYSCON_STARTERP1_WWDT_BODINT                   (1 << 13)
#define SYSCON_STARTERP1_USB_WAKEUP                    (1 << 19)
#define SYSCON_STARTERP1_GROUP0INT                     (1 << 20)
#define SYSCON_STARTERP1_GROUP1INT                     (1 << 21)
#define SYSCON_STARTERP1_USART1_4                      (1 << 23)
#define SYSCON_STARTERP1_USART2_3                      (1 << 24)

/**********  Bit definition for PDSLEEPCFG register  **************************/
#define SYSCON_PDSLEEPCFG_BODPD                        (1 << 3)
#define SYSCON_PDSLEEPCFG_WDTOSC_PD                    (1 << 6)

/**********  Bit definition for PDAWAKECFG register  **************************/
#define SYSCON_PDAWAKECFG_IRCOUT_PD                    (1 << 0)
#define SYSCON_PDAWAKECFG_IRC_PD                       (1 << 1)
#define SYSCON_PDAWAKECFG_FLASH_PD                     (1 << 2)
#define SYSCON_PDAWAKECFG_BOD_PD                       (1 << 3)
#define SYSCON_PDAWAKECFG_ADC_PD                       (1 << 4)
#define SYSCON_PDAWAKECFG_SYSOSC_PD                    (1 << 5)
#define SYSCON_PDAWAKECFG_WDTOSC_PD                    (1 << 6)
#define SYSCON_PDAWAKECFG_SYSPLL_PD                    (1 << 7)
#define SYSCON_PDAWAKECFG_USBPLL_PD                    (1 << 8)
#define SYSCON_PDAWAKECFG_USBPAD_PD                    (1 << 10)
#define SYSCON_PDAWAKECFG_TEMPSENSE_PD                 (1 << 13)

/**********  Bit definition for PDRUNCFG register  ***************************/
#define SYSCON_PDRUNCFG_IRCOUT_PD                      (1 << 0)
#define SYSCON_PDRUNCFG_IRC_PD                         (1 << 1)
#define SYSCON_PDRUNCFG_FLASH_PD                       (1 << 2)
#define SYSCON_PDRUNCFG_BOD_PD                         (1 << 3)
#define SYSCON_PDRUNCFG_ADC_PD                         (1 << 4)
#define SYSCON_PDRUNCFG_SYSOSC_PD                      (1 << 5)
#define SYSCON_PDRUNCFG_WDTOSC_PD                      (1 << 6)
#define SYSCON_PDRUNCFG_SYSPLL_PD                      (1 << 7)
#define SYSCON_PDRUNCFG_USBPLL_PD                      (1 << 8)
#define SYSCON_PDRUNCFG_USBPAD_PD                      (1 << 10)
#define SYSCON_PDRUNCFG_TEMPSENSE_PD                   (1 << 13)

/******************************************************************************/
/*                                                                            */
/*                            GPIO                                            */
/*                                                                            */
/******************************************************************************/
//common for most IO
#define GPIO_MODE_NOPULL                        (0 << 3)
#define GPIO_MODE_PULLDOWN                      (1 << 3)
#define GPIO_MODE_PULLUP                        (2 << 3)
#define GPIO_MODE_REPEATER                      (3 << 3)

#define GPIO_HYS                                (1 << 5)
#define GPIO_INV                                (1 << 6)
#define GPIO_ANALOG                             (1 << 7)
#define GPIO_ANALOG_FILTER                      (1 << 8)
#define GPIO_OPEN_DRAIN                         (1 << 10)

//I2C specific mode
#define GPIO_I2C_MODE_STANDART                  (0 << 8)
#define GPIO_I2C_MODE_STANDART_IO               (1 << 8)
#define GPIO_I2C_MODE_FAST                      (2 << 8)

//PIO0_0
#define PIN_MODE_RESET                          0
#define PIN_MODE_PIO0_0                         1

//PIO0_1
#define PIN_MODE_PIO0_1                         0
#define PIN_MODE_CLKOUT                         1
#define PIN_MODE_CT32B0_MAT2                    2
#define PIN_MODE_USB_FTOGGLE                    3

//PIO0_2
#define PIN_MODE_PIO0_2                         0
#define PIN_MODE_SSEL0                          1
#define PIN_MODE_CT16B0_CAP0                    2
#define PIN_MODE_IOH_0                          3

//PIO0_3
#define PIN_MODE_PIO0_3                         0
#define PIN_MODE_VBUS                           1
#define PIN_MODE_IOH_1                          2

//PIO0_4
#define PIN_MODE_PIO0_4                         0
#define PIN_MODE_I2C_SCL                        1
#define PIN_MODE_IOH_2                          2

//PIO0_5
#define PIN_MODE_PIO0_5                         0
#define PIN_MODE_I2C_SDA                        1
#define PIN_MODE_IOH_3                          2

//PIO0_6
#define PIN_MODE_PIO0_6                         0
#define PIN_MODE_USB_CONNECT                    1
#define PIN_MODE_SCK0                           2
#define PIN_MODE_SCK0_IOH_4                     3

//PIO0_7
#define PIN_MODE_PIO0_7                         0
#define PIN_MODE_CTS                            1
#define PIN_MODE_IOH_5                          2

//PIO0_8
#define PIN_MODE_PIO0_8                         0
#define PIN_MODE_MIS00                          1
#define PIN_MODE_CT16B0_MAT0                    2
#define PIN_MODE_IOH_6                          4

//PIO0_9
#define PIN_MODE_PIO0_9                         0
#define PIN_MODE_MOSI0                          1
#define PIN_MODE_CT16B0_MAT1                    2
#define PIN_MODE_IOH_7                          4

//PIO0_10
#define PIN_MODE_SWCLK                          0
#define PIN_MODE_PIO0_10                        1
#define PIN_MODE_SCK0                           2
#define PIN_MODE_CT16B0_MAT2                    3

//PIO0_11
#define PIN_MODE_TDI                            0
#define PIN_MODE_PIO0_11                        1
#define PIN_MODE_AD0                            2
#define PIN_MODE_CT32B0_MAT3                    3

//PIO0_12
#define PIN_MODE_TMS                            0
#define PIN_MODE_PIO0_12                        1
#define PIN_MODE_AD1                            2
#define PIN_MODE_CT32B1_CAP0                    3

//PIO0_13
#define PIN_MODE_TDO                            0
#define PIN_MODE_PIO0_13                        1
#define PIN_MODE_AD2                            2
#define PIN_MODE_CT32B1_MAT0                    3

//PIO0_14
#define PIN_MODE_TRST                           0
#define PIN_MODE_PIO0_14                        1
#define PIN_MODE_AD3                            2
#define PIN_MODE_CT32B1_MAT1                    3

//PIO0_15
#define PIN_MODE_SDIO                           0
#define PIN_MODE_PIO0_15                        1
#define PIN_MODE_AD4                            2
#define PIN_MODE_CT32B1_MAT2                    3

//PIO0_16
#define PIN_MODE_PIO0_16                        0
#define PIN_MODE_AD5                            1
#define PIN_MODE_CT32B1_MAT3                    2
#define PIN_MODE_IOH_8                          3

//PIO0_17
#define PIN_MODE_PIO0_17                        0
#define PIN_MODE_RTS                            1
#define PIN_MODE_CT32B0_CAP0                    2
#define PIN_MODE_SCLK                           3

//PIO0_18
#define PIN_MODE_PIO0_18                        0
#define PIN_MODE_RXD                            1
#define PIN_MODE_CT32B0_MAT0                    2

//PIO0_19
#define PIN_MODE_PIO0_19                        0
#define PIN_MODE_TXD                            1
#define PIN_MODE_CT32B0_MAT1                    2

//PIO0_20
#define PIN_MODE_PIO0_20                        0
#define PIN_MODE_CT16B1_CAP0                    1

//PIO0_21
#define PIN_MODE_PIO0_21                        0
#define PIN_MODE_CT16B1_MAT0                    1
#define PIN_MODE_MOSI1                          2

//PIO0_22
#define PIN_MODE_PIO0_22                        0
#define PIN_MODE_AD6                            1
#define PIN_MODE_CT16B1_MAT1                    2
#define PIN_MODE_MISO1                          3

//PIO0_23
#define PIN_MODE_PIO0_23                        0
#define PIN_MODE_AD7                            1
#define PIN_MODE_IOH_9                          2

//PIO1_0
#define PIN_MODE_PIO1_0                         0
#define PIN_MODE_CT32B1_MAT1_0                  1
#define PIN_MODE_IOH_10                         2

//PIO1_1
#define PIN_MODE_PIO1_1                         0
#define PIN_MODE_CT32B1_MAT1_1                  1
#define PIN_MODE_IOH_10                         2

//PIO1_2
#define PIN_MODE_PIO1_2                         0
#define PIN_MODE_CT32B1_MAT2_1                  1
#define PIN_MODE_IOH_12                         2

//PIO1_3
#define PIN_MODE_PIO1_3                         0
#define PIN_MODE_CT32B1_MAT3_1                  1
#define PIN_MODE_IOH_13                         2

//PIO1_4
#define PIN_MODE_PIO1_4                         0
#define PIN_MODE_CT32B1_CAP0_1_4                1
#define PIN_MODE_IOH_14                         2

//PIO1_5
#define PIN_MODE_PIO1_5                         0
#define PIN_MODE_CT32B1_CAP1_1_5                1
#define PIN_MODE_IOH_15                         2

//PIO1_6
#define PIN_MODE_PIO1_6                         0
#define PIN_MODE_IOH_16                         1

//PIO1_7
#define PIN_MODE_PIO1_7                         0
#define PIN_MODE_IOH_17                         1

//PIO1_8
#define PIN_MODE_PIO1_8                         0
#define PIN_MODE_IOH_18                         1

//PIO1_9
#define PIN_MODE_PIO1_9                         0

//PIO1_10
#define PIN_MODE_PIO1_10                        0

//PIO1_11
#define PIN_MODE_PIO1_11                        0

//PIO1_12
#define PIN_MODE_PIO1_12                        0

//PIO1_13
#define PIN_MODE_PIO1_13                        0
#define PIN_MODE_DTR                            1
#define PIN_MODE_CT16B0_MAT0_1                  2
#define PIN_MODE_TXD_1                          3

//PIO1_14
#define PIN_MODE_PIO1_14                        0
#define PIN_MODE_DSR                            1
#define PIN_MODE_CT16B0_MAT1_1                  2
#define PIN_MODE_RXD_1                          3

//PIO1_15
#define PIN_MODE_PIO1_15                        0
#define PIN_MODE_DCD                            1
#define PIN_MODE_CT16B0_MAT2_1                  2
#define PIN_MODE_SCK1                           3

//PIO1_16
#define PIN_MODE_PIO1_16                        0
#define PIN_MODE_RI                             1
#define PIN_MODE_CT16B0_CAP0_1                  2

//PIO1_17
#define PIN_MODE_PIO1_17                        0
#define PIN_MODE_CT16B0_CAP1_1                  1
#define PIN_MODE_RXD_2                          2

//PIO1_18
#define PIN_MODE_PIO1_18                        0
#define PIN_MODE_CT16B1_CAP1_1                  1
#define PIN_MODE_TXD_2                          2

//PIO1_19
#define PIN_MODE_PIO1_19                        0
#define PIN_MODE_DTR_1                          1
#define PIN_MODE_SSEL1                          2

//PIO1_20
#define PIN_MODE_PIO1_20                        0
#define PIN_MODE_DSR_1                          1
#define PIN_MODE_SCK1_1                         2

//PIO1_21
#define PIN_MODE_PIO1_21                        0
#define PIN_MODE_DCD_1                          1
#define PIN_MODE_MISO1_1                        2

//PIO1_22
#define PIN_MODE_PIO1_22                        0
#define PIN_MODE_RI_1                           1
#define PIN_MODE_MOSI1_1                        2

//PIO1_23
#define PIN_MODE_PIO1_23                        0
#define PIN_MODE_CT16B1_MAT1_1                  1
#define PIN_MODE_SSEL1_1                        2

//PIO1_24
#define PIN_MODE_PIO1_24                        0
#define PIN_MODE_CT32B0_MAT0_1                  1

//PIO1_25
#define PIN_MODE_PIO1_25                        0
#define PIN_MODE_CT32B0_MAT1_1                  1

//PIO1_26
#define PIN_MODE_PIO1_26                        0
#define PIN_MODE_CT32B0_MAT2_1                  1
#define PIN_MODE_RXD_3                          2
#define PIN_MODE_IOH_19                         3

//PIO1_27
#define PIN_MODE_PIO1_27                        0
#define PIN_MODE_CT32B0_MAT3_1                  1
#define PIN_MODE_TXD_3                          2
#define PIN_MODE_IOH_20                         3

//PIO1_28
#define PIN_MODE_PIO1_28                        0
#define PIN_MODE_CT32B0_CAP0_1                  1
#define PIN_MODE_SCLK_1                         2

//PIO1_29
#define PIN_MODE_PIO1_29                        0
#define PIN_MODE_SCK0_1                         1
#define PIN_MODE_CT32B0_CAP1_1                  2

//PIO1_31
#define PIN_MODE_PIO1_31                        0


/******************************************************************************/
/*                                                                            */
/*                            USART                                           */
/*                                                                            */
/******************************************************************************/

/**********  Bit definition for IER register  *********************************/
#define USART_IER_RBRINTEN                      (1 << 0)
#define USART_IER_THRINTEN                      (1 << 1)
#define USART_IER_RLSINTEN                      (1 << 2)
#define USART_IER_MSIINTEN                      (1 << 3)
#define USART_IER_ABEOINTEN                     (1 << 8)
#define USART_IER_ABTOINTEN                     (1 << 9)

/**********  Bit definition for IIR register  *********************************/
#define USART_IIR_INTSTATUS                     (1 << 0)

#define USART_IIR_INTID_MASK                    (7 << 1)
#define USART_IIR_INTID_MODEM_STATUS            (0 << 1)
#define USART_IIR_INTID_THRE                    (1 << 1)
#define USART_IIR_INTID_RDA                     (2 << 1)
#define USART_IIR_INTID_RLS                     (3 << 1)
#define USART_IIR_INTID_CTI                     (6 << 1)

#define USART_IIR_FIFOEN_MASK                   (3 << 6)

#define USART_IIR_ABEOINT                       (1 << 8)
#define USART_IIR_ABTOINT                       (1 << 9)

/**********  Bit definition for FCR register  *********************************/
#define USART_FCR_FIFOEN                        (1 << 0)
#define USART_FCR_RXFIFORES                     (1 << 1)
#define USART_FCR_TXFIFORES                     (1 << 2)

#define USART_FCR_RXTL_1                        (0 << 6)
#define USART_FCR_RXTL_4                        (1 << 6)
#define USART_FCR_RXTL_8                        (2 << 6)
#define USART_FCR_RXTL_14                       (3 << 6)

/**********  Bit definition for LCR register  *********************************/
#define USART_LCR_WLS_POS                       0
#define USART_LCR_WLS_MASK                      (3 << 0)
#define USART_LCR_WLS_5                         (0 << 0)
#define USART_LCR_WLS_6                         (1 << 0)
#define USART_LCR_WLS_7                         (2 << 0)
#define USART_LCR_WLS_8                         (3 << 0)

#define USART_LCR_SBS_POS                       2
#define USART_LCR_SBS_1                         (0 << 2)
#define USART_LCR_SBS_2                         (1 << 2)

#define USART_LCR_PE                            (1 << 3)

#define USART_LCR_PS_MASK                       (3 << 4)
#define USART_LCR_PS_ODD                        (0 << 4)
#define USART_LCR_PS_EVEN                       (1 << 4)
#define USART_LCR_PS_FORCE_1                    (2 << 4)
#define USART_LCR_PS_FORCE_0                    (3 << 4)

#define USART_LCR_BC                            (1 << 6)
#define USART_LCR_DLAB                          (1 << 7)

/**********  Bit definition for MCR register  *********************************/
#define USART_MCR_DTRCTRL                       (1 << 0)
#define USART_MCR_RTSCTRL                       (1 << 1)
#define USART_MCR_LMS                           (1 << 4)
#define USART_MCR_RTSEN                         (1 << 6)
#define USART_MCR_CTSEN                         (1 << 7)

/**********  Bit definition for LSR register  *********************************/
#define USART_LSR_RDR                           (1 << 0)
#define USART_LSR_OE                            (1 << 1)
#define USART_LSR_PE                            (1 << 2)
#define USART_LSR_FE                            (1 << 3)
#define USART_LSR_BI                            (1 << 4)
#define USART_LSR_THRE                          (1 << 5)
#define USART_LSR_TEMT                          (1 << 6)
#define USART_LSR_RXFE                          (1 << 7)
#define USART_LSR_TXERR                         (1 << 8)

/**********  Bit definition for MSR register  *********************************/
#define USART_MSR_DCTS                          (1 << 0)
#define USART_MSR_DDSR                          (1 << 1)
#define USART_MSR_TERI                          (1 << 2)
#define USART_MSR_DDCR                          (1 << 3)
#define USART_MSR_CTS                           (1 << 4)
#define USART_MSR_DSR                           (1 << 5)
#define USART_MSR_RI                            (1 << 6)
#define USART_MSR_DCD                           (1 << 7)

/**********  Bit definition for ACR register  *********************************/
#define USART_ACR_START                         (1 << 0)
#define USART_ACR_MODE                          (1 << 1)
#define USART_ACR_AUTORESTART                   (1 << 2)
#define USART_ACR_ABEOINTCLR                    (1 << 8)
#define USART_ACR_ABTOINTCLR                    (1 << 9)

/**********  Bit definition for ICR register  *********************************/
#define USART_ICR_IRDAEN                        (1 << 0)
#define USART_ICR_IRDAINV                       (1 << 1)
#define USART_ICR_FIXPULSEEN                    (1 << 2)

#define USART_ICR_PULSEDIV_MASK                 (7 << 3)
#define USART_ICR_PULSEDIV_3_16                 (0 << 3)
#define USART_ICR_PULSEDIV_2TPCLK               (1 << 3)
#define USART_ICR_PULSEDIV_4TPCLK               (2 << 3)
#define USART_ICR_PULSEDIV_8TPCLK               (3 << 3)
#define USART_ICR_PULSEDIV_16TPCL               (4 << 3)
#define USART_ICR_PULSEDIV_32TPCL               (5 << 3)
#define USART_ICR_PULSEDIV_64TPCL               (6 << 3)
#define USART_ICR_PULSEDIV_128TPC               (7 << 3)

/**********  Bit definition for FDR register  *********************************/
#define USART_FDR_DIVADDVAL_MASK                (0xf << 0)
#define USART_FDR_DIVADDVAL_POS                 0
#define USART_FDR_MULVAL_MASK                   (0xf << 4)
#define USART_FDR_MULVAL_POS                    4

/**********  Bit definition for OSR register  *********************************/
#define USART_OSR_OSFRAC_MASK                   (7 << 1)
#define USART_OSR_OSFRAC_POS                    1
#define USART_OSR_OSINT_MASK                    (0xf << 4)
#define USART_OSR_OSINT_POS                     4

#define USART_OSR_FDINT_MASK                    (0x7f << 8)
#define USART_OSR_FDINT_POS                     8

/**********  Bit definition for TER register  *********************************/
#define USART_TER_TXEN                          (1 << 0)

/**********  Bit definition for HDEN register  ********************************/
#define USART_HDEN_HDEN                         (1 << 0)

/**********  Bit definition for SCICTRL register  *****************************/
#define USART_SCICTRL_SCIEN                     (1 << 0)
#define USART_SCICTRL_NACKDIS                   (1 << 1)
#define USART_SCICTRL_PROTSEL                   (1 << 2)

#define USART_SCICTRL_TXRETRY_MASK              (7 << 5)
#define USART_SCICTRL_TXRETRY_POS               5

#define USART_SCICTRL_XTRAGUARD_MASK            (0xff << 8)
#define USART_SCICTRL_XTRAGUARD_POS             8

/**********  Bit definition for RS485CTRL register  ***************************/
#define USART_RS485CTRL_NMMEN                   (1 << 0)
#define USART_RS485CTRL_RXDIS                   (1 << 1)
#define USART_RS485CTRL_AADEN                   (1 << 2)
#define USART_RS485CTRL_SEL                     (1 << 3)
#define USART_RS485CTRL_DCTRL                   (1 << 4)
#define USART_RS485CTRL_OINV                    (1 << 5)

/**********  Bit definition for SYNCCTRL register  ****************************/
#define USART_SYNCCTRL_SYNC                     (1 << 0)
#define USART_SYNCCTRL_CSRC                     (1 << 1)
#define USART_SYNCCTRL_FES                      (1 << 2)
#define USART_SYNCCTRL_TSBYPASS                 (1 << 3)
#define USART_SYNCCTRL_CSCEN                    (1 << 4)
#define USART_SYNCCTRL_SSDIS                    (1 << 5)
#define USART_SYNCCTRL_CCCLR                    (1 << 6)

/******************************************************************************/
/*                                                                            */
/*                            USART(1-4) LPC11U6xx                            */
/*                                                                            */
/******************************************************************************/

//USART1-4 (LPC11U6xx)

/**********  Bit definition for CFG register  *********************************/
#define USART4_CFG_ENABLE                       (1 << 0)

#define USART4_CFG_DATALEN_POS                  2
#define USART4_CFG_DATALEN_MASK                 (3 << 2)
#define USART4_CFG_DATALEN_7                    (0 << 2)
#define USART4_CFG_DATALEN_8                    (1 << 2)
#define USART4_CFG_DATALEN_9                    (2 << 2)

#define USART4_CFG_PARITYSEL_POS                4
#define USART4_CFG_PARITYSEL_MASK               (3 << 4)
#define USART4_CFG_PARITYSEL_NO_PARITY          (0 << 4)
#define USART4_CFG_PARITYSEL_EVEN               (2 << 4)
#define USART4_CFG_PARITYSEL_ODD                (3 << 4)

#define USART4_CFG_STOPLEN_POS                  6
#define USART4_CFG_STOPLEN_1                    (0 << 6)
#define USART4_CFG_STOPLEN_2                    (1 << 6)

#define USART4_CFG_MODE32K                      (1 << 7)
#define USART4_CFG_CTSEN                        (1 << 9)
#define USART4_CFG_SYNCEN                       (1 << 11)
#define USART4_CFG_CLKPOL                       (1 << 12)
#define USART4_CFG_SYNMST                       (1 << 14)
#define USART4_CFG_LOOP                         (1 << 15)
#define USART4_CFG_OETA                         (1 << 18)
#define USART4_CFG_AUTOADDR                     (1 << 19)
#define USART4_CFG_OESEL                        (1 << 20)
#define USART4_CFG_OEPOL                        (1 << 21)
#define USART4_CFG_RXPOL                        (1 << 22)
#define USART4_CFG_TXPOL                        (1 << 23)

/**********  Bit definition for CTL register  ********************************/
#define USART4_CTL_TXBRKEN                      (1 << 1)
#define USART4_CTL_ADDRDET                      (1 << 2)
#define USART4_CTL_TXDIS                        (1 << 6)
#define USART4_CTL_CC                           (1 << 8)
#define USART4_CTL_CLRCCONRX                    (1 << 9)
#define USART4_CTL_AUTOBAUD                     (1 << 16)

/**********  Bit definition for STAT register  *******************************/
#define USART4_STAT_RXRDY                       (1 << 0)
#define USART4_STAT_RXIDLE                      (1 << 1)
#define USART4_STAT_TXRDY                       (1 << 2)
#define USART4_STAT_TXIDLE                      (1 << 3)
#define USART4_STAT_CTS                         (1 << 4)
#define USART4_STAT_DELTACTS                    (1 << 5)
#define USART4_STAT_TXDISTAT                    (1 << 6)
#define USART4_STAT_OVERRUNINT                  (1 << 8)
#define USART4_STAT_RXBRK                       (1 << 10)
#define USART4_STAT_DELTARXBRK                  (1 << 11)
#define USART4_STAT_START                       (1 << 12)
#define USART4_STAT_FRAMERRINT                  (1 << 13)
#define USART4_STAT_PERITYERRINT                (1 << 14)
#define USART4_STAT_RXNOISEINT                  (1 << 15)
#define USART4_STAT_ABERR                       (1 << 16)

/**********  Bit definition for INTENSET register  ****************************/
#define USART4_INTENSET_RXRDYEN                 (1 << 0)
#define USART4_INTENSET_TXRDYEN                 (1 << 2)
#define USART4_INTENSET_TXIDLEEN                (1 << 3)
#define USART4_INTENSET_DELTACTSEN              (1 << 5)
#define USART4_INTENSET_TXDISEN                 (1 << 6)
#define USART4_INTENSET_OVERRUNEN               (1 << 8)
#define USART4_INTENSET_DELTARXBRKEN            (1 << 11)
#define USART4_INTENSET_STARTEN                 (1 << 12)
#define USART4_INTENSET_FRAMERREN               (1 << 13)
#define USART4_INTENSET_PERITYERREN             (1 << 14)
#define USART4_INTENSET_RXNOISEEN               (1 << 15)
#define USART4_INTENSET_ABERREN                 (1 << 16)

/**********  Bit definition for INTENCLR register  ****************************/
#define USART4_INTENCLR_RXRDYCLR                (1 << 0)
#define USART4_INTENCLR_TXRDYCLR                (1 << 2)
#define USART4_INTENCLR_TXIDLECLR               (1 << 3)
#define USART4_INTENCLR_DELTACTSCLR             (1 << 5)
#define USART4_INTENCLR_TXDISCLR                (1 << 6)
#define USART4_INTENCLR_OVERRUNCLR              (1 << 8)
#define USART4_INTENCLR_DELTARXBRKCLR           (1 << 11)
#define USART4_INTENCLR_STARTCLR                (1 << 12)
#define USART4_INTENCLR_FRAMERRCLR              (1 << 13)
#define USART4_INTENCLR_PERITYERRCLR            (1 << 14)
#define USART4_INTENCLR_RXNOISECLR              (1 << 15)
#define USART4_INTENCLR_ABERRCLR                (1 << 16)

/**********  Bit definition for RXDATSTAT register  ***************************/
#define USART4_RXDATSTAT_RXDAT_POS              0
#define USART4_RXDATSTAT_RXDAT_MASK             (0xff << 0)

#define USART4_RXDATSTAT_FRAMERR                (1 << 13)
#define USART4_RXDATSTAT_PERITYERR              (1 << 14)
#define USART4_RXDATSTAT_RXNOISE                (1 << 15)

/**********  Bit definition for INTSTAT register  *****************************/
#define USART4_INTSTAT_RXRDY                    (1 << 0)
#define USART4_INTSTAT_TXRDY                    (1 << 2)
#define USART4_INTSTAT_TXIDLE                   (1 << 3)
#define USART4_INTSTAT_DELTACTS                 (1 << 5)
#define USART4_INTSTAT_TXDISTAT                 (1 << 6)
#define USART4_INTSTAT_OVERRUNINT               (1 << 8)
#define USART4_INTSTAT_DELTARXBRK               (1 << 11)
#define USART4_INTSTAT_START                    (1 << 12)
#define USART4_INTSTAT_FRAMERRINT               (1 << 13)
#define USART4_INTSTAT_PERITYERRINT             (1 << 14)
#define USART4_INTSTAT_RXNOISEINT               (1 << 15)
#define USART4_INTSTAT_ABERR                    (1 << 16)

/******************************************************************************/
/*                                                                            */
/*                                 I2C                                        */
/*                                                                            */
/******************************************************************************/

/**********  Bit definition for CONSET register  ******************************/
#define I2C_CONSET_AA                           (1 << 2)                /* Assert acknowledge flag */
#define I2C_CONSET_SI                           (1 << 3)                /* I2C interrupt flag */
#define I2C_CONSET_STO                          (1 << 4)                /* STOP flag */
#define I2C_CONSET_STA                          (1 << 5)                /* START flag */
#define I2C_CONSET_I2EN                         (1 << 6)                /* I2C interface enable */

/***********  Bit definition for STAT register  *******************************/
#define I2C_STAT_STATUS_MASK                    (0x1f << 3)

#define I2C_STAT_START                          0x08                    /* A START condition has been transmitted */
#define I2C_STAT_REPEATED_START                 0x10                    /* A Repeated START condition has been transmitted */
#define I2C_STAT_SLAW_ACK                       0x18                    /* SLA+W has been transmitted; ACK has been received */
#define I2C_STAT_SLAW_NACK                      0x20                    /* SLA+W has been transmitted; NOT ACK has been received */
#define I2C_STAT_DATW_ACK                       0x28                    /* Data byte in DAT has been transmitted; ACK has been received */
#define I2C_STAT_DATW_NACK                      0x30                    /* Data byte in DAT has been transmitted; NOT ACK has been received */
#define I2C_STAT_ARBITRATION_LOST               0x38                    /* Arbitration lost in SLA+R/W or Data bytes */
#define I2C_STAT_SLAR_ACK                       0x40                    /* SLA+R has been transmitted; ACK has been received */
#define I2C_STAT_SLAR_NACK                      0x48                    /* SLA+R has been transmitted; NOT ACK has been received */
#define I2C_STAT_DATR_ACK                       0x50                    /* Data byte has been received; ACK has been returned */
#define I2C_STAT_DATR_NACK                      0x58                    /* Data byte has been received; NOT ACK has been returned */
#define I2C_STAT_SLAVE_SLAW_ACK                 0x60                    /* Own SLA+W has been received; ACK has been returned */
#define I2C_STAT_SLAVE_SLAW_NACK                0x68                    /* Arbitration lost in SLA+R/W as master;
                                                                           Own SLA+W has been received, ACK returned */
#define I2C_STAT_SLAVE_GCA_ACK                  0x70                    /* General call address (0x00) has been received; ACK has been returned*/
#define I2C_STAT_SLAVE_GCA_NACK                 0x78                    /* Arbitration lost in SLA+R/W as master;
                                                                           General call address has been received, ACK has been returned */
#define I2C_STAT_SLAVE_DATW_ACK                 0x80                    /* Previously addressed with own SLV address;
                                                                           DATA has been received; ACK has been returned */
#define I2C_STAT_SLAVE_DATW_NACK                0x88                    /* Previously addressed with own SLV address;
                                                                           DATA has been received; NOT ACK has been returned */
#define I2C_STAT_SLAVE_GCA_DATW_ACK             0x90                    /* Previously addressed with General Call;
                                                                           DATA byte has been received; ACK has been returned */
#define I2C_STAT_SLAVE_GCA_DATW_NACK            0x98                    /* Previously addressed with General Call;
                                                                           DATA byte has been received; NOT ACK has been returned */
#define I2C_STAT_SLAVE_STOP                     0xA0                    /* A STOP condition or Repeated START condition has been
                                                                           received while still addressed as SLV/REC or SLV/TRX */
#define I2C_STAT_SLAVE_SLAR_ACK                 0xA8                    /* Own SLA+R has been received; ACK has been returned */
#define I2C_STAT_SLAVE_SLAR_NACK                0xB0                    /* Arbitration lost in SLA+R/W as master;
                                                                           Own SLA+R has beenreceived, ACK has been returned */
#define I2C_STAT_SLAVE_DATR_ACK                 0xB8                    /* Data byte in DAT has been transmitted; ACK has been received */
#define I2C_STAT_SLAVE_DATR_NACK                0xC0                    /* Data byte in DAT has been transmitted; NOT ACK has been received */
#define I2C_STAT_SLAVE_DATR_LAST                0xC8                    /* Last data byte in DAT has been transmitted (AA = 0); ACK has been received */

#define I2C_STAT_NO_INFO                        0xF8                    /* No relevant state information available; SI = 0 */
#define I2C_STAT_ERROR                          0x00                    /* Bus error during MSTor selected slave modes, due to an illegal START or
                                                                           STOP condition. State 0x00 can also occur when interference causes the I2C block
                                                                           to enter an undefined state */


/***********  Bit definition for ADR0 register  *******************************/
#define I2C_ADR0_GC                             (1 << 0)                /* General Call enable bit */


/**********  Bit definition for CONCLR register  ******************************/
#define I2C_CONCLR_AAC                          (1 << 2)                /* Assert acknowledge flag */
#define I2C_CONCLR_SIC                          (1 << 3)                /* I2C interrupt flag */
#define I2C_CONCLR_STOC                         (1 << 4)                /* STOP flag */
#define I2C_CONCLR_STAC                         (1 << 5)                /* START flag */
#define I2C_CONCLR_I2ENC                        (1 << 6)                /* I2C interface enable */

/**********  Bit definition for MMCTRL register  ******************************/
#define I2C_MMCTRL_MMENA                        (1 << 0)                /* Monitor mode enable */
#define I2C_MMCTRL_ENASCL                       (1 << 1)                /* SCL output enable */
#define I2C_MMCTRL_MATCH_ALL                    (1 << 2)                /* Select interrupt register match */


/******************************************************************************/
/*                                                                            */
/*                            Timer16/32                                      */
/*                                                                            */
/******************************************************************************/

/**********  Bit definition for IR register  **********************************/
#define CT_IR_MR0INT                            (1 << 0)                /* Interrupt flag for match channel 0 */
#define CT_IR_MR1INT                            (1 << 1)                /* Interrupt flag for match channel 0 */
#define CT_IR_MR2INT                            (1 << 2)                /* Interrupt flag for match channel 0 */
#define CT_IR_MR3INT                            (1 << 3)                /* Interrupt flag for match channel 0 */
#define CT_IR_CR0INT                            (1 << 4)                /* Interrupt flag for capture channel 0 event */

#define CT1_IR_CR1INT                           (1 << 5)                /* Interrupt flag for capture channel 1 event CTxxB1 */
#define CT0_IR_CR1INT                           (1 << 6)                /* Interrupt flag for capture channel 1 event CTxxB0 */

/**********  Bit definition for TCR register  *********************************/
#define CT_TCR_CEN                              (1 << 0)                /* Counter enable */
#define CT_TCR_CRST                             (1 << 1)                /* Counter reset */

/**********  Bit definition for MCR register  *********************************/
#define CT_MCR_MR0I                             (1 << 0)                /* Interrupt on MR0: an interrupt is generated when MR0 matches the value in the TC */
#define CT_MCR_MR0R                             (1 << 1)                /* Reset on MR0: the TC will be reset if MR0 matches it */
#define CT_MCR_MR0S                             (1 << 2)                /* Stop on MR0: the TC and PC will be stopped and TCR[0] will be set to 0 if MR0 matches the TC */
#define CT_MCR_MR1I                             (1 << 3)                /* Interrupt on MR1: an interrupt is generated when MR1 matches the value in the TC */
#define CT_MCR_MR1R                             (1 << 4)                /* Reset on MR1: the TC will be reset if MR1 matches it */
#define CT_MCR_MR1S                             (1 << 5)                /* Stop on MR1: the TC and PC will be stopped and TCR[1] will be set to 0 if MR1 matches the TC */
#define CT_MCR_MR2I                             (1 << 6)                /* Interrupt on MR2: an interrupt is generated when MR2 matches the value in the TC */
#define CT_MCR_MR2R                             (1 << 7)                /* Reset on MR2: the TC will be reset if MR2 matches it */
#define CT_MCR_MR2S                             (1 << 8)                /* Stop on MR2: the TC and PC will be stopped and TCR[2] will be set to 0 if MR2 matches the TC */
#define CT_MCR_MR3I                             (1 << 9)                /* Interrupt on MR3: an interrupt is generated when MR3 matches the value in the TC */
#define CT_MCR_MR3R                             (1 << 10)               /* Reset on MR3: the TC will be reset if MR3 matches it */
#define CT_MCR_MR3S                             (1 << 11)               /* Stop on MR3: the TC and PC will be stopped and TCR[3] will be set to 0 if MR3 matches the TC */

/**********  Bit definition for CCR register  *********************************/
#define CT_CCR_CAP0RE                           (1 << 0)                /* Capture on CTxxBx_CAP0 rising edge: a sequence of 0 then 1 on CTxxBx_CAP0 will
                                                                           cause CR0 to be loaded with the contents of TC */
#define CT_CCR_CAP0FE                           (1 << 1)                /* Capture on CTxxBx_CAP0 falling edge: a sequence of 1 then 0 on CTxxBx_CAP0 will
                                                                           cause CR0 to be loaded with the contents of TC */
#define CT_CCR_CAP0I                            (1 << 2)                /* Interrupt on CTxxBx_CAP0 event: a CR0 load due to a CTxxBx_CAP0 event will
                                                                           generate an interrupt */

#define CT1_CCR_CAP1RE                          (1 << 3)                /* Capture on CTxxB1_CAP1 rising edge: a sequence of 0 then 1 on CTxxB1_CAP1 will
                                                                           cause CR1 to be loaded with the contents of TC */
#define CT1_CCR_CAP1FE                          (1 << 4)                /* Capture on CTxxB1_CAP0 falling edge: a sequence of 1 then 0 on CTxxB1_CAP1 will
                                                                           cause CR1 to be loaded with the contents of TC */
#define CT1_CCR_CAP1I                           (1 << 5)                /* Interrupt on CTxxB1_CAP1 event: a CR1 load due to a CTxxB1_CAP1 event will
                                                                           generate an interrupt */

#define CT0_CCR_CAP1RE                          (1 << 6)                /* Capture on CTxxB0_CAP1 rising edge: a sequence of 0 then 1 on CTxxB0_CAP1 will
                                                                           cause CR1 to be loaded with the contents of TC */
#define CT0_CCR_CAP1FE                          (1 << 7)                /* Capture on CTxxB0_CAP0 falling edge: a sequence of 1 then 0 on CTxxB0_CAP1 will
                                                                           cause CR1 to be loaded with the contents of TC */
#define CT0_CCR_CAP1I                           (1 << 8)                /* Interrupt on CTxxB0_CAP1 event: a CR1 load due to a CTxxB0_CAP1 event will
                                                                           generate an interrupt */
/**********  Bit definition for EMR register  *********************************/
#define CT_EMR_EM0                              (1 << 0)                /* External Match 0. This bit reflects the state of output CTxxBx_MAT0,
                                                                           whether or not this output is connected to its pin. When a match occurs between the TC
                                                                           and MR0, this bit can either toggle, go LOW, go HIGH, or do nothing. Bits EMR[5:4]
                                                                           control the functionality of this output. This bit is driven to the
                                                                           CTxxBx_MAT0 pins if the match function is selected in the IOCON
                                                                           registers (0 = LOW, 1 = HIGH) */
#define CT_EMR_EM1                              (1 << 1)                /* External Match 1. This bit reflects the state of output CTxxBx_MAT1,
                                                                           whether or not this output is connected to its pin. When a match occurs between the TC
                                                                           and MR1, this bit can either toggle, go LOW, go HIGH, or do nothing. Bits EMR[7:6]
                                                                           control the functionality of this output. This bit is driven to the
                                                                           CTxxBx_MAT1 pins if the match function is selected in the IOCON
                                                                           registers (0 = LOW, 1 = HIGH) */
#define CT_EMR_EM2                              (1 << 2)                /* External Match 2. This bit reflects the state of output CTxxBx_MAT2,
                                                                           whether or not this output is connected to its pin. When a match occurs between the TC
                                                                           and MR2, this bit can either toggle, go LOW, go HIGH, or do nothing. Bits EMR[9:8]
                                                                           control the functionality of this output. This bit is driven to the
                                                                           CTxxBx_MAT2 pins if the match function is selected in the IOCON
                                                                           registers (0 = LOW, 1 = HIGH) */
#define CT_EMR_EM3                              (1 << 3)                /* External Match 3. This bit reflects the state of output CTxxBx_MAT3,
                                                                           whether or not this output is connected to its pin. When a match occurs between the TC
                                                                           and MR3, this bit can either toggle, go LOW, go HIGH, or do nothing. Bits EMR[11:10]
                                                                           control the functionality of this output. This bit is driven to the
                                                                           CTxxBx_MAT3 pins if the match function is selected in the IOCON
                                                                           registers (0 = LOW, 1 = HIGH) */
#define CT_EMR_EMC0_POS                         4                       /* External Match Control 0. Determines the functionality of External Match 0 */
#define CT_EMR_EMC0_MASK                        (3 << 4)

#define CT_EMR_EMC1_POS                         6                       /* External Match Control 1. Determines the functionality of External Match 1 */
#define CT_EMR_EMC1_MASK                        (3 << 6)

#define CT_EMR_EMC2_POS                         8                       /* External Match Control 2. Determines the functionality of External Match 2 */
#define CT_EMR_EMC2_MASK                        (3 << 8)

#define CT_EMR_EMC3_POS                         10                      /* External Match Control 10. Determines the functionality of External Match 3 */
#define CT_EMR_EMC3_MASK                        (3 << 10)

#define CT_EMR_EMC_DO_NOTHING                   0                       /* Do Nothing */
#define CT_EMR_EMC_CLEAR                        1                       /* Clear the corresponding External Match bit/output to 0 (CTxxBx_MATx pin is LOW if
                                                                           pinned out) */
#define CT_EMR_EMC_SET                          2                       /* Set the corresponding External Match bit/output to 1 (CTxxBx_MATx pin is HIGH if
                                                                           pinned out). */
#define CT_EMR_EMC_TOGGLE                       3                       /* Toggle the corresponding External Match bit/output */

/**********  Bit definition for CTCR register  ********************************/
#define CT_CTCR_CTM_POS                         0                       /* Counter/Timer Mode. This field selects which rising PCLK
                                                                           edges can increment Timer’s Prescale Counter (PC), or
                                                                           clear PC and increment Timer Counter (TC) */
#define CT_CTCR_CTM_TIMER                       (0 << 0)                /* Timer Mode: every rising PCLK edge */
#define CT_CTCR_CTM_COUNTER_RE                  (1 << 0)                /* Counter Mode: TC is incremented on rising edges on the CAP input selected by bits 3:2 */
#define CT_CTCR_CTM_COUNTER_FE                  (2 << 0)                /* Counter Mode: TC is incremented on falling edges on the CAP input selected by bits 3:2 */
#define CT_CTCR_CTM_COUNTER_BE                  (3 << 0)                /* Counter Mode: TC is incremented on both edges on the CAP input selected by bits 3:2 */

#define CT_CTCR_CIS_POS                         2                       /* Count Input Select. In counter mode (when bits 1:0 in this
                                                                           register are not 00), these bits select which CAP pin is
                                                                           sampled for clocking. */
#define CT_CTCR_CIS_CAP0                       (0 << 2)                 /* CTxxBx_CAP0 */
#define CT1_CTCR_CIS_CAP1                      (1 << 2)                 /* CTxxB1_CAP1 */
#define CT0_CTCR_CIS_CAP1                      (2 << 2)                 /* CTxxB0_CAP1 */

#define CT_CTCR_ENCC                           (1 << 4)                 /* Setting this bit to 1 enables clearing of the timer and the
                                                                           prescaler when the capture-edge event specified in bits 7:5
                                                                           occurs. */
#define CT_CTCR_SELCC_POS                      5                        /* Edge select. When bit 4 is 1, these bits select which capture input edge will cause the timer
                                                                           and prescaler to be cleared. These bits have no effect when bit 4 is low. */
#define CT_CTCR_SELCC_CAP0_RE                  (0 << 5)                 /* Rising Edge of CTxxBx_CAP0 clears the timer */
#define CT_CTCR_SELCC_CAP0_FE                  (1 << 5)                 /* Falling Edge of CTxxBx_CAP0 clears the timer */
#define CT1_CTCR_SELCC_CAP1_RE                 (2 << 5)                 /* Rising Edge of CTxxB1_CAP1 clears the timer */
#define CT1_CTCR_SELCC_CAP1_FE                 (3 << 5)                 /* Falling Edge of CTxxB1_CAP1 clears the timer */
#define CT0_CTCR_SELCC_CAP1_RE                 (4 << 5)                 /* Rising Edge of CTxxB0_CAP1 clears the timer */
#define CT0_CTCR_SELCC_CAP1_FE                 (5 << 5)                 /* Falling Edge of CTxxB0_CAP1 clears the timer */

/**********  Bit definition for PWMC register  ********************************/
#define CT_PWMC_PWMEN0                         (1 << 0)                 /*  PWM mode enable for channel0 */
#define CT_PWMC_PWMEN1                         (1 << 1)                 /*  PWM mode enable for channel1 */
#define CT_PWMC_PWMEN2                         (1 << 2)                 /*  PWM mode enable for channel2 */
#define CT_PWMC_PWMEN3                         (1 << 3)                 /*  PWM mode enable for channel3 */

#endif // LPC11UXX_BITS_H
