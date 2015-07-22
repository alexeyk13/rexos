/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef LPC11UXX_BITS_H
#define LPC11UXX_BITS_H

/******************************************************************************/
/*                                                                            */
/*                            SYSCON                                          */
/*                                                                            */
/******************************************************************************/

//TODO:

/******************************************************************************/
/*                                                                            */
/*                            GPIO                                            */
/*                                                                            */
/******************************************************************************/

/**********  Bit definition for SFS* registers  *******************************/
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

/*********  Bit definition for SFSUSB registers  *******************************/
#define SCU_SFSUSB_AIM                          (1 << 0)                /* Differential data input AIP/AIM.
                                                                           0 Going LOW with full speed edge rate
                                                                           1 Going HIGH with full speed edge rate */
#define SCU_SFSUSB_ESEA                         (1 << 1)                /* Control signal for differential input or single input.
                                                                           0 Reserved. Do not use.
                                                                           1 Single input. Enables USB1. Use with the on-chip full-speed PHY */
#define SCU_SFSUSB_EPD                          (1 << 2)                /* Enable pull-down connect.
                                                                           0 Pull-down disconnected
                                                                           1 Pull-down connected */
#define SCU_SFSUSB_EPWR                         (1 << 4)                /* Power mode.
                                                                           0 Power saving mode (Suspend mode)
                                                                           1 Normal mode */
#define SCU_SFSUSB_VBUS                         (1 << 5)                /* Enable the vbus_valid signal. This signal is monitored by the USB1 block. Use this bit
                                                                           for software de-bouncing of the VBUS sense signal or to indicate the VBUS state to the
                                                                           USB1 controller when the VBUS signal is present but the USB1_VBUS function is not
                                                                           connected in the SFSP2_5 register.
                                                                           Remark: The setting of this bit has no effect if the USB1_VBUS function of pin
                                                                           P2_5 is enabled through the SFSP2_5 register.
                                                                           0 VBUS signal LOW or inactive
                                                                           1 VBUS signal HIGH or active */

/*********  Bit definition for SFSI2C0 registers  *******************************/
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
#define P4_3_GPIO2_2                            (0 << 0)
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

//TODO:

#if 0
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

#endif

/******************************************************************************/
/*                                                                            */
/*                                 I2C                                        */
/*                                                                            */
/******************************************************************************/

//TODO:

#if 0

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

#endif

/******************************************************************************/
/*                                                                            */
/*                            Timer16/32                                      */
/*                                                                            */
/******************************************************************************/

//TODO:

#if 0

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

#endif

/******************************************************************************/
/*                                                                            */
/*                                USB                                         */
/*                                                                            */
/******************************************************************************/

//TODO:

#if 0

/************  Bit definition for DEVCMDSTAT register  ************************/
#define USB_DEVCMDSTAT_DEV_ADDR_POS            0                        /* USB device address. After bus reset, the address is reset to
                                                                           0x00. If the enable bit is set, the device will respond on packets
                                                                           for function address DEV_ADDR. When receiving a SetAddress
                                                                           Control Request from the USB host, software must program the
                                                                           new address before completing the status phase of the
                                                                           SetAddress Control Request */
#define USB_DEVCMDSTAT_DEV_ADDR_MASK           (0x7f << 0)

#define USB_DEVCMDSTAT_DEV_EN                  (1 << 7)                 /* USB device enable. If this bit is set, the HW will start respondingon
                                                                           packets for function address DEV_ADDR */
#define USB_DEVCMDSTAT_SETUP                   (1 << 8)                 /* SETUP token received. If a SETUP token is received and
                                                                           acknowledged by the device, this bit is set. As long as this bit is
                                                                           set all received IN and OUT tokens will be NAKed by HW. SW
                                                                           must clear this bit by writing a one. If this bit is zero, HW will
                                                                           handle the tokens to the CTRL EP0 as indicated by the CTRL
                                                                           EP0 IN and OUT data information programmed by SW */
#define USB_DEVCMDSTAT_PLL_ON                  (1 << 9)                 /* Always PLL Clock on:
                                                                           0 - USB_NeedClk functional
                                                                           1 - USB_NeedClk always 1. Clock will not be stopped in case of suspend */
#define USB_DEVCMDSTAT_LPM_SUP                 (1 << 11)                /* LPM Supported:
                                                                           0 - LPM not supported
                                                                           1 - LPM supported */
#define USB_DEVCMDSTAT_INTONNAK_AO             (1 << 12)                /* Interrupt on NAK for interrupt and bulk OUT EP
                                                                           0 - Only acknowledged packets generate an interrupt
                                                                           1 - Both acknowledged and NAKed packets generate interrupts */
#define USB_DEVCMDSTAT_INTONNAK_AI             (1 << 13)                /* Interrupt on NAK for interrupt and bulk IN EP
                                                                           0 - Only acknowledged packets generate an interrupt
                                                                           1 - Both acknowledged and NAKed packets generate interrupts */
#define USB_DEVCMDSTAT_INTONNAK_CO             (1 << 14)                /* Interrupt on NAK for interrupt and control OUT EP
                                                                           0 - Only acknowledged packets generate an interrupt
                                                                           1 - Both acknowledged and NAKed packets generate interrupts */
#define USB_DEVCMDSTAT_INTONNAK_CI             (1 << 15)                /* Interrupt on NAK for interrupt and control IN EP
                                                                           0 - Only acknowledged packets generate an interrupt
                                                                           1 - Both acknowledged and NAKed packets generate interrupts */
#define USB_DEVCMDSTAT_DCON                    (1 << 16)                /* Device status - connect.
                                                                           The connect bit must be set by SW to indicate that the device
                                                                           must signal a connect. The pull-up resistor on USB_DP will be
                                                                           enabled when this bit is set and the VbusDebounced bit is one */
#define USB_DEVCMDSTAT_DSUS                    (1 << 17)                /* Device status - suspend.
                                                                           The suspend bit indicates the current suspend state. It is set to
                                                                           1 when the device hasn’t seen any activity on its upstream port
                                                                           for more than 3 milliseconds. It is reset to 0 on any activity.
                                                                           When the device is suspended (Suspend bit DSUS = 1) and the
                                                                           software writes a 0 to it, the device will generate a remote
                                                                           wake-up. This will only happen when the device is connected
                                                                           (Connect bit = 1). When the device is not connected or not
                                                                           suspended, a writing a 0 has no effect. Writing a 1 never has an
                                                                           effect */
#define USB_DEVCMDSTAT_LPM_SUS                  (1 << 19)               /* Device status - LPM Suspend.
                                                                           This bit represents the current LPM suspend state. It is set to 1
                                                                           by HW when the device has acknowledged the LPM request
                                                                           from the USB host and the Token Retry Time of 10 s has
                                                                           elapsed. When the device is in the LPM suspended state (LPM
                                                                           suspend bit = 1) and the software writes a zero to this bit, the
                                                                           device will generate a remote walk-up. Software can only write
                                                                           a zero to this bit when the LPM_REWP bit is set to 1. HW resets
                                                                           this bit when it receives a host initiated resume. HW only
                                                                           updates the LPM_SUS bit when the LPM_SUPP bit is equal to
                                                                           one */
#define USB_DEVCMDSTAT_LPM_REWP                 (1 << 20)               /* LPM Remote Wake-up Enabled by USB host.
                                                                           HW sets this bit to one when the bRemoteWake bit in the LPM
                                                                           extended token is set to 1. HW will reset this bit to 0 when it
                                                                           receives the host initiated LPM resume, when a remote
                                                                           wake-up is sent by the device or when a USB bus reset is
                                                                           received. Software can use this bit to check if the remote
                                                                           wake-up feature is enabled by the host for the LPM transaction */
#define USB_DEVCMDSTAT_DCON_C                   (1 << 24)               /* Device status - connect change.
                                                                           The Connect Change bit is set when the device’s pull-up
                                                                           resistor is disconnected because VBus disappeared. The bit is
                                                                           reset by writing a one to it */
#define USB_DEVCMDSTAT_DSUS_C                   (1 << 25)               /* Device status - suspend change.
                                                                           The suspend change bit is set to 1 when the suspend bit
                                                                           toggles. The suspend bit can toggle because:
                                                                           - The device goes in the suspended state
                                                                           - The device is disconnected
                                                                           - The device receives resume signaling on its upstream port.
                                                                           The bit is reset by writing a one to it */
#define USB_DEVCMDSTAT_DRES_C                   (1 << 25)               /* Device status - reset change.
                                                                           This bit is set when the device received a bus reset. On a bus
                                                                           reset the device will automatically go to the default state
                                                                           (unconfigured and responding to address 0). The bit is reset by
                                                                           writing a one to it */
#define USB_DEVCMDSTAT_VBUSDEBOUNCED            (1 << 28)               /* This bit indicates if Vbus is detected or not. The bit raises
                                                                           immediately when Vbus becomes high. It drops to zero if Vbus
                                                                           is low for at least 3 ms. If this bit is high and the DCon bit is set,
                                                                           the HW will enable the pull-up resistor to signal a connect */

/************  Bit definition for INFO register  ******************************/
#define USB_INFO_FRAME_NR_POS                   0                       /* Frame number. This contains the frame number of the last
                                                                           successfully received SOF. In case no SOF was received by the
                                                                           device at the beginning of a frame, the frame number returned is
                                                                           that of the last successfully received SOF. In case the SOF frame
                                                                           number contained a CRC error, the frame number returned will be
                                                                           the corrupted frame number as received by the device */
#define USB_INFO_FRAME_NR_MASK                  (0x7ff << 0)
#define USB_INFO_ERR_CODE_MASK                  (0xf << 11)             /* The error code which last occurred */
#define USB_INFO_ERR_CODE_POS                   11

#define USB_INFO_ERR_CODE_NO_ERROR              (0x0 << 11)             /* No error */
#define USB_INFO_ERR_CODE_PID_ENCODING          (0x1 << 11)             /* PID encoding error */
#define USB_INFO_ERR_CODE_PID_UNKNOWN           (0x2 << 11)             /* PID unknown */
#define USB_INFO_ERR_CODE_PACKET_UNEXPECTED     (0x3 << 11)             /* Packet unexpected */
#define USB_INFO_ERR_CODE_TOKEN_CRC             (0x4 << 11)             /* Token CRC error */
#define USB_INFO_ERR_CODE_DATA_CRC              (0x5 << 11)             /* Data CRC error */
#define USB_INFO_ERR_CODE_TIME_OUT              (0x6 << 11)             /* Time out */
#define USB_INFO_ERR_CODE_BABBLE                (0x7 << 11)             /* Babble */
#define USB_INFO_ERR_CODE_TRUNCATED_EOP         (0x8 << 11)             /* Truncated EOP */
#define USB_INFO_ERR_CODE_IONAK                 (0x9 << 11)             /* Sent/Received NAK */
#define USB_INFO_ERR_CODE_STALL                 (0xa << 11)             /* Sent Stall */
#define USB_INFO_ERR_CODE_OVERRUN               (0xb << 11)             /* Overrun */
#define USB_INFO_ERR_CODE_EMPTY_PACKET          (0xc << 11)             /* Sent empty packet */
#define USB_INFO_ERR_CODE_BITSTUFF              (0xd << 11)             /* Bitstuff error */
#define USB_INFO_ERR_CODE_SYNC                  (0xe << 11)             /* Sync error */
#define USB_INFO_ERR_CODE_WRONG_DATA_TOGGLE     (0xf << 11)             /* Wrong data toggle */

/************  Bit definition for LPM register  *******************************/
#define USB_LPM_HIRD_HW_POS                     0                       /* Host Initiated Resume Duration - HW. This is
                                                                           the HIRD value from the last received LPM token */
#define USB_LPM_HIRD_HW_MASK                    (0xf << 0)
#define USB_LPM_HIRD_SW_POS                     4                       /* Host Initiated Resume Duration - SW. This is
                                                                           the time duration required by the USB device
                                                                           system to come out of LPM initiated suspend
                                                                           after receiving the host initiated LPM resume */
#define USB_LPM_HIRD_SW_MASK                    (0xf << 4)
#define USB_LPM_DATA_PENDING                    (1 << 8)                /* As long as this bit is set to one and LPM
                                                                           supported bit is set to one, HW will return a
                                                                           NYET handshake on every LPM token it
                                                                           receives.
                                                                           If LPM supported bit is set to one and this bit is
                                                                           zero, HW will return an ACK handshake on
                                                                           every LPM token it receives.
                                                                           If SW has still data pending and LPM is
                                                                           supported, it must set this bit to 1 */
/************  Bit definition for INTSTAT register  ***************************/
#define USB_INTSTAT_EP0OUT                      (1 << 0)                /* Interrupt status register bit for the Control EP0 OUT direction.
                                                                           This bit will be set if NBytes transitions to zero or the skip bit is set by software
                                                                           or a SETUP packet is successfully received for the control EP0.
                                                                           If the IntOnNAK_CO is set, this bit will also be set when a NAK is transmitted
                                                                           for the Control EP0 OUT direction.
                                                                           Software can clear this bit by writing a one to it */
#define USB_INTSTAT_EP0IN                       (1 << 1)                /* Interrupt status register bit for the Control EP0 IN direction.
                                                                           This bit will be set if NBytes transitions to zero or the skip bit is set by
                                                                           software.
                                                                           If the IntOnNAK_CI is set, this bit will also be set when a NAK is transmitted
                                                                           for the Control EP0 IN direction.
                                                                           Software can clear this bit by writing a one to it */
#define USB_INTSTAT_EP1OUT                      (1 << 2)                /* Interrupt status register bit for the EP1 OUT direction.
                                                                           This bit will be set if the corresponding Active bit is cleared by HW. This is
                                                                           done in case the programmed NBytes transitions to zero or the skip bit is set
                                                                           by software.
                                                                           If the IntOnNAK_AO is set, this bit will also be set when a NAK is transmitted
                                                                           for the EP1 OUT direction.
                                                                           Software can clear this bit by writing a one to it */
#define USB_INTSTAT_EP1IN                       (1 << 3)                /* Interrupt status register bit for the EP1 IN direction.
                                                                           This bit will be set if the corresponding Active bit is cleared by HW. This is
                                                                           done in case the programmed NBytes transitions to zero or the skip bit is set
                                                                           by software.
                                                                           If the IntOnNAK_AI is set, this bit will also be set when a NAK is transmitted
                                                                           for the EP1 IN direction.
                                                                           Software can clear this bit by writing a one to it */
#define USB_INTSTAT_EP2OUT                      (1 << 4)                /* Interrupt status register bit for the EP2 OUT direction.
                                                                           This bit will be set if the corresponding Active bit is cleared by HW. This is
                                                                           done in case the programmed NBytes transitions to zero or the skip bit is set
                                                                           by software.
                                                                           If the IntOnNAK_AO is set, this bit will also be set when a NAK is transmitted
                                                                           for the EP2 OUT direction.
                                                                           Software can clear this bit by writing a one to it */
#define USB_INTSTAT_EP2IN                       (1 << 5)                /* Interrupt status register bit for the EP2 IN direction.
                                                                           This bit will be set if the corresponding Active bit is cleared by HW. This is
                                                                           done in case the programmed NBytes transitions to zero or the skip bit is set
                                                                           by software.
                                                                           If the IntOnNAK_AI is set, this bit will also be set when a NAK is transmitted
                                                                           for the EP2 IN direction.
                                                                           Software can clear this bit by writing a one to it */
#define USB_INTSTAT_EP3OUT                      (1 << 6)                /* Interrupt status register bit for the EP3 OUT direction.
                                                                           This bit will be set if the corresponding Active bit is cleared by HW. This is
                                                                           done in case the programmed NBytes transitions to zero or the skip bit is set
                                                                           by software.
                                                                           If the IntOnNAK_AO is set, this bit will also be set when a NAK is transmitted
                                                                           for the EP3 OUT direction.
                                                                           Software can clear this bit by writing a one to it */
#define USB_INTSTAT_EP3IN                       (1 << 7)                /* Interrupt status register bit for the EP3 IN direction.
                                                                           This bit will be set if the corresponding Active bit is cleared by HW. This is
                                                                           done in case the programmed NBytes transitions to zero or the skip bit is set
                                                                           by software.
                                                                           If the IntOnNAK_AI is set, this bit will also be set when a NAK is transmitted
                                                                           for the EP3 IN direction.
                                                                           Software can clear this bit by writing a one to it */
#define USB_INTSTAT_EP4OUT                      (1 << 8)                /* Interrupt status register bit for the EP4 OUT direction.
                                                                           This bit will be set if the corresponding Active bit is cleared by HW. This is
                                                                           done in case the programmed NBytes transitions to zero or the skip bit is set
                                                                           by software.
                                                                           If the IntOnNAK_AO is set, this bit will also be set when a NAK is transmitted
                                                                           for the EP4 OUT direction.
                                                                           Software can clear this bit by writing a one to it */
#define USB_INTSTAT_EP4IN                       (1 << 9)                /* Interrupt status register bit for the EP4 IN direction.
                                                                           This bit will be set if the corresponding Active bit is cleared by HW. This is
                                                                           done in case the programmed NBytes transitions to zero or the skip bit is set
                                                                           by software.
                                                                           If the IntOnNAK_AI is set, this bit will also be set when a NAK is transmitted
                                                                           for the EP4 IN direction.
                                                                           Software can clear this bit by writing a one to it */
#define USB_INTSTAT_FRAME_INT                   (1 << 30)               /* Frame interrupt.
                                                                           This bit is set to one every millisecond when the VbusDebounced bit and the
                                                                           DCON bit are set. This bit can be used by software when handling
                                                                           isochronous endpoints.
                                                                           Software can clear this bit by writing a one to it */
#define USB_INTSTAT_DEV_INT                     (1 << 31)               /* Device status interrupt. This bit is set by HW when one of the bits in the
                                                                           Device Status Change register are set. Software can clear this bit by writing a
                                                                           one to it */

/************  Bit definition for EPLISTST ************************************/
#define USB_EP_LISTST_OFFSET_POS                0                       /* Bits 21 to 6 of the buffer start address.
                                                                           The address offset is updated by hardware after each successful
                                                                           reception/transmission of a packet. Hardware increments the original value
                                                                           with the integer value when the packet size is divided by 64 */
#define USB_EP_LISTST_OFFSET_MASK               (0xffff << 0)
#define USB_EP_LISTST_OFFSET_SET(addr)          ((((unsigned int)(addr)) >> 6) & 0xffff)
#define USB_EP_LISTST_NBYTES_POS                16                      /* For OUT endpoints this is the number of bytes that can be received in this
                                                                           buffer.
                                                                           For IN endpoints this is the number of bytes that must be transmitted.
                                                                           HW decrements this value with the packet size every time when a packet is
                                                                           successfully transferred.
                                                                           Note: If a short packet is received on an OUT endpoint, the active bit will be
                                                                           cleared and the NBytes value indicates the remaining buffer space that is
                                                                           not used. Software calculates the received number of bytes by subtracting
                                                                           the remaining NBytes from the programmed value */
#define USB_EP_LISTST_NBYTES_MASK              (0x3ff << 16)
#define USB_EP_LISTST_NBYTES_SET(val)          (((val) & 0x3ff) << 16)
#define USB_EP_LISTST_T                        (1 << 26)                /* Endpoint Type
                                                                           0: Generic endpoint. The endpoint is configured as a bulk or interrupt
                                                                           endpoint
                                                                           1: Isochronous endpoint */
#define USB_EP_LISTST_RF                       (1 << 27)                /* When the endpoint is used as an interrupt endpoint, it can be set to the
                                                                           following values.
                                                                           0: Interrupt endpoint in ‘toggle mode’
                                                                           1: Interrupt endpoint in ‘rate feedback mode’. This means that the data
                                                                           toggle is fixed to zero for all data packets.
                                                                           When the interrupt endpoint is in ‘rate feedback mode’, the TR bit must
                                                                           always be set to zero */
#define USB_EP_LISTST_TV                       (1 << 27)                /* When the endpoint is used as an interrupt endpoint, it can be set to the
                                                                           For bulk endpoints and isochronous endpoints this bit is reserved and must
                                                                           be set to zero.
                                                                           For the control endpoint zero this bit is used as the toggle value. When the
                                                                           toggle reset bit is set, the data toggle is updated with the value
                                                                           programmed in this bit */
#define USB_EP_LISTST_TR                       (1 << 28)                /* Toggle Reset
                                                                           When software sets this bit to one, the HW will set the toggle value equal to
                                                                           the value indicated in the “toggle value” (TV) bit.
                                                                           For the control endpoint zero, this is not needed to be used because the
                                                                           hardware resets the endpoint toggle to one for both directions when a
                                                                           setup token is received.
                                                                           For the other endpoints, the toggle can only be reset to zero when the
                                                                           endpoint is reset */
#define USB_EP_LISTST_S                        (1 << 29)                /* Stall
                                                                           0: The selected endpoint is not stalled
                                                                           1: The selected endpoint is stalled
                                                                           The Active bit has always higher priority than the Stall bit. This means that
                                                                           a Stall handshake is only sent when the active bit is zero and the stall bit is
                                                                           one.
                                                                           Software can only modify this bit when the active bit is zero */

#define USB_EP_LISTST_D                        (1 << 30)                /* Disabled
                                                                           0: The selected endpoint is enabled.
                                                                           1: The selected endpoint is disabled.
                                                                           If a USB token is received for an endpoint that has the disabled bit set,
                                                                           hardware will ignore the token and not return any data or handshake.
                                                                           When a bus reset is received, software must set the disable bit of all
                                                                           endpoints to 1.
                                                                           Software can only modify this bit when the active bit is zero */
#define USB_EP_LISTST_A                        (1 << 31)                /* Active
                                                                           The buffer is enabled. HW can use the buffer to store received OUT data or
                                                                           to transmit data on the IN endpoint.
                                                                           Software can only set this bit to ‘1’. As long as this bit is set to one,
                                                                           software is not allowed to update any of the values in this 32-bit word. In
                                                                           case software wants to deactivate the buffer, it must write a one to the
                                                                           corresponding “skip” bit in the USB Endpoint skip register. Hardware can
                                                                           only write this bit to zero. It will do this when it receives a short packet or
                                                                           when the NBytes field transitions to zero or when software has written a
                                                                           one to the “skip” bit */


#endif

#endif // LPC11UXX_BITS_H
