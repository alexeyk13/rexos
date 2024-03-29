/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2018, Alexey Kramarenko
    All rights reserved.

    author: Alexey E. Kramarenko (alexeyk13@yandex.ru)
*/

#ifndef STM32_DRIVER_H
#define STM32_DRIVER_H

#include "stm32.h"
#include "stm32_config.h"
#include "../adc.h"
#include "../power.h"
#include "../uart.h"

//-------------------------------------------------- POWER ---------------------------------------------------------------------

#if defined(STM32F10X_CL)
#define PLL_MUL_6_5                             15
#define PLL2_MUL_20                             15
#endif

#define STANDBY_WKUP_PIN1                       (1 << 8)
#define STANDBY_WKUP_PIN2                       (1 << 9)

typedef enum {
    //if enabled
    STM32_POWER_GET_RESET_REASON = POWER_MAX,
#if defined(STM32H7)
    STM32_POWER_SET_MCO,
#endif
} STM32_POWER_IPCS;

typedef enum {
    STM32_CLOCK_APB1 = POWER_CLOCK_MAX,
    STM32_CLOCK_APB2,
    STM32_CLOCK_ADC,
#if defined(STM32H7)
    STM32_CLOCK_PLL1_Q,
    STM32_CLOCK_PLL2_P,
    STM32_CLOCK_PLL2_Q,
    STM32_CLOCK_PLL2_R,
    STM32_CLOCK_PLL3_P,
    STM32_CLOCK_PLL3_Q,
    STM32_CLOCK_PLL3_R,
#endif // STM32H7
} STM32_POWER_CLOCKS;

typedef enum {
    DMA_1 = 0,
    DMA_2
} STM32_DMA;

typedef enum {
    DMA_CHANNEL_1 = 0,
    DMA_CHANNEL_2,
    DMA_CHANNEL_3,
    DMA_CHANNEL_4,
    DMA_CHANNEL_5,
    DMA_CHANNEL_6,
    DMA_CHANNEL_7
} STM32_DMA_CHANNEL;

typedef enum {
    RESET_REASON_UNKNOWN = 0,
    RESET_REASON_LOW_POWER,
    RESET_REASON_WATCHDOG,
    RESET_REASON_SOFTWARE,
    RESET_REASON_POWERON,
    RESET_REASON_PIN_RST,
    RESET_REASON_WAKEUP,
    RESET_REASON_OPTION_BYTES
} RESET_REASON;

#if defined(STM32H7)
typedef enum {
    STM32_PLL_OUT_P,
    STM32_PLL_OUT_Q,
    STM32_PLL_OUT_R,
}STM32_PLL_OUT;

typedef enum {
    MCO1_CLOCK_SRC_HSI = 0,
    MCO1_CLOCK_SRC_LSE,
    MCO1_CLOCK_SRC_HSE,
    MCO1_CLOCK_SRC_PLL1_Q,
    MCO1_CLOCK_SRC_HSI48,

    MCO2_CLOCK_SRC_CORE = 0 | 0x80,
    MCO2_CLOCK_SRC_PLL2_P,
    MCO2_CLOCK_SRC_HSE,
    MCO2_CLOCK_SRC_PLL1_P,
    MCO2_CLOCK_SRC_CSI,
    MCO2_CLOCK_SRC_LSI,
}MCO_CLOCK_SRC;

typedef enum {
    MCO_CLOCK_DISABLE = 0,
    MCO_CLOCK_DIV_1,
    MCO_CLOCK_DIV_2,
    MCO_CLOCK_DIV_3,
    MCO_CLOCK_DIV_4,
    MCO_CLOCK_DIV_5,
    MCO_CLOCK_DIV_6,
    MCO_CLOCK_DIV_7,
    MCO_CLOCK_DIV_8,
    MCO_CLOCK_DIV_9,
    MCO_CLOCK_DIV_10,
}MCO_CLOCK_DIV;


#define USB_CLOCK_SRC_PLL1_Q                    (1 << RCC_D2CCIP2R_USBSEL_Pos)
#define USB_CLOCK_SRC_PLL3_Q                    (2 << RCC_D2CCIP2R_USBSEL_Pos)
#define USB_CLOCK_SRC_HSI48                     (3 << RCC_D2CCIP2R_USBSEL_Pos)

#define SDMMC_CLOCK_SRC_PLL1_Q                  (0 << RCC_D1CCIPR_SDMMCSEL_Pos)
#define SDMMC_CLOCK_SRC_PLL2_R                  (1 << RCC_D1CCIPR_SDMMCSEL_Pos)

#define RNG_CLOCK_SRC_HSI48                     (0 << RCC_D2CCIP2R_RNGSEL_Pos)
#define RNG_CLOCK_SRC_PLL1_Q                    (1 << RCC_D2CCIP2R_RNGSEL_Pos)
#define RNG_CLOCK_SRC_LSE                       (2 << RCC_D2CCIP2R_RNGSEL_Pos)
#define RNG_CLOCK_SRC_LSI                       (3 << RCC_D2CCIP2R_RNGSEL_Pos)

#define ADC_CLOCK_SRC_PLL2_P                    (0 << RCC_D3CCIPR_ADCSEL_Pos)
#define ADC_CLOCK_SRC_PLL3_R                    (1 << RCC_D3CCIPR_ADCSEL_Pos)
#define ADC_CLOCK_SRC_PERIF                     (2 << RCC_D3CCIPR_ADCSEL_Pos)

#define UART_CLOCK_SRC_ARB                      0
#define UART_CLOCK_SRC_PLL2_Q                   1
#define UART_CLOCK_SRC_PLL3_Q                   2
#define UART_CLOCK_SRC_HSI                      3
#define UART_CLOCK_SRC_CSI                      4
#define UART_CLOCK_SRC_LSE                      5


#define SDMMC_RECEIVE_CLK_INT                   0
#define SDMMC_RECEIVE_CLK_CKIN                  1
#define SDMMC_RECEIVE_CLK_TUNED                 2


#define ADC_PRESCALER_1                         0
#define ADC_PRESCALER_2                         1
#define ADC_PRESCALER_4                         2
#define ADC_PRESCALER_8                         3
#define ADC_PRESCALER_10                        4
#define ADC_PRESCALER_12                        5
#define ADC_PRESCALER_16                        6
#define ADC_PRESCALER_32                        7
#define ADC_PRESCALER_64                        8
#define ADC_PRESCALER_128                       9
#define ADC_PRESCALER_256                       10

#endif // STM32H7


//------------------------------------------------- GPIO ---------------------------------------------------------------------

typedef enum {
    STM32_GPIO_ENABLE_EXTI = IPC_USER,
    STM32_GPIO_DISABLE_EXTI
} STM32_GPIO_IPCS;

typedef enum {
    A0 = 0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15,
    B0,     B1, B2, B3, B4, B5, B6, B7, B8, B9, B10, B11, B12, B13, B14, B15,
    C0,     C1, C2, C3, C4, C5, C6, C7, C8, C9, C10, C11, C12, C13, C14, C15,
    D0,     D1, D2, D3, D4, D5, D6, D7, D8, D9, D10, D11, D12, D13, D14, D15,
    E0,     E1, E2, E3, E4, E5, E6, E7, E8, E9, E10, E11, E12, E13, E14, E15,
    F0,     F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15,
    G0,     G1, G2, G3, G4, G5, G6, G7, G8, G9, G10, G11, G12, G13, G14, G15,
    H0,     H1, H2, H3, H4, H5, H6, H7, H8, H9, H10, H11, H12, H13, H14, H15,
    I0,     I1, I2, I3, I4, I5, I6, I7, I8, I9, I10, I11, I12, I13, I14, I15,
    J0,     J1, J2, J3, J4, J5, J6, J7, J8, J9, J10, J11, J12, J13, J14, J15,
    K0,     K1, K2, K3, K4, K5, K6, K7, K8, K9, K10, K11, K12, K13, K14, K15,
    //only for service calls
    PIN_DEFAULT,
    PIN_UNUSED
} PIN;

typedef enum {
    GPIO_PORT_A = 0,
    GPIO_PORT_B,
    GPIO_PORT_C,
    GPIO_PORT_D,
    GPIO_PORT_E,
    GPIO_PORT_F,
    GPIO_PORT_G,
    GPIO_PORT_H,
    GPIO_PORT_I,
    GPIO_PORT_J,
    GPIO_PORT_K
} GPIO_PORT_NUM;

#if defined(STM32F1)

typedef enum {
    STM32_GPIO_MODE_INPUT_ANALOG = 0x0,
    STM32_GPIO_MODE_INPUT_FLOAT = 0x4,
    STM32_GPIO_MODE_INPUT_PULL = 0x8,
    STM32_GPIO_MODE_OUTPUT_PUSH_PULL_10MHZ = 0x1,
    STM32_GPIO_MODE_OUTPUT_PUSH_PULL_2MHZ = 0x2,
    STM32_GPIO_MODE_OUTPUT_PUSH_PULL_50MHZ = 0x3,
    STM32_GPIO_MODE_OUTPUT_OPEN_DRAIN_10MHZ = 0x5,
    STM32_GPIO_MODE_OUTPUT_OPEN_DRAIN_2MHZ = 0x6,
    STM32_GPIO_MODE_OUTPUT_OPEN_DRAIN_50MHZ = 0x7,
    STM32_GPIO_MODE_OUTPUT_AF_PUSH_PULL_10MHZ = 0x9,
    STM32_GPIO_MODE_OUTPUT_AF_PUSH_PULL_2MHZ = 0xa,
    STM32_GPIO_MODE_OUTPUT_AF_PUSH_PULL_50MHZ = 0xb,
    STM32_GPIO_MODE_OUTPUT_AF_OPEN_DRAIN_10MHZ = 0xd,
    STM32_GPIO_MODE_OUTPUT_AF_OPEN_DRAIN_2MHZ = 0xe,
    STM32_GPIO_MODE_OUTPUT_AF_OPEN_DRAIN_50MHZ = 0xf
}STM32_GPIO_MODE;

#elif defined(STM32F0) || defined(STM32F2) || defined(STM32F4) || defined(STM32L0) \
        || defined(STM32L1) || defined(STM32H7)

#define STM32_GPIO_MODE_INPUT                   (0x0 << 0)
#define STM32_GPIO_MODE_OUTPUT                  (0x1 << 0)
#define STM32_GPIO_MODE_AF                      (0x2 << 0)
#define STM32_GPIO_MODE_ANALOG                  (0x3 << 0)

#define GPIO_OT_PUSH_PULL                       (0x0 << 2)
#define GPIO_OT_OPEN_DRAIN                      (0x1 << 2)

#if defined(STM32L0) || defined(STM32L1)
#define GPIO_SPEED_VERY_LOW                     (0x0 << 3)
#define GPIO_SPEED_LOW                          (0x1 << 3)
#define GPIO_SPEED_MEDIUM                       (0x2 << 3)
#define GPIO_SPEED_HIGH                         (0x3 << 3)
#else
#define GPIO_SPEED_LOW                          (0x0 << 3)
#define GPIO_SPEED_MEDIUM                       (0x1 << 3)
#if defined(STM32F2) || defined(STM32F4) || defined(STM32H7)
#define GPIO_SPEED_FAST                         (0x2 << 3)
#endif //defined(STM32F2) || defined(STM32F4) || defined(STM32L0)
#define GPIO_SPEED_HIGH                         (0x3 << 3)
#endif

#define GPIO_PUPD_NO_PULLUP                     (0x0 << 5)
#define GPIO_PUPD_PULLUP                        (0x1 << 5)
#define GPIO_PUPD_PULLDOWN                      (0x2 << 5)

typedef enum {
    AF0 = 0,
    AF1,
    AF2,
    AF3,
    AF4,
    AF5,
    AF6,
    AF7,
    AF8,
    AF9,
    AF10,
    AF11,
    AF12,
    AF13,
    AF14,
    AF15
} AF;

#endif

#define EXTI_FLAGS_RISING                            (1 << 0)
#define EXTI_FLAGS_FALLING                           (1 << 1)
#define EXTI_FLAGS_EDGE_MASK                         (3 << 0)

typedef GPIO_TypeDef* GPIO_TypeDef_P;
extern const GPIO_TypeDef_P GPIO[];

#define GPIO_MAKE_PIN(port, pin)                     ((port) * 16  + (pin))
#define GPIO_PORT(pin)                               (pin / 16)
#define GPIO_PIN(pin)                                (pin & 15)

//------------------------------------------------- timer ---------------------------------------------------------------------

#if defined(STM32L0)
typedef enum {
    TIM_2 = 0,
    TIM_6,
    TIM_21,
    TIM_22
}TIMER_NUM;
#elif defined(STM32L1)
typedef enum {
    TIM_2 = 0,
    TIM_3,
    TIM_4,
    TIM_5, /* !< This TIMER is available in Cat.3, Cat.4, Cat.5 and Cat.6 devices only */
    TIM_6,
    TIM_7,
    TIM_9,
    TIM_10,
    TIM_11,
}TIMER_NUM;
#elif defined(STM32F0)
typedef enum {
    TIM_1 = 0,
    TIM_2,
    TIM_3,
    TIM_6,
    TIM_7,
    TIM_14,
    TIM_15,
    TIM_16,
    TIM_17,
    TIM_MAX
}TIMER_NUM;
#else
typedef enum {
    TIM_1 = 0,
    TIM_2,
    TIM_3,
    TIM_4,
    TIM_5,
    TIM_6,
    TIM_7,
    TIM_8,
    TIM_9,
    TIM_10,
    TIM_11,
    TIM_12,
    TIM_13,
    TIM_14,
    TIM_15,
    TIM_16,
    TIM_17,
    TIM_18,
    TIM_19,
    TIM_20
}TIMER_NUM;
#endif

typedef enum {
    TIM_CHANNEL1 = 0,
    TIM_CHANNEL2,
    TIM_CHANNEL3,
    TIM_CHANNEL4,
} TIMER_CHANNEL;

#define STM32_TIMER_DMA_ENABLE                          (1 << 16)

//-------------------------------------------------- I2C ---------------------------------------------------------------------
typedef enum {
    I2C_1,
    I2C_2
} I2C_PORT;

//pins are required for STM32F1 configuration because of errata
#define STM32F1_I2C_SCL_Msk                             (0xff << 16)
#define STM32F1_I2C_SDA_Msk                             (0xff << 24)
#define STM32F1_I2C_SCL_Pos                             16
#define STM32F1_I2C_SDA_Pos                             24
#define STM32F1_I2C_ENCODE_PINS(scl, sda) (((scl) << STM32F1_I2C_SCL_Pos) | ((sda) << STM32F1_I2C_SDA_Pos))
#define STM32F1_I2C_SLAVE_ENCODE(sla, scl, sda)  (I2C_MODE_SLAVE | (sla) | (STM32F1_I2C_ENCODE_PINS(scl, sda)) )

// ------------------------------------------------ SPI -----------------------------------------------------------------------
typedef enum {
    SPI_1 = 0,
    SPI_2,
    SPI_MAX
} SPI_PORT;

//------------------------------------------------- UART ----------------------------------------------------------------------

typedef enum {
    UART_1 = 0,
    UART_2,
    UART_3,
    UART_4,
    UART_5,
    UART_6,
    UART_7,
    UART_8,
    UART_MAX
}UART_PORT;

typedef enum {
    IPC_STM32_UART_SETUP_ISO7816 = IPC_UART_MAX,
    IPC_STM32_UART_MAX
} STM32_UART_IPCS;

//-------------------------------------------------- ADC ----------------------------------------------------------------------
#if defined(STM32H7)
#define STM32_ADC_SMPR_1_5                           0
#define STM32_ADC_SMPR_2_5                           1
#define STM32_ADC_SMPR_8_5                           2
#define STM32_ADC_SMPR_16_5                          3
#define STM32_ADC_SMPR_32_5                          4
#define STM32_ADC_SMPR_64_5                          5
#define STM32_ADC_SMPR_387_5                         6
#define STM32_ADC_SMPR_810_5                         7

#else
#define STM32_ADC_SMPR_1_5                           0
#define STM32_ADC_SMPR_7_5                           1
#define STM32_ADC_SMPR_13_5                          2
#define STM32_ADC_SMPR_28_5                          3
#define STM32_ADC_SMPR_41_5                          4
#define STM32_ADC_SMPR_55_5                          5
#define STM32_ADC_SMPR_71_5                          6
#define STM32_ADC_SMPR_239_5                         7
#endif // STM32H7
typedef enum {
    STM32_ADC0,
    STM32_ADC1,
    STM32_ADC2,
    STM32_ADC3,
    STM32_ADC4,
    STM32_ADC5,
    STM32_ADC6,
    STM32_ADC7,
    STM32_ADC8,
    STM32_ADC9,
    STM32_ADC10,
    STM32_ADC11,
    STM32_ADC12,
    STM32_ADC13,
    STM32_ADC14,
    STM32_ADC15,
#if defined(STM32L0)
    STM32_ADC_VLCD,
#endif //STM32L0
#if  defined(STM32H7)
    STM32_ADC16,
    STM32_VBAT_4,
#endif //STM32L0
    STM32_ADC_TEMP, // STM32H7 - 18
    STM32_ADC_VREF,

    STM32_ADC_MAX
} STM32_ADC_CHANNEL;

#if defined(STM32H7)
#define TS_CAL1 ((uint16_t*)0x1FF1E820)
#define TS_CAL2 ((uint16_t*)0x1FF1E840)


__STATIC_INLINE int stm32_adc_temp()
{
    int prom =(110 - 30) * (adc_get(STM32_ADC_TEMP, STM32_ADC_SMPR_387_5) - *TS_CAL1);
    prom = prom / (*TS_CAL2 - *TS_CAL1);

    return prom + 30;

}

#else
__STATIC_INLINE int stm32_adc_temp(int vref, int res)
{
    return (V25_MV * 1000l - ADC2uV(adc_get(STM32_ADC_TEMP, STM32_ADC_SMPR_239_5), vref, res)) * 10l / AVG_SLOPE + 25l * 10l;
}
#endif // STM32H7

#endif // STM32_DRIVER_H
