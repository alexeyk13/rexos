/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef LPC18UXX_BITS_H
#define LPC18UXX_BITS_H

/*
 *      Only values, not provided by vendor library
 */

/******************************************************************************/
/*                                                                            */
/*                 CREG (Configuration registers)                             */
/*                                                                            */
/******************************************************************************/

/*********  Bit definition for FLASHCFG* registers  ***************************/
#define CREG_FLASHCFG_MAGIC_Msk                  (0x3a << 0)
#define CREG_FLASHCFG_FLASHTIM_Pos               12
#define CREG_FLASHCFG_FLASHTIM_Msk               (0xf << 12)
#define CREG_FLASHCFG_POW_Msk                    (1 << 31)

/******************************************************************************/
/*                                                                            */
/*                  CGU (Clock Generation Unit)                               */
/*                                                                            */
/******************************************************************************/

#define CGU_CLK_CLK_SEL_POS                     24

#define CGU_CLK_LSE                             (0 << 24)
#define CGU_CLK_IRC                             (1 << 24)
#define CGU_CLK_ENET_RX                         (2 << 24)
#define CGU_CLK_ENET_TX                         (3 << 24)
#define CGU_CLK_GP_CLKIN                        (4 << 24)
#define CGU_CLK_HSE                             (6 << 24)
#define CGU_CLK_PLL0USB                         (7 << 24)
#define CGU_CLK_PLL0AUDIO                       (8 << 24)
#define CGU_CLK_PLL1                            (9 << 24)
#define CGU_CLK_IDIVA                           (0xc << 24)
#define CGU_CLK_IDIVB                           (0xd << 24)
#define CGU_CLK_IDIVC                           (0xe << 24)
#define CGU_CLK_IDIVD                           (0xf << 24)
#define CGU_CLK_IDIVE                           (0x10 << 24)

#define CGU_IDIVx_CTRL_BASE                     0x40050048
#define CGU_IDIVx_CTRL                          ((unsigned int*)(CGU_IDIVx_CTRL_BASE))
#define CGU_IDIVx_CTRL_CLK_SEL_Msk              (0x1f << 24)
#define CGU_IDIVx_CTRL_IDIV_Msk                 (0xff << 2)
#define CGU_IDIVx_CTRL_IDIV_Pos                 2


/******************************************************************************/
/*                                                                            */
/*                   SCU (System Control Unit)                                */
/*                                                                            */
/******************************************************************************/

/********  Bit definition for SCU_SFS* registers  ***************************/
#define SCU_SFS_EPD                             (1 << 3)                /* Enable pull-down resistor at pad.
                                                                           0 Disable pull-down.
                                                                           1 Enable pull-down. Enable both pull-down resistor and pull-up resistor
                                                                           for repeater mode */
#define SCU_SFS_EPUN                            (1 << 4)                /* Disable pull-up resistor at pad. By default, the pull-up resistor is enabled at reset.
                                                                           0 Enable pull-up. Enable both pull-down resistor and pull-up resistor for repeater mode.
                                                                           1 Disable pull-up */
#define SCU_SFS_EHS                             (1 << 5)                /* Select Slew rate.
                                                                           0 Slow (low noise with medium speed)
                                                                           1 Fast (medium noise with fast speed)*/
#define SCU_SFS_EZI                             (1 << 6)                /* Input buffer enable. The input buffer is disabled by default at reset and must be
                                                                           enabled for receiving.
                                                                           0 Disable input buffer
                                                                           1 Enable input buffer */
#define SCU_SFS_ZIF                             (1 << 7)                /* Input glitch filter. Disable the input glitch filter for clocking signals higher than 30 MHz.
                                                                           0 Enable input glitch filter
                                                                           1 Disable input glitch filter */
                                                                        /* Select drive strength. */
#define SCU_SFS_EHD_NORMAL                      (0 << 8)                /* 0x0 Normal-drive: 4 mA drive strength */
#define SCU_SFS_EHD_MEDIUM                      (1 << 8)                /* 0x1 Medium-drive: 8 mA drive strength */
#define SCU_SFS_EHD_HIGH                        (2 << 8)                /* 0x2 High-drive: 14 mA drive strength */
#define SCU_SFS_EHD_ULTRA_HIGH                  (3 << 8)                /* 0x3 Ultra high-drive: 20 mA drive strength */

/******  Bit definition for SCU_SFSUSB registers  ***************************/
#define SCU_SFSUSB_AIM_Msk                      (1 << 0)                /* Differential data input AIP/AIM.
                                                                           0 Going LOW with full speed edge rate
                                                                           1 Going HIGH with full speed edge rate */
#define SCU_SFSUSB_ESEA_Msk                     (1 << 1)                /* Control signal for differential input or single input.
                                                                           0 Reserved. Do not use.
                                                                           1 Single input. Enables USB1. Use with the on-chip full-speed PHY */
#define SCU_SFSUSB_EPD_Msk                      (1 << 2)                /* Enable pull-down connect.
                                                                           0 Pull-down disconnected
                                                                           1 Pull-down connected */
#define SCU_SFSUSB_EPWR_Msk                     (1 << 4)                /* Power mode.
                                                                           0 Power saving mode (Suspend mode)
                                                                           1 Normal mode */
#define SCU_SFSUSB_VBUS_Msk                     (1 << 5)                /* Enable the vbus_valid signal. This signal is monitored by the USB1 block. Use this bit
                                                                           for software de-bouncing of the VBUS sense signal or to indicate the VBUS state to the
                                                                           USB1 controller when the VBUS signal is present but the USB1_VBUS function is not
                                                                           connected in the SFSP2_5 register.
                                                                           Remark: The setting of this bit has no effect if the USB1_VBUS function of pin
                                                                           P2_5 is enabled through the SFSP2_5 register.
                                                                           0 VBUS signal LOW or inactive
                                                                           1 VBUS signal HIGH or active */

/******  Bit definition for SCU_SFSI2C0 registers  **************************/
#define SCU_SFSI2C0_SCL_EFP                     (1 << 0)                /* Select input glitch filter time constant for the SCL pin.
                                                                           0 50 ns glitch filter
                                                                           1 3 ns glitch filter */
#define SCU_SFSI2C0_SCL_EHD                     (1 << 2)                /* Select I2C mode for the SCL pin.
                                                                           0 Standard/Fast mode transmit
                                                                           1 Fast-mode Plus transmit */
#define SCU_SFSI2C0_SCL_EZI                     (1 << 3)                /* Enable the input receiver for the SCL pin. Always write a 1 to this bit when using
                                                                           the I2C0.
                                                                           0 Disabled
                                                                           1 Enabled */
#define SCU_SFSI2C0_SCL_ZIF                     (1 << 7)                /* Enable or disable input glitch filter for the  SCL pin. The filter time constant is
                                                                           determined by bit SCL_EFP.
                                                                           0 Enable input filter
                                                                           1 Disable input filter */
#define SCU_SFSI2C0_SDA_EFP                     (1 << 8)                /* Select input glitch filter time constant for the SDA pin.
                                                                           0 50 ns glitch filter
                                                                           1 3 ns glitch filter */
#define SCU_SFSI2C0_SDA_EHD                     (1 << 10)               /* Select I2C mode for the SDA pin.
                                                                           0 Standard/Fast mode transmit
                                                                           1 Fast-mode Plus transmit */
#define SCU_SFSI2C0_SDA_EZI                     (1 << 11)               /* Enable the input receiver for the SDA pin. Always write a 1 to this bit when using
                                                                           the I2C0.
                                                                           0 Disabled
                                                                           1 Enabled */
#define SCU_SFSI2C0_SDA_ZIF                     (1 << 15)               /* Enable or disable input glitch filter for the SDA pin. The filter time constant is
                                                                           determined by bit SDA_EFP.
                                                                           0 Enable input filter
                                                                           1 Disable input filter */

//P0_0
#define P0_0_GPIO0_0                            (0 << 0)
#define P0_0_SSP1_MISO                          (1 << 0)
#define P0_0_ENET_RXD1                          (2 << 0)
#define P0_0_I2S0_TX_WS                         (6 << 0)
#define P0_0_I2S1_TX_WS                         (7 << 0)

//P0_1
#define P0_1_GPIO0_1                            (0 << 0)
#define P0_1_SSP1_MOSI                          (1 << 0)
#define P0_1_ENET_COL                           (2 << 0)
#define P0_1_ENET_TX_EN                         (6 << 0)
#define P0_1_I2S1_TX_SDA                        (7 << 0)

//P1_0
#define P1_0_GPIO0_4                            (0 << 0)
#define P1_0_CTIN_3                             (1 << 0)
#define P1_0_EMC_A5                             (2 << 0)
#define P1_0_SSP0_SSEL                          (5 << 0)
#define P1_0_EMC_D12                            (7 << 0)

//P1_1
#define P1_1_GPIO0_8                            (0 << 0)
#define P1_1_CTOUT_7                            (1 << 0)
#define P1_1_EMC_A6                             (2 << 0)
#define P1_1_SSP0_MISO                          (5 << 0)
#define P1_1_EMC_D13                            (7 << 0)

//P1_2
#define P1_2_GPIO0_9                            (0 << 0)
#define P1_2_CTOUT_6                            (1 << 0)
#define P1_2_EMC_A7                             (2 << 0)
#define P1_2_SSP0_MOSI                          (5 << 0)
#define P1_2_EMC_D14                            (7 << 0)

//P1_3
#define P1_3_GPIO0_10                           (0 << 0)
#define P1_3_CTOUT_8                            (1 << 0)
#define P1_3_EMC_OE                             (3 << 0)
#define P1_3_USB0_IND1                          (4 << 0)
#define P1_3_SSP1_MISO                          (5 << 0)
#define P1_3_SD_RST                             (7 << 0)

//P1_4
#define P1_4_GPIO0_11                           (0 << 0)
#define P1_4_CTOUT_9                            (1 << 0)
#define P1_4_EMC_BLS0                           (3 << 0)
#define P1_4_USB0_IND0                          (4 << 0)
#define P1_4_SSP1_MOSI                          (5 << 0)
#define P1_4_EMC_D15                            (6 << 0)
#define P1_4_SD_VOLT1                           (7 << 0)

//P1_5
#define P1_5_GPIO1_8                            (0 << 0)
#define P1_5_CTOUT_10                           (1 << 0)
#define P1_5_EMC_CS0                            (3 << 0)
#define P1_5_USB0_PWR_FAULT                     (4 << 0)
#define P1_5_SSP1_SSEL                          (5 << 0)
#define P1_5_SD_POW                             (7 << 0)

//P1_6
#define P1_6_GPIO1_9                            (0 << 0)
#define P1_6_CTIN_5                             (1 << 0)
#define P1_6_EMC_WE                             (3 << 0)
#define P1_6_EMC_BLS0                           (5 << 0)
#define P1_6_SD_CMD                             (7 << 0)

//P1_7
#define P1_7_GPIO1_0                            (0 << 0)
#define P1_7_U1_DSR                             (1 << 0)
#define P1_7_CTOUT_13                           (2 << 0)
#define P1_7_EMC_D0                             (3 << 0)
#define P1_7_USB0_PWR_EN                        (4 << 0)

//P1_8
#define P1_8_GPIO1_1                            (0 << 0)
#define P1_8_U1_DTR                             (1 << 0)
#define P1_8_CTOUT_12                           (2 << 0)
#define P1_8_EMC_D1                             (3 << 0)
#define P1_8_SD_VOLT0                           (7 << 0)

//P1_9
#define P1_9_GPIO1_2                            (0 << 0)
#define P1_9_U1_RTS                             (1 << 0)
#define P1_9_CTOUT_11                           (2 << 0)
#define P1_9_EMC_D2                             (3 << 0)
#define P1_9_SD_DAT0                            (7 << 0)

//P1_10
#define P1_10_GPIO1_3                           (0 << 0)
#define P1_10_U1_RI                             (1 << 0)
#define P1_10_CTOUT_14                          (2 << 0)
#define P1_10_EMC_D3                            (3 << 0)
#define P1_10_SD_DAT1                           (7 << 0)

//P1_11
#define P1_11_GPIO1_4                           (0 << 0)
#define P1_11_U1_CTS                            (1 << 0)
#define P1_11_CTOUT_15                          (2 << 0)
#define P1_11_EMC_D4                            (3 << 0)
#define P1_11_SD_DAT2                           (7 << 0)

//P1_12
#define P1_12_GPIO1_5                           (0 << 0)
#define P1_12_U1_DCD                            (1 << 0)
#define P1_12_EMC_D5                            (3 << 0)
#define P1_12_T0_CAP1                           (4 << 0)
#define P1_12_SD_DAT3                           (7 << 0)

//P1_13
#define P1_13_GPIO1_6                           (0 << 0)
#define P1_13_U1_TXD                            (1 << 0)
#define P1_13_EMC_D6                            (3 << 0)
#define P1_13_T0_CAP0                           (4 << 0)
#define P1_13_SD_CD                             (7 << 0)

//P1_14
#define P1_14_GPIO1_7                           (0 << 0)
#define P1_14_U1_RXD                            (1 << 0)
#define P1_14_EMC_D7                            (3 << 0)
#define P1_14_T0_MAT2                           (4 << 0)

//P1_15
#define P1_15_GPIO0_2                           (0 << 0)
#define P1_15_U2_TXD                            (1 << 0)
#define P1_15_ENET_RXD0                         (3 << 0)
#define P1_15_T0_MAT1                           (4 << 0)
#define P1_15_EMC_D8                            (6 << 0)

//P1_16
#define P1_16_GPIO0_3                           (0 << 0)
#define P1_16_U2_RXD                            (1 << 0)
#define P1_16_ENET_CRS                          (3 << 0)
#define P1_16_T0_MAT0                           (4 << 0)
#define P1_16_EMC_D9                            (6 << 0)
#define P1_16_ENET_RX_DV                        (7 << 0)

//P1_17
#define P1_17_GPIO0_12                          (0 << 0)
#define P1_17_U2_UCLK                           (1 << 0)
#define P1_17_ENET_MDIO                         (3 << 0)
#define P1_17_T0_CAP3                           (4 << 0)
#define P1_17_CAN1_TD                           (5 << 0)

//P1_18
#define P1_18_GPIO0_13                          (0 << 0)
#define P1_18_U2_DIR                            (1 << 0)
#define P1_18_ENET_TXD0                         (3 << 0)
#define P1_18_T0_MAT3                           (4 << 0)
#define P1_18_CAN1_RD                           (5 << 0)
#define P1_18_EMC_D10                           (7 << 0)

//P1_19
#define P1_19_ENET_TX_CLK                       (0 << 0)
#define P1_19_SSP1_SCK                          (1 << 0)
#define P1_19_CLKOUT                            (4 << 0)
#define P1_19_I2S0_RX_MCLK                      (6 << 0)
#define P1_19_I2S1_TX_SCK                       (7 << 0)

//P1_20
#define P1_20_GPIO0_15                          (0 << 0)
#define P1_20_SSP1_SSEL                         (1 << 0)
#define P1_20_ENET_TXD1                         (3 << 0)
#define P1_20_T0_CAP2                           (4 << 0)
#define P1_20_EMC_D11                           (7 << 0)

//P2_0
#define P2_0_U0_TXD                             (1 << 0)
#define P2_0_EMC_A13                            (2 << 0)
#define P2_0_USB0_PWR_EN                        (3 << 0)
#define P2_0_GPIO5_0                            (4 << 0)
#define P2_0_T3_CAP0                            (6 << 0)
#define P2_0_ENET_MDC                           (7 << 0)

//P2_1
#define P2_1_U0_RXD                             (1 << 0)
#define P2_1_EMC_A12                            (2 << 0)
#define P2_1_USB0_PWR_FAULT                     (3 << 0)
#define P2_1_GPIO5_1                            (4 << 0)
#define P2_1_T3_CAP1                            (6 << 0)

//P2_2
#define P2_2_U0_UCLK                            (1 << 0)
#define P2_2_EMC_A11                            (2 << 0)
#define P2_2_USB0_IND1                          (3 << 0)
#define P2_2_GPIO5_2                            (4 << 0)
#define P2_2_CTIN_6                             (5 << 0)
#define P2_2_T3_CAP2                            (6 << 0)
#define P2_2_EMC_CS1                            (7 << 0)

//P2_3
#define P2_3_I2C1_SDA                           (1 << 0)
#define P2_3_U3_TXD                             (2 << 0)
#define P2_3_CTIN_1                             (3 << 0)
#define P2_3_GPIO5_3                            (4 << 0)
#define P2_3_T3_MAT0                            (6 << 0)
#define P2_3_USB0_PWR_EN                        (7 << 0)

//P2_4
#define P2_4_I2C1_SCL                           (1 << 0)
#define P2_4_U3_RXD                             (2 << 0)
#define P2_4_CTIN_0                             (3 << 0)
#define P2_4_GPIO5_4                            (4 << 0)
#define P2_4_T3_MAT1                            (6 << 0)
#define P2_4_USB0_PWR_FAULT                     (7 << 0)

//P2_5
#define P2_5_CTIN_2                             (1 << 0)
#define P2_5_USB1_VBUS                          (2 << 0)
#define P2_5_ADCTRIG1                           (3 << 0)
#define P2_5_GPIO5_5                            (4 << 0)
#define P2_5_T3_MAT2                            (6 << 0)
#define P2_5_USB0_IND0                          (7 << 0)

//P2_6
#define P2_6_U0_DIR                             (1 << 0)
#define P2_6_EMC_A10                            (2 << 0)
#define P2_6_USB0_IND0                          (3 << 0)
#define P2_6_GPIO5_6                            (4 << 0)
#define P2_6_CTIN_7                             (5 << 0)
#define P2_6_T3_CAP3                            (6 << 0)
#define P2_6_EMC_BLS1                           (7 << 0)

//P2_7
#define P2_7_GPIO0_7                            (0 << 0)
#define P2_7_CTOUT_1                            (1 << 0)
#define P2_7_U3_UCLK                            (2 << 0)
#define P2_7_EMC_A9                             (3 << 0)
#define P2_7_T3_MAT3                            (6 << 0)

//P2_8
#define P2_8_CTOUT_0                            (1 << 0)
#define P2_8_U3_DIR                             (2 << 0)
#define P2_8_EMC_A8                             (3 << 0)
#define P2_8_GPIO5_7                            (4 << 0)

//P2_9
#define P2_9_GPIO1_10                           (0 << 0)
#define P2_9_CTOUT_3                            (1 << 0)
#define P2_9_U3_BAUD                            (2 << 0)
#define P2_9_EMC_A0                             (3 << 0)

//P2_10
#define P2_10_GPIO0_14                          (0 << 0)
#define P2_10_CTOUT_2                           (1 << 0)
#define P2_10_U2_TXD                            (2 << 0)
#define P2_10_EMC_A1                            (3 << 0)

//P2_11
#define P2_11_GPIO1_11                          (0 << 0)
#define P2_11_CTOUT_5                           (1 << 0)
#define P2_11_U2_RXD                            (2 << 0)
#define P2_11_EMC_A2                            (3 << 0)

//P2_12
#define P2_12_GPIO1_12                          (0 << 0)
#define P2_12_CTOUT_4                           (1 << 0)
#define P2_12_EMC_A3                            (3 << 0)
#define P2_12_U2_UCLK                           (7 << 0)

//P2_13
#define P2_13_GPIO1_13                          (0 << 0)
#define P2_13_CTIN_4                            (1 << 0)
#define P2_13_EMC_A4                            (3 << 0)
#define P2_13_U2_DIR                            (7 << 0)

//P3_0
#define P3_0_I2S0_RX_SCK                        (0 << 0)
#define P3_0_I2S0_RX_MCLK                       (1 << 0)
#define P3_0_I2S0_TX_SCK                        (2 << 0)
#define P3_0_I2S0_TX_MCLK                       (3 << 0)
#define P3_0_SSP0_SCK                           (4 << 0)

//P3_1
#define P3_1_I2S0_TX_WS                         (0 << 0)
#define P3_1_I2S0_RX_WS                         (1 << 0)
#define P3_1_CAN0_RD                            (2 << 0)
#define P3_1_USB1_IND1                          (3 << 0)
#define P3_1_GPIO5_8                            (4 << 0)
#define P3_1_LCD_VD15                           (6 << 0)

//P3_2
#define P3_2_I2S0_TX_SDA                        (0 << 0)
#define P3_2_I2S0_RX_SDA                        (1 << 0)
#define P3_2_CAN0_TD                            (2 << 0)
#define P3_2_USB1_IND0                          (3 << 0)
#define P3_2_GPIO5_9                            (4 << 0)
#define P3_2_LCD_VD14                           (6 << 0)

//P3_3
#define P3_3_SSP0_SCK                           (2 << 0)
#define P3_3_SPIFI_SCK                          (3 << 0)
#define P3_3_CGU_OUT1                           (4 << 0)
#define P3_3_I2S0_TX_MCLK                       (6 << 0)
#define P3_3_I2S1_TX_SCK                        (7 << 0)

//P3_4
#define P3_4_GPIO1_14                           (0 << 0)
#define P3_4_SPIFI_SIO3                         (3 << 0)
#define P3_4_U1_TXD                             (4 << 0)
#define P3_4_I2S0_TX_WS                         (5 << 0)
#define P3_4_I2S1_RX_SDA                        (6 << 0)
#define P3_4_LCD_VD13                           (7 << 0)

//P3_5
#define P3_5_GPIO1_15                           (0 << 0)
#define P3_5_SPIFI_SIO2                         (3 << 0)
#define P3_5_U1_RXD                             (4 << 0)
#define P3_5_I2S0_TX_SDA                        (5 << 0)
#define P3_5_I2S1_RX_WS                         (6 << 0)
#define P3_5_LCD_VD12                           (7 << 0)

//P3_6
#define P3_6_GPIO0_6                            (0 << 0)
#define P3_6_SSP0_SSEL                          (2 << 0)
#define P3_6_SPIFI_MISO                         (3 << 0)
#define P3_6_SSP0_MISO                          (5 << 0)

//P3_7
#define P3_7_SSP0_MISO                          (2 << 0)
#define P3_7_SPIFI_MOSI                         (3 << 0)
#define P3_7_GPIO5_10                           (4 << 0)
#define P3_7_SSP0_MOSI                          (5 << 0)

//P3_8
#define P3_8_SSP0_MOSI                          (2 << 0)
#define P3_8_SPIFI_CS                           (3 << 0)
#define P3_8_GPIO5_11                           (4 << 0)
#define P3_8_SSP0_SSEL                          (5 << 0)

//P4_0
#define P4_0_GPIO2_0                            (0 << 0)
#define P4_0_MCOA0                              (1 << 0)
#define P4_0_NMI                                (2 << 0)
#define P4_0_LCD_VD13                           (5 << 0)
#define P4_0_U3_UCLK                            (6 << 0)

//P4_1
#define P4_1_GPIO2_1                            (0 << 0)
#define P4_1_CTOUT_1                            (1 << 0)
#define P4_1_LCD_VD0                            (2 << 0)
#define P4_1_LCD_VD19                           (5 << 0)
#define P4_1_U3_TXD                             (6 << 0)
#define P4_1_ENET_COL                           (7 << 0)

//P4_2
#define P4_2_GPIO2_2                            (0 << 0)
#define P4_2_CTOUT_0                            (1 << 0)
#define P4_2_LCD_VD3                            (2 << 0)
#define P4_2_LCD_VD12                           (5 << 0)
#define P4_2_U3_RXD                             (6 << 0)

//P4_3
#define P4_3_GPIO2_3                            (0 << 0)
#define P4_3_CTOUT_3                            (1 << 0)
#define P4_3_LCD_VD2                            (2 << 0)
#define P4_3_LCD_VD21                           (5 << 0)
#define P4_3_U3_BAUD                            (6 << 0)

//P4_4
#define P4_4_GPIO2_4                            (0 << 0)
#define P4_4_CTOUT_2                            (1 << 0)
#define P4_4_LCD_VD1                            (2 << 0)
#define P4_4_LCD_VD20                           (5 << 0)
#define P4_4_U3_DIR                             (6 << 0)

//P4_5
#define P4_5_GPIO2_5                            (0 << 0)
#define P4_5_CTOUT_5                            (1 << 0)
#define P4_5_LCD_FP                             (2 << 0)

//P4_6
#define P4_6_GPIO2_6                            (0 << 0)
#define P4_6_CTOUT_4                            (1 << 0)
#define P4_6_LCD_ENAB_LCDM                      (2 << 0)

//P4_7
#define P4_7_LCD_DCLK                           (0 << 0)
#define P4_7_GP_CLKIN                           (1 << 0)
#define P4_7_I2S1_TX_SCK                        (6 << 0)
#define P4_7_I2S0_TX_SCK                        (7 << 0)

//P4_8
#define P4_8_CTIN_5                             (1 << 0)
#define P4_8_LCD_VD9                            (2 << 0)
#define P4_8_GPIO5_12                           (4 << 0)
#define P4_8_LCD_VD22                           (5 << 0)
#define P4_8_CAN1_TD                            (6 << 0)

//P4_9
#define P4_9_CTIN_6                             (1 << 0)
#define P4_9_LCD_VD11                           (2 << 0)
#define P4_9_GPIO5_13                           (4 << 0)
#define P4_9_LCD_VD15                           (5 << 0)
#define P4_9_CAN1_RD                            (6 << 0)

//P4_10
#define P4_10_CTIN_2                            (1 << 0)
#define P4_10_LCD_VD10                          (2 << 0)
#define P4_10_GPIO5_14                          (4 << 0)
#define P4_10_LCD_VD14                          (5 << 0)

//P5_0
#define P5_0_GPIO2_9                            (0 << 0)
#define P5_0_MCOB2                              (1 << 0)
#define P5_0_EMC_D12                            (2 << 0)
#define P5_0_U1_DSR                             (4 << 0)
#define P5_0_T1_CAP0                            (5 << 0)

//P5_1
#define P5_1_GPIO2_10                           (0 << 0)
#define P5_1_MCI2                               (1 << 0)
#define P5_1_EMC_D13                            (2 << 0)
#define P5_1_U1_DTR                             (4 << 0)
#define P5_1_T1_CAP1                            (5 << 0)

//P5_2
#define P5_2_GPIO2_11                           (0 << 0)
#define P5_2_MCI1                               (1 << 0)
#define P5_2_EMC_D14                            (2 << 0)
#define P5_2_U1_RTS                             (4 << 0)
#define P5_2_T1_CAP2                            (5 << 0)

//P5_3
#define P5_3_GPIO2_12                           (0 << 0)
#define P5_3_MCI0                               (1 << 0)
#define P5_3_EMC_D15                            (2 << 0)
#define P5_3_U1_RI                              (4 << 0)
#define P5_3_T1_CAP3                            (5 << 0)

//P5_4
#define P5_4_GPIO2_13                           (0 << 0)
#define P5_4_MCOB0                              (1 << 0)
#define P5_4_EMC_D8                             (2 << 0)
#define P5_4_U1_CTS                             (4 << 0)
#define P5_4_T1_MAT0                            (5 << 0)

//P5_5
#define P5_5_GPIO2_14                           (0 << 0)
#define P5_5_MCOA1                              (1 << 0)
#define P5_5_EMC_D9                             (2 << 0)
#define P5_5_U1_DCD                             (4 << 0)
#define P5_5_T1_MAT1                            (5 << 0)

//P5_6
#define P5_6_GPIO2_15                           (0 << 0)
#define P5_6_MCOB1                              (1 << 0)
#define P5_6_EMC_D10                            (2 << 0)
#define P5_6_U1_TXD                             (4 << 0)
#define P5_6_T1_MAT2                            (5 << 0)

//P5_7
#define P5_7_GPIO2_7                            (0 << 0)
#define P5_7_MCOA2                              (1 << 0)
#define P5_7_EMC_D11                            (2 << 0)
#define P5_7_U1_RXD                             (4 << 0)
#define P5_7_T1_MAT3                            (5 << 0)

//P6_0
#define P6_0_I2S0_RX_MCLK                       (1 << 0)
#define P6_0_I2S0_RX_SCK                        (4 << 0)

//P6_1
#define P6_1_GPIO3_0                            (0 << 0)
#define P6_1_EMC_DYCS1                          (1 << 0)
#define P6_1_U0_UCLK                            (2 << 0)
#define P6_1_I2S0_RX_WS                         (3 << 0)
#define P6_1_T2_CAP0                            (5 << 0)

//P6_2
#define P6_2_GPIO3_1                            (0 << 0)
#define P6_2_EMC_CKEOUT1                        (1 << 0)
#define P6_2_U0_DIR                             (2 << 0)
#define P6_2_I2S0_RX_SDA                        (3 << 0)
#define P6_2_T2_CAP1                            (5 << 0)

//P6_3
#define P6_3_GPIO3_2                            (0 << 0)
#define P6_3_USB0_PWR_EN                        (1 << 0)
#define P6_3_EMC_CS1                            (3 << 0)
#define P6_3_T2_CAP2                            (5 << 0)

//P6_4
#define P6_4_GPIO3_3                            (0 << 0)
#define P6_4_CTIN_6                             (1 << 0)
#define P6_4_U0_TXD                             (2 << 0)
#define P6_4_EMC_CAS                            (3 << 0)

//P6_5
#define P6_5_GPIO3_4                            (0 << 0)
#define P6_5_CTOUT_6                            (1 << 0)
#define P6_5_U0_RXD                             (2 << 0)
#define P6_5_EMC_RAS                            (3 << 0)

//P6_6
#define P6_6_GPIO0_5                            (0 << 0)
#define P6_6_EMC_BLS1                           (1 << 0)
#define P6_6_USB0_PWR_FAULT                     (3 << 0)
#define P6_6_T2_CAP3                            (5 << 0)

//P6_7
#define P6_7_EMC_A15                            (1 << 0)
#define P6_7_USB0_IND1                          (3 << 0)
#define P6_7_GPIO5_15                           (4 << 0)
#define P6_7_T2_MAT0                            (5 << 0)

//P6_8
#define P6_8_EMC_A14                            (1 << 0)
#define P6_8_USB0_IND0                          (3 << 0)
#define P6_8_GPIO5_16                           (4 << 0)
#define P6_8_T2_MAT1                            (5 << 0)

//P6_9
#define P6_9_GPIO3_5                            (0 << 0)
#define P6_9_EMC_DYCS0                          (3 << 0)
#define P6_9_T2_MAT2                            (5 << 0)

//P6_10
#define P6_10_GPIO3_6                           (0 << 0)
#define P6_10_MCABORT                           (1 << 0)
#define P6_10_EMC_DQMOUT1                       (3 << 0)

//P6_11
#define P6_11_GPIO3_7                           (0 << 0)
#define P6_11_EMC_CKEOUT0                       (3 << 0)
#define P6_11_T2_MAT3                           (5 << 0)

//P6_12
#define P6_12_GPIO2_8                           (0 << 0)
#define P6_12_CTOUT_7                           (1 << 0)
#define P6_12_EMC_DQMOUT0                       (3 << 0)

//P7_0
#define P7_0_GPIO3_8                            (0 << 0)
#define P7_0_CTOUT_14                           (1 << 0)
#define P7_0_LCD_LE                             (3 << 0)

//P7_1
#define P7_1_GPIO3_9                            (0 << 0)
#define P7_1_CTOUT_15                           (1 << 0)
#define P7_1_I2S0_TX_WS                         (2 << 0)
#define P7_1_LCD_VD19                           (3 << 0)
#define P7_1_LCD_VD7                            (4 << 0)
#define P7_1_U2_TXD                             (6 << 0)

//P7_2
#define P7_2_GPIO3_10                           (0 << 0)
#define P7_2_CTIN_4                             (1 << 0)
#define P7_2_I2S0_TX_SDA                        (2 << 0)
#define P7_2_LCD_VD18                           (3 << 0)
#define P7_2_LCD_VD6                            (4 << 0)
#define P7_2_U2_RXD                             (6 << 0)

//P7_3
#define P7_3_GPIO3_11                           (0 << 0)
#define P7_3_CTIN_3                             (1 << 0)
#define P7_3_LCD_VD17                           (3 << 0)
#define P7_3_LCD_VD5                            (4 << 0)

//P7_4
#define P7_4_GPIO3_12                           (0 << 0)
#define P7_4_CTOUT_13                           (1 << 0)
#define P7_4_LCD_VD16                           (3 << 0)
#define P7_4_LCD_VD4                            (4 << 0)
#define P7_4_TRACEDATA_0                        (5 << 0)

//P7_5
#define P7_5_GPIO3_13                           (0 << 0)
#define P7_5_CTOUT_12                           (1 << 0)
#define P7_5_LCD_VD8                            (3 << 0)
#define P7_5_LCD_VD23                           (4 << 0)
#define P7_5_TRACEDATA_1                        (5 << 0)

//P7_6
#define P7_6_GPIO3_14                           (0 << 0)
#define P7_6_CTOUT_11                           (1 << 0)
#define P7_6_LCD_LP                             (3 << 0)
#define P7_6_TRACEDATA_2                        (5 << 0)

//P7_7
#define P7_7_GPIO3_15                           (0 << 0)
#define P7_7_CTOUT_8                            (1 << 0)
#define P7_7_LCD_PWR                            (3 << 0)
#define P7_7_TRACEDATA_3                        (5 << 0)
#define P7_7_ENET_MDC                           (6 << 0)

//P8_0
#define P8_0_GPIO4_0                            (0 << 0)
#define P8_0_USB0_PWR_FAULT                     (1 << 0)
#define P8_0_MCI2                               (3 << 0)
#define P8_0_T0_MAT0                            (7 << 0)

//P8_1
#define P8_1_GPIO4_1                            (0 << 0)
#define P8_1_USB0_IND1                          (1 << 0)
#define P8_1_MCI1                               (3 << 0)
#define P8_1_T0_MAT1                            (7 << 0)

//P8_2
#define P8_2_GPIO4_2                            (0 << 0)
#define P8_2_USB0_IND0                          (1 << 0)
#define P8_2_MCI0                               (3 << 0)
#define P8_2_T0_MAT2                            (7 << 0)

//P8_3
#define P8_3_GPIO4_3                            (0 << 0)
#define P8_3_USB1_ULPI_D2                       (1 << 0)
#define P8_3_LCD_VD12                           (3 << 0)
#define P8_3_LCD_VD19                           (4 << 0)
#define P8_3_T0_MAT3                            (7 << 0)

//P8_4
#define P8_4_GPIO4_4                            (0 << 0)
#define P8_4_USB1_ULPI_D1                       (1 << 0)
#define P8_4_LCD_VD7                            (3 << 0)
#define P8_4_LCD_VD16                           (4 << 0)
#define P8_4_T0_CAP0                            (7 << 0)

//P8_5
#define P8_5_GPIO4_5                            (0 << 0)
#define P8_5_USB1_ULPI_D0                       (1 << 0)
#define P8_5_LCD_VD6                            (3 << 0)
#define P8_5_LCD_VD8                            (4 << 0)
#define P8_5_T0_CAP1                            (7 << 0)

//P8_6
#define P8_6_GPIO4_6                            (0 << 0)
#define P8_6_USB1_ULPI_NXT                      (1 << 0)
#define P8_6_LCD_VD5                            (3 << 0)
#define P8_6_LCD_LP                             (4 << 0)
#define P8_6_T0_CAP2                            (7 << 0)

//P8_7
#define P8_7_GPIO4_7                            (0 << 0)
#define P8_7_USB1_ULPI_STP                      (1 << 0)
#define P8_7_LCD_VD4                            (3 << 0)
#define P8_7_LCD_PWR                            (4 << 0)
#define P8_7_T0_CAP3                            (7 << 0)

//P8_8
#define P8_8_USB1_ULPI_CLK                      (1 << 0)
#define P8_8_CGU_OUT0                           (6 << 0)
#define P8_8_I2S1_TX_MCLK                       (7 << 0)

//P9_0
#define P9_0_GPIO4_12                           (0 << 0)
#define P9_0_MCABORT                            (1 << 0)
#define P9_0_ENET_CRS                           (5 << 0)
#define P9_0_SSP0_SSEL                          (7 << 0)

//P9_1
#define P9_1_GPIO4_13                           (0 << 0)
#define P9_1_MCOA2                              (1 << 0)
#define P9_1_I2S0_TX_WS                         (4 << 0)
#define P9_1_ENET_RX_ER                         (5 << 0)
#define P9_1_SSP0_MISO                          (7 << 0)

//P9_2
#define P9_2_GPIO4_14                           (0 << 0)
#define P9_2_MCOB2                              (1 << 0)
#define P9_2_I2S0_TX_SDA                        (4 << 0)
#define P9_2_ENET_RXD3                          (5 << 0)
#define P9_2_SSP0_MOSI                          (7 << 0)

//P9_3
#define P9_3_GPIO4_15                           (0 << 0)
#define P9_3_MCOA0                              (1 << 0)
#define P9_3_USB1_IND1                          (2 << 0)
#define P9_3_ENET_RXD2                          (5 << 0)
#define P9_3_U3_TXD                             (7 << 0)

//P9_4
#define P9_4_MCOB0                              (1 << 0)
#define P9_4_USB1_IND0                          (2 << 0)
#define P9_4_GPIO5_17                           (4 << 0)
#define P9_4_ENET_TXD2                          (5 << 0)
#define P9_4_U3_RXD                             (7 << 0)

//P9_5
#define P9_5_MCOA1                              (1 << 0)
#define P9_5_USB1_VBUS_EN                       (2 << 0)
#define P9_5_GPIO5_18                           (4 << 0)
#define P9_5_ENET_TXD3                          (5 << 0)
#define P9_5_U0_TXD                             (7 << 0)

//P9_6
#define P9_6_GPIO4_11                           (0 << 0)
#define P9_6_MCOB1                              (1 << 0)
#define P9_6_USB1_PWR_FAULT                     (2 << 0)
#define P9_6_ENET_COL                           (5 << 0)
#define P9_6_U0_RXD                             (7 << 0)

//PA_0
#define PA_0_I2S1_RX_MCLK                       (5 << 0)
#define PA_0_CGU_OUT1                           (6 << 0)

//PA_1
#define PA_1_GPIO4_8                            (0 << 0)
#define PA_1_QEI_IDX                            (1 << 0)
#define PA_1_U2_TXD                             (3 << 0)

//PA_2
#define PA_2_GPIO4_9                            (0 << 0)
#define PA_2_QEI_PHB                            (1 << 0)
#define PA_2_U2_RXD                             (3 << 0)

//PA_3
#define PA_3_GPIO4_10                           (0 << 0)
#define PA_3_QEI_PHA                            (1 << 0)

//PA_4
#define PA_4_CTOUT_9                            (1 << 0)
#define PA_4_EMC_A23                            (3 << 0)
#define PA_4_GPIO5_19                           (4 << 0)

//PB_0
#define PB_0_CTOUT_10                           (1 << 0)
#define PB_0_LCD_VD23                           (2 << 0)
#define PB_0_GPIO5_20                           (4 << 0)

//PB_1
#define PB_1_USB1_ULPI_DIR                      (1 << 0)
#define PB_1_LCD_VD22                           (2 << 0)
#define PB_1_GPIO5_21                           (4 << 0)
#define PB_1_CTOUT_6                            (5 << 0)

//PB_2
#define PB_2_USB1_ULPI_D7                       (1 << 0)
#define PB_2_LCD_VD21                           (2 << 0)
#define PB_2_GPIO5_22                           (4 << 0)
#define PB_2_CTOUT_7                            (5 << 0)

//PB_3
#define PB_3_USB1_ULPI_D6                       (1 << 0)
#define PB_3_LCD_VD20                           (2 << 0)
#define PB_3_GPIO5_23                           (4 << 0)
#define PB_3_CTOUT_8                            (5 << 0)

//PB_4
#define PB_4_USB1_ULPI_D5                       (1 << 0)
#define PB_4_LCD_VD15                           (2 << 0)
#define PB_4_GPIO5_24                           (4 << 0)
#define PB_4_CTIN_5                             (5 << 0)

//PB_5
#define PB_5_USB1_ULPI_D4                       (1 << 0)
#define PB_5_LCD_VD14                           (2 << 0)
#define PB_5_GPIO5_25                           (4 << 0)
#define PB_5_CTIN_7                             (5 << 0)
#define PB_5_LCD_PWR                            (6 << 0)

//PB_6
#define PB_6_USB1_ULPI_D3                       (1 << 0)
#define PB_6_LCD_VD13                           (2 << 0)
#define PB_6_GPIO5_26                           (4 << 0)
#define PB_6_CTIN_6                             (5 << 0)
#define PB_6_LCD_VD19                           (6 << 0)

//PC_0
#define PC_0_USB1_ULPI_CLK                      (1 << 0)
#define PC_0_ENET_RX_CLK LCD_DCLK               (3 << 0)
#define PC_0_SD_CLK                             (6 << 0)

//PC_1
#define PC_1_USB1_ULPI_D7                       (0 << 0)
#define PC_1_U1_RI                              (2 << 0)
#define PC_1_ENET_MDC                           (3 << 0)
#define PC_1_GPIO6_0                            (4 << 0)
#define PC_1_T3_CAP0                            (6 << 0)
#define PC_1_SD_VOLT0                           (7 << 0)

//PC_2
#define PC_2_USB1_ULPI_D6                       (0 << 0)
#define PC_2_U1_CTS                             (2 << 0)
#define PC_2_ENET_TXD2                          (3 << 0)
#define PC_2_GPIO6_1                            (4 << 0)
#define PC_2_SD_RST                             (7 << 0)

//PC_3
#define PC_3_USB1_ULPI_D5                       (0 << 0)
#define PC_3_U1_RTS                             (2 << 0)
#define PC_3_ENET_TXD3                          (3 << 0)
#define PC_3_GPIO6_2                            (4 << 0)
#define PC_3_SD_VOLT1                           (7 << 0)

//PC_4
#define PC_4_USB1_ULPI_D4                       (1 << 0)
#define PC_4_ENET_TX_EN                         (3 << 0)
#define PC_4_GPIO6_3                            (4 << 0)
#define PC_4_T3_CAP1                            (6 << 0)
#define PC_4_SD_DAT0                            (7 << 0)

//PC_5
#define PC_5_USB1_ULPI_D3                       (1 << 0)
#define PC_5_ENET_TX_ER                         (3 << 0)
#define PC_5_GPIO6_4                            (4 << 0)
#define PC_5_T3_CAP2                            (6 << 0)
#define PC_5_SD_DAT1                            (7 << 0)

//PC_6
#define PC_6_USB1_ULPI_D2                       (1 << 0)
#define PC_6_ENET_RXD2                          (3 << 0)
#define PC_6_GPIO6_5                            (4 << 0)
#define PC_6_T3_CAP3                            (6 << 0)
#define PC_6_SD_DAT2                            (7 << 0)

//PC_7
#define PC_7_USB1_ULPI_D1                       (1 << 0)
#define PC_7_ENET_RXD3                          (3 << 0)
#define PC_7_GPIO6_6                            (4 << 0)
#define PC_7_T3_MAT0                            (6 << 0)
#define PC_7_SD_DAT3                            (7 << 0)

//PC_8
#define PC_8_USB1_ULPI_D0                       (1 << 0)
#define PC_8_ENET_RX_DV                         (3 << 0)
#define PC_8_GPIO6_7                            (4 << 0)
#define PC_8_T3_MAT1                            (6 << 0)
#define PC_8_SD_CD                              (7 << 0)

//PC_9
#define PC_9_USB1_ULPI_NXT                      (1 << 0)
#define PC_9_ENET_RX_ER                         (3 << 0)
#define PC_9_GPIO6_8                            (4 << 0)
#define PC_9_T3_MAT2                            (6 << 0)
#define PC_9_SD_POW                             (7 << 0)

//PC_10
#define PC_10_USB1_ULPI_STP                     (1 << 0)
#define PC_10_U1_DSR                            (2 << 0)
#define PC_10_GPIO6_9                           (4 << 0)
#define PC_10_T3_MAT3                           (6 << 0)
#define PC_10_SD_CMD                            (7 << 0)

//PC_11
#define PC_11_USB1_ULPI_DIR                     (1 << 0)
#define PC_11_U1_DCD                            (2 << 0)
#define PC_11_GPIO6_10                          (4 << 0)
#define PC_11_SD_DAT4                           (7 << 0)

//PC_12
#define PC_12_U1_DTR                            (2 << 0)
#define PC_12_GPIO6_11                          (4 << 0)
#define PC_12_I2S0_TX_SDA                       (6 << 0)
#define PC_12_SD_DAT5                           (7 << 0)

//PC_13
#define PC_13_U1_TXD                            (2 << 0)
#define PC_13_GPIO6_12                          (4 << 0)
#define PC_13_I2S0_TX_WS                        (6 << 0)
#define PC_13_SD_DAT6                           (7 << 0)

//PC_14
#define PC_14_U1_RXD                            (2 << 0)
#define PC_14_GPIO6_13                          (4 << 0)
#define PC_14_ENET_TX_ER                        (6 << 0)
#define PC_14_SD_DAT7                           (7 << 0)

//PD_0
#define PD_0_CTOUT_15                           (1 << 0)
#define PD_0_EMC_DQMOUT2                        (2 << 0)
#define PD_0_GPIO6_14                           (4 << 0)

//PD_1
#define PD_1_EMC_CKEOUT2                        (2 << 0)
#define PD_1_GPIO6_15                           (4 << 0)
#define PD_1_SD_POW                             (5 << 0)

//PD_2
#define PD_2_CTOUT_7                            (1 << 0)
#define PD_2_EMC_D16                            (2 << 0)
#define PD_2_GPIO6_16                           (4 << 0)

//PD_3
#define PD_3_CTOUT_6                            (1 << 0)
#define PD_3_EMC_D17                            (2 << 0)
#define PD_3_GPIO6_17                           (4 << 0)

//PD_4
#define PD_4_CTOUT_8                            (1 << 0)
#define PD_4_EMC_D18                            (2 << 0)
#define PD_4_GPIO6_18                           (4 << 0)

//PD_5
#define PD_5_CTOUT_9                            (1 << 0)
#define PD_5_EMC_D19                            (2 << 0)
#define PD_5_GPIO6_19                           (4 << 0)

//PD_6
#define PD_6_CTOUT_10                           (1 << 0)
#define PD_6_EMC_D20                            (2 << 0)
#define PD_6_GPIO6_20                           (4 << 0)

//PD_7
#define PD_7_CTIN_5                             (1 << 0)
#define PD_7_EMC_D21                            (2 << 0)
#define PD_7_GPIO6_21                           (4 << 0)

//PD_8
#define PD_8_CTIN_6                             (1 << 0)
#define PD_8_EMC_D22                            (2 << 0)
#define PD_8_GPIO6_22                           (4 << 0)

//PD_9
#define PD_9_CTOUT_13                           (1 << 0)
#define PD_9_EMC_D23                            (2 << 0)
#define PD_9_GPIO6_23                           (4 << 0)

//PD_10
#define PD_10_CTIN_1                            (1 << 0)
#define PD_10_EMC_BLS3                          (2 << 0)
#define PD_10_GPIO6_24                          (4 << 0)

//PD_11
#define PD_11_EMC_CS3                           (2 << 0)
#define PD_11_GPIO6_25                          (4 << 0)
#define PD_11_USB1_ULPI_D0                      (5 << 0)
#define PD_11_CTOUT_14                          (6 << 0)

//PD_12
#define PD_12_EMC_CS2                           (2 << 0)
#define PD_12_GPIO6_26                          (4 << 0)
#define PD_12_CTOUT_10                          (6 << 0)

//PD_13
#define PD_13_CTIN_0                            (1 << 0)
#define PD_13_EMC_BLS2                          (2 << 0)
#define PD_13_GPIO6_27                          (4 << 0)
#define PD_13_CTOUT_13                          (6 << 0)

//PD_14
#define PD_14_EMC_DYCS2                         (2 << 0)
#define PD_14_GPIO6_28                          (4 << 0)
#define PD_14_CTOUT_11                          (6 << 0)

//PD_15
#define PD_15_EMC_A17                           (2 << 0)
#define PD_15_GPIO6_29                          (4 << 0)
#define PD_15_SD_WP                             (5 << 0)
#define PD_15_CTOUT_8                           (6 << 0)

//PD_16
#define PD_16_EMC_A16                           (2 << 0)
#define PD_16_GPIO6_30                          (4 << 0)
#define PD_16_SD_VOLT2                          (5 << 0)
#define PD_16_CTOUT_12                          (6 << 0)

//PE_0
#define PE_0_EMC_A18                            (3 << 0)
#define PE_0_GPIO7_0                            (4 << 0)
#define PE_0_CAN1_TD                            (5 << 0)

//PE_1
#define PE_1_EMC_A19                            (3 << 0)
#define PE_1_GPIO7_1                            (4 << 0)
#define PE_1_CAN1_RD                            (5 << 0)

//PE_2
#define PE_2_ADCTRIG0                           (0 << 0)
#define PE_2_CAN0_RD                            (1 << 0)
#define PE_2_EMC_A20                            (3 << 0)
#define PE_2_GPIO7_2                            (4 << 0)

//PE_3
#define PE_3_CAN0_TD                            (1 << 0)
#define PE_3_ADCTRIG1                           (2 << 0)
#define PE_3_EMC_A21                            (3 << 0)
#define PE_3_GPIO7_3                            (4 << 0)

//PE_4
#define PE_4_NMI                                (1 << 0)
#define PE_4_EMC_A22                            (3 << 0)
#define PE_4_GPIO7_4                            (4 << 0)

//PE_5
#define PE_5_CTOUT_3                            (1 << 0)
#define PE_5_U1_RTS                             (2 << 0)
#define PE_5_EMC_D24                            (3 << 0)
#define PE_5_GPIO7_5                            (4 << 0)

//PE_6
#define PE_6_CTOUT_2                            (1 << 0)
#define PE_6_U1_RI                              (2 << 0)
#define PE_6_EMC_D25                            (3 << 0)
#define PE_6_GPIO7_6                            (4 << 0)

//PE_7
#define PE_7_CTOUT_5                            (1 << 0)
#define PE_7_U1_CTS                             (2 << 0)
#define PE_7_EMC_D26                            (3 << 0)
#define PE_7_GPIO7_7                            (4 << 0)

//PE_8
#define PE_8_CTOUT_4                            (1 << 0)
#define PE_8_U1_DSR                             (2 << 0)
#define PE_8_EMC_D27                            (3 << 0)
#define PE_8_GPIO7_8                            (4 << 0)

//PE_9
#define PE_9_CTIN_4                             (1 << 0)
#define PE_9_U1_DCD                             (2 << 0)
#define PE_9_EMC_D28                            (3 << 0)
#define PE_9_GPIO7_9                            (4 << 0)

//PE_10
#define PE_10_CTIN_3                            (1 << 0)
#define PE_10_U1_DTR                            (2 << 0)
#define PE_10_EMC_D29                           (3 << 0)
#define PE_10_GPIO7_10                          (4 << 0)

//PE_11
#define PE_11_CTOUT_12                          (1 << 0)
#define PE_11_U1_TXD                            (2 << 0)
#define PE_11_EMC_D30                           (3 << 0)
#define PE_11_GPIO7_11                          (4 << 0)

//PE_12
#define PE_12_CTOUT_11                          (1 << 0)
#define PE_12_U1_RXD                            (2 << 0)
#define PE_12_EMC_D31                           (3 << 0)
#define PE_12_GPIO7_12                          (4 << 0)

//PE_13
#define PE_13_CTOUT_14                          (1 << 0)
#define PE_13_I2C1_SDA                          (2 << 0)
#define PE_13_EMC_DQMOUT                        (3 << 0)
#define PE_13_GPIO7_13                          (4 << 0)

//PE_14
#define PE_14_EMC_DYCS3                         (3 << 0)
#define PE_14_GPIO7_14                          (4 << 0)

//PE_15
#define PE_15_CTOUT_0                           (1 << 0)
#define PE_15_I2C1_SCL                          (2 << 0)
#define PE_15_EMC_CKEOUT                        (3 << 0)
#define PE_15_GPIO7_15                          (4 << 0)

//PF_0
#define PF_0_SSP0_SCK                           (0 << 0)
#define PF_0_GP_CLKIN                           (1 << 0)
#define PF_0_I2S1_TX_MCLK                       (7 << 0)

//PF_1
#define PF_1_SSP0_SSEL                          (2 << 0)
#define PF_1_GPIO7_16                           (4 << 0)

//PF_2
#define PF_2_U3_TXD                             (1 << 0)
#define PF_2_SSP0_MISO                          (2 << 0)
#define PF_2_GPIO7_17                           (4 << 0)

//PF_3
#define PF_3_U3_RXD                             (1 << 0)
#define PF_3_SSP0_MOSI                          (2 << 0)
#define PF_3_GPIO7_18                           (4 << 0)

//PF_4
#define PF_4_SSP1_SCK                           (0 << 0)
#define PF_4_GP_CLKIN                           (1 << 0)
#define PF_4_TRACECLK                           (2 << 0)
#define PF_4_I2S0_TX_MCLK                       (6 << 0)
#define PF_4_I2S0_RX_SCK                        (7 << 0)

//PF_5
#define PF_5_U3_UCLK                            (1 << 0)
#define PF_5_SSP1_SSEL                          (2 << 0)
#define PF_5_TRACEDATA_0                        (3 << 0)
#define PF_5_GPIO7_19                           (4 << 0)

//PF_6
#define PF_6_U3_DIR                             (1 << 0)
#define PF_6_SSP1_MISO                          (2 << 0)
#define PF_6_TRACEDATA_1                        (3 << 0)
#define PF_6_GPIO7_20                           (4 << 0)
#define PF_6_I2S1_TX_SDA                        (7 << 0)

//PF_7
#define PF_7_U3_BAUD                            (1 << 0)
#define PF_7_SSP1_MOSI                          (2 << 0)
#define PF_7_TRACEDATA_2                        (3 << 0)
#define PF_7_GPIO7_21                           (4 << 0)
#define PF_7_I2S1_TX_WS                         (7 << 0)

//PF_8
#define PF_8_U0_UCLK                            (1 << 0)
#define PF_8_CTIN_2                             (2 << 0)
#define PF_8_TRACEDATA_3                        (3 << 0)
#define PF_8_GPIO7_22                           (4 << 0)

//PF_9
#define PF_9_U0_DIR                             (1 << 0)
#define PF_9_CTOUT_1                            (2 << 0)
#define PF_9_GPIO7_23                           (4 << 0)

//PF_10
#define PF_10_U0_TXD                            (1 << 0)
#define PF_10_GPIO7_24                          (4 << 0)
#define PF_10_SD_WP                             (6 << 0)

//PF_11
#define PF_11_U0_RXD                            (1 << 0)
#define PF_11_GPIO7_25                          (4 << 0)
#define PF_11_SD_VOLT2                          (6 << 0)

//CLK_0
#define CLK_0_EMC_CLK0                          (0 << 0)
#define CLK_0_CLKOUT                            (1 << 0)
#define CLK_0_SD_CLK                            (4 << 0)
#define CLK_0_EMC_CLK01                         (5 << 0)
#define CLK_0_SSP1_SCK                          (6 << 0)
#define CLK_0_ENET_TX_CLK                       (7 << 0)
#define CLK_0_ENET_REF_CLK                      (7 << 0)

//CLK_1
#define CLK_1_EMC_CLK1                          (0 << 0)
#define CLK_1_CLKOUT                            (1 << 0)
#define CLK_1_CGU_OUT0                          (5 << 0)
#define CLK_1_I2S1_TX_MCLK                      (7 << 0)

//CLK_2
#define CLK_2_EMC_CLK3                          (0 << 0)
#define CLK_2_CLKOUT                            (1 << 0)
#define CLK_2_SD_CLK                            (4 << 0)
#define CLK_2_EMC_CLK23                         (5 << 0)
#define CLK_2_I2S0_TX_MCLK                      (6 << 0)
#define CLK_2_I2S1_RX_SCK                       (7 << 0)

//CLK_3
#define CLK_3_EMC_CLK2                          (0 << 0)
#define CLK_3_CLKOUT                            (1 << 0)
#define CLK_3_CGU_OUT1                          (5 << 0)
#define CLK_3_I2S1_RX_SCK                       (7 << 0)


/******************************************************************************/
/*                                                                            */
/*                            USART                                           */
/*                                                                            */
/******************************************************************************/

/**********  Bit definition for LCR register  *********************************/
#define USART0_LCR_WLS_5                        (0 << 0)
#define USART0_LCR_WLS_6                        (1 << 0)
#define USART0_LCR_WLS_7                        (2 << 0)
#define USART0_LCR_WLS_8                        (3 << 0)

#define USART0_LCR_SBS_1                        (0 << 2)
#define USART0_LCR_SBS_2                        (1 << 2)

#define USART0_LCR_PS_ODD                       (0 << 4)
#define USART0_LCR_PS_EVEN                      (1 << 4)
#define USART0_LCR_PS_FORCE_1                   (2 << 4)
#define USART0_LCR_PS_FORCE_0                   (3 << 4)

/**********  Bit definition for IIR register  *********************************/
#define USART0_IIR_INTID_MODEM_STATUS           (0 << 1)
#define USART0_IIR_INTID_THRE                   (1 << 1)
#define USART0_IIR_INTID_RDA                    (2 << 1)
#define USART0_IIR_INTID_RLS                    (3 << 1)
#define USART0_IIR_INTID_CTI                    (6 << 1)

/******************************************************************************/
/*                                                                            */
/*                               USB0                                         */
/*                                                                            */
/******************************************************************************/

/******  Bit definition for USB0_USBSTS_D register  ***************************/
#define USB0_USBSTS_D_SEI_Msk                   (1 << 4)                        //forgotten in vendor shipped LPC18xx.h

/*****  Bit definition for USB0_USBINTR_D register  ***************************/
#define USB0_USBINTR_D_SEE_Msk                  (1 << 4)                        //forgotten in vendor shipped LPC18xx.h

/*******  Bit definition for USB0_USBMODE register  ***************************/
#define USB0_USBMODE_CM_RESERVED                (0 << 0)
#define USB0_USBMODE_CM_DEVICE                  (2 << 0)
#define USB0_USBMODE_CM_HOST                    (3 << 0)

/*********  Bit definition for USB0_DQH_CAPA **********************************/
#define USB0_DQH_CAPA_IOS_Msk                   (1 << 15)
#define USB0_DQH_CAPA_MAX_PACKET_LENGTH_Pos     16
#define USB0_DQH_CAPA_MAX_PACKET_LENGTH_Msk     (0x7ff << 16)
#define USB0_DQH_CAPA_ZLT_Msk                   (1 << 29)
#define USB0_DQH_CAPA_MULT_NONE                 (0 << 30)
#define USB0_DQH_CAPA_MULT_ONE                  (1 << 30)
#define USB0_DQH_CAPA_MULT_TWO                  (2 << 30)
#define USB0_DQH_CAPA_MULT_THREE                (3 << 30)

/*********  Bit definition for USB0_DQH_NEXT **********************************/
#define USB0_DQH_NEXT_T_Msk                     (1 << 0)

/****  Bit definition for USB0_DTD_SIZE_FLAGS *********************************/
#define USB0_DTD_SIZE_FLAGS_SIZE_Pos            16
#define USB0_DTD_SIZE_FLAGS_SIZE_Msk            (0x7fff << 16)
#define USB0_DTD_SIZE_FLAGS_IOC_Msk             (1 << 15)
#define USB0_DTD_SIZE_FLAGS_MULT_ONE            (1 << 10)
#define USB0_DTD_SIZE_FLAGS_MULT_TWO            (2 << 10)
#define USB0_DTD_SIZE_FLAGS_MULT_THREE          (3 << 10)
#define USB0_DTD_SIZE_FLAGS_STATUS_Msk          (0xff << 0)
#define USB0_DTD_SIZE_FLAGS_ACTIVE_Msk          (1 << 7)
#define USB0_DTD_SIZE_FLAGS_HALTED_Msk          (1 << 6)
#define USB0_DTD_SIZE_FLAGS_BUFFER_ERR_Msk      (1 << 5)
#define USB0_DTD_SIZE_FLAGS_TRANSACTION_ERR_Msk (1 << 3)

/******  Bit definition for USB0_ENDPTCTRL register ***************************/
#define USB0_ENDPTCTRL_S_Msk                    (1 << 0)
#define USB0_ENDPTCTRL_T_Pos                    2
#define USB0_ENDPTCTRL_I_Msk                    (1 << 5)
#define USB0_ENDPTCTRL_R_Msk                    (1 << 6)
#define USB0_ENDPTCTRL_E_Msk                    (1 << 7)

/******************************************************************************/
/*                                                                            */
/*                              EEPROM                                        */
/*                                                                            */
/******************************************************************************/

typedef struct {                                /*!< (@ 0x4000e000) EEPROM Structure                */
  uint32_t CMD;                                 /*!< (@ 0x4000e000) command register                */
  uint32_t reserved1;
  uint32_t RWSTATE;                             /*!< (@ 0x4000e008) read wait state register        */
  uint32_t AUTOPROG;                            /*!< (@ 0x4000e00c) auto programming register       */
  uint32_t WSTATE;                              /*!< (@ 0x4000e010) wait state register             */
  uint32_t CLKDIV;                              /*!< (@ 0x4000e014) clock divider register          */
  uint32_t PWRDWN;                              /*!< (@ 0x4000e018) power-down register             */
  uint32_t reserved2[1007];
  uint32_t INTENCLR;                            /*!< (@ 0x4000efd8) interrupt enable clear register */
  uint32_t INTENSET;                            /*!< (@ 0x4000efdc) interrupt enable set register   */
  uint32_t INTSTAT;                             /*!< (@ 0x4000efe0) interrupt status register       */
  uint32_t INTEN;                               /*!< (@ 0x4000efe4) interrupt enable register       */
  uint32_t INTSTATCLR;                          /*!< (@ 0x4000efe8) interrupt status clear register */
  uint32_t INTSTATSET;                          /*!< (@ 0x4000efec) interrupt status set register   */
} LPC_EEPROM_Type;

#define LPC_EEPROM_BASE                         0x4000e000
#define LPC_EEPROM                              ((LPC_EEPROM_Type*) LPC_EEPROM_BASE)

/**********  Bit definition for LPC_EEPROM_CMD ***************************/
#define LPC_EEPROM_CMD_ERASE_PROGRAM            6

/******  Bit definition for LPC_EEPROM_RWSTATE ***************************/
#define LPC_EEPROM_RWSTATE_RPHASE2_Pos          0
#define LPC_EEPROM_RWSTATE_RPHASE2_Msk          (0xff << 0)
#define LPC_EEPROM_RWSTATE_RPHASE1_Pos          8
#define LPC_EEPROM_RWSTATE_RPHASE1_Msk          (0xff << 8)

/*****  Bit definition for LPC_EEPROM_AUTOPROG ***************************/
#define LPC_EEPROM_AUTOPROG_OFF                 (0 << 0)
#define LPC_EEPROM_AUTOPROG_SINGLE_WORD         (1 << 0)
#define LPC_EEPROM_AUTOPROG_LAST_WORD           (2 << 0)

/*******  Bit definition for LPC_EEPROM_WSTATE ***************************/
#define LPC_EEPROM_WSTATE_RPHASE3_Pos           0
#define LPC_EEPROM_WSTATE_RPHASE3_Msk           (0xff << 0)
#define LPC_EEPROM_WSTATE_RPHASE2_Pos           8
#define LPC_EEPROM_WSTATE_RPHASE2_Msk           (0xff << 8)
#define LPC_EEPROM_WSTATE_RPHASE1_Pos           16
#define LPC_EEPROM_WSTATE_RPHASE1_Msk           (0xff << 16)
#define LPC_EEPROM_WSTATE_LCK_PARWEP_Msk        (1 << 31)

/*******  Bit definition for LPC_EEPROM_CLKDIV ***************************/
#define LPC_EEPROM_CLKDIV_Msk                   (0xffff << 0)

/*******  Bit definition for LPC_EEPROM_PWRDWN ***************************/
#define LPC_EEPROM_PWRDWN_Msk                   (1 << 0)

/******  Bit definition for LPC_EEPROM_INTENCLR ***************************/
#define LPC_EEPROM_INTENCLR_PROG_CLR_EN_Msk     (1 << 2)

/******  Bit definition for LPC_EEPROM_INTENSET ***************************/
#define LPC_EEPROM_INTENSET_PROG_SET_EN_Msk     (1 << 2)

/*******  Bit definition for LPC_EEPROM_INTSTAT ***************************/
#define LPC_EEPROM_INTSTAT_END_OF_PROG_Msk      (1 << 2)

/*********  Bit definition for LPC_EEPROM_INTEN ***************************/
#define LPC_EEPROM_INTEN_EE_PROG_DONE_Msk       (1 << 2)

/******  Bit definition for LPC_EEPROM_INTSTATCLR *************************/
#define LPC_EEPROM_INTSTATCLR_PROG_CLR_ST_Msk   (1 << 2)

/******  Bit definition for LPC_EEPROM_INTSTATSET *************************/
#define LPC_EEPROM_INTSTATSET_PROG_SET_ST_Msk   (1 << 2)

/******************************************************************************/
/*                                                                            */
/*                                 I2C                                        */
/*                                                                            */
/******************************************************************************/

/***********  Bit definition for STAT register  *******************************/
#define I2C0_STAT_STATUS_Msk                    (0x1f << 3)

#define I2C0_STAT_START                         0x08                    /* A START condition has been transmitted */
#define I2C0_STAT_REPEATED_START                0x10                    /* A Repeated START condition has been transmitted */
#define I2C0_STAT_SLAW_ACK                      0x18                    /* SLA+W has been transmitted; ACK has been received */
#define I2C0_STAT_SLAW_NACK                     0x20                    /* SLA+W has been transmitted; NOT ACK has been received */
#define I2C0_STAT_DATW_ACK                      0x28                    /* Data byte in DAT has been transmitted; ACK has been received */
#define I2C0_STAT_DATW_NACK                     0x30                    /* Data byte in DAT has been transmitted; NOT ACK has been received */
#define I2C0_STAT_ARBITRATION_LOST              0x38                    /* Arbitration lost in SLA+R/W or Data bytes */
#define I2C0_STAT_SLAR_ACK                      0x40                    /* SLA+R has been transmitted; ACK has been received */
#define I2C0_STAT_SLAR_NACK                     0x48                    /* SLA+R has been transmitted; NOT ACK has been received */
#define I2C0_STAT_DATR_ACK                      0x50                    /* Data byte has been received; ACK has been returned */
#define I2C0_STAT_DATR_NACK                     0x58                    /* Data byte has been received; NOT ACK has been returned */
#define I2C0_STAT_SLAVE_SLAW_ACK                0x60                    /* Own SLA+W has been received; ACK has been returned */
#define I2C0_STAT_SLAVE_SLAW_NACK               0x68                    /* Arbitration lost in SLA+R/W as master;
                                                                           Own SLA+W has been received, ACK returned */
#define I2C0_STAT_SLAVE_GCA_ACK                 0x70                    /* General call address (0x00) has been received; ACK has been returned*/
#define I2C0_STAT_SLAVE_GCA_NACK                0x78                    /* Arbitration lost in SLA+R/W as master;
                                                                           General call address has been received, ACK has been returned */
#define I2C0_STAT_SLAVE_DATW_ACK                0x80                    /* Previously addressed with own SLV address;
                                                                           DATA has been received; ACK has been returned */
#define I2C0_STAT_SLAVE_DATW_NACK               0x88                    /* Previously addressed with own SLV address;
                                                                           DATA has been received; NOT ACK has been returned */
#define I2C0_STAT_SLAVE_GCA_DATW_ACK            0x90                    /* Previously addressed with General Call;
                                                                           DATA byte has been received; ACK has been returned */
#define I2C0_STAT_SLAVE_GCA_DATW_NACK           0x98                    /* Previously addressed with General Call;
                                                                           DATA byte has been received; NOT ACK has been returned */
#define I2C0_STAT_SLAVE_STOP                    0xA0                    /* A STOP condition or Repeated START condition has been
                                                                           received while still addressed as SLV/REC or SLV/TRX */
#define I2C0_STAT_SLAVE_SLAR_ACK                0xA8                    /* Own SLA+R has been received; ACK has been returned */
#define I2C0_STAT_SLAVE_SLAR_NACK               0xB0                    /* Arbitration lost in SLA+R/W as master;
                                                                           Own SLA+R has beenreceived, ACK has been returned */
#define I2C0_STAT_SLAVE_DATR_ACK                0xB8                    /* Data byte in DAT has been transmitted; ACK has been received */
#define I2C0_STAT_SLAVE_DATR_NACK               0xC0                    /* Data byte in DAT has been transmitted; NOT ACK has been received */
#define I2C0_STAT_SLAVE_DATR_LAST               0xC8                    /* Last data byte in DAT has been transmitted (AA = 0); ACK has been received */

#define I2C0_STAT_NO_INFO                       0xF8                    /* No relevant state information available; SI = 0 */
#define I2C0_STAT_ERROR                         0x00                    /* Bus error during MSTor selected slave modes, due to an illegal START or
                                                                           STOP condition. State 0x00 can also occur when interference causes the I2C block
                                                                           to enter an undefined state */

/******************************************************************************/
/*                                                                            */
/*                                 CREG                                       */
/*                                                                            */
/******************************************************************************/

/**********  Bit definition for CREG6 register  *******************************/
#define CREG_CREG6_ETHMODE_MII                  (0x0 << 0)
#define CREG_CREG6_ETHMODE_RMII                 (0x4 << 0)

/******************************************************************************/
/*                                                                            */
/*                                 SDMMC                                      */
/*                                                                            */
/******************************************************************************/

/*********  Bit definition for CLKSRC register  *******************************/
#define SDMMC_CLKSRC_CLK_SOURCE00               (0x0 << 0)
#define SDMMC_CLKSRC_CLK_SOURCE01               (0x1 << 0)
#define SDMMC_CLKSRC_CLK_SOURCE02               (0x2 << 0)
#define SDMMC_CLKSRC_CLK_SOURCE03               (0x3 << 0)

/*********  Bit definition for CLKENA register  *******************************/
#define SDMMC_CLKENA_CCLK_ENABLE                (0x1 << 0)
#define SDMMC_CLKENA_CCLK_LOW_POWER             (0x1 << 16)

/*********  Bit definition for CTYPE register  ********************************/
#define SDMMC_CTYPE_CARD_WIDTH0                 (0x1 << 0)
#define SDMMC_CTYPE_CARD_WIDTH1                 (0x1 << 16)

/*********  Bit definition for FIFOTH register  ********************************/
#define SDMMC_FIFOTH_DMA_MTS_1                  (0 << 28)
#define SDMMC_FIFOTH_DMA_MTS_4                  (1 << 28)
#define SDMMC_FIFOTH_DMA_MTS_8                  (2 << 28)
#define SDMMC_FIFOTH_DMA_MTS_16                 (3 << 28)
#define SDMMC_FIFOTH_DMA_MTS_32                 (4 << 28)
#define SDMMC_FIFOTH_DMA_MTS_64                 (5 << 28)
#define SDMMC_FIFOTH_DMA_MTS_128                (6 << 28)
#define SDMMC_FIFOTH_DMA_MTS_256                (7 << 28)

/*********  Bit definition for DESC0 descriptor  ********************************/
#define SDMMC_DESC0_DIC_Msk                     (1 << 1)
#define SDMMC_DESC0_LD_Msk                      (1 << 2)
#define SDMMC_DESC0_FS_Msk                      (1 << 3)
#define SDMMC_DESC0_CH_Msk                      (1 << 4)
#define SDMMC_DESC0_ER_Msk                      (1 << 5)
#define SDMMC_DESC0_CES_Msk                     (1 << 30)
#define SDMMC_DESC0_OWN_Msk                     (1 << 31)

/*********  Bit definition for DESC1 descriptor  ********************************/
#define SDMMC_DESC1_BS1_Pos                     0
#define SDMMC_DESC1_BS1_Msk                     (0x1fff << 0)
#define SDMMC_DESC1_BS2_Pos                     13
#define SDMMC_DESC1_BS2_Msk                     (0x1fff << 13)

#endif // LPC18UXX_BITS_H
