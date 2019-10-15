/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: RJ (jam_roma@yahoo.com)
*/

#ifndef _NRF51_NRF_DRIVER_H_
#define _NRF51_NRF_DRIVER_H_

#include "sys_config.h"
#include "../object.h"
#include "../power.h"

//---------------------------------- ADC ---------------------------------------
typedef enum {
    NRF_ADC_AIN0 = 0,
    NRF_ADC_AIN1,
    NRF_ADC_AIN2,
    NRF_ADC_AIN3,
    NRF_ADC_AIN4,
    NRF_ADC_AIN5,
    NRF_ADC_AIN6,
    NRF_ADC_AIN7
} NRF_ADC_AIN;

typedef enum {
    NRF_ADC_INPUT_P0_26 = 0,
    NRF_ADC_INPUT_P0_27,
    NRF_ADC_INPUT_P0_01,
    NRF_ADC_INPUT_P0_02,
    NRF_ADC_INPUT_P0_03,
    NRF_ADC_INPUT_P0_04,
    NRF_ADC_INPUT_P0_05,
    NRF_ADC_INPUT_P0_06
} NRF_ADC_AINPUT;
//--------------------------------- POWER --------------------------------------
typedef enum {
    //if enabled
   NRF_POWER_GET_RESET_REASON = POWER_MAX
} NRF_POWER_IPCS;

typedef enum {
    RESET_REASON_UNKNOWN = 0,
    RESET_REASON_PIN_RST,
    RESET_REASON_WATCHDOG,
    RESET_REASON_SYSTEM_RESET_REQ,
    RESET_REASON_LOCKUP,
    RESET_REASON_WAKEUP,
    RESET_REASON_LPCOMP,
    RESET_REASON_DEBUG_INTERFACE_MODE,
} RESET_REASON;
//---------------------------------- GPIO --------------------------------------
typedef enum {
    P0 = 0, P1, P2, P3, P4, P5, P6, P7, P8, P9, P10, P11, P12, P13, P14, P15,
    P16, P17, P18, P19, P20, P21, P22, P23, P24, P25, P26, P27, P28, P29, P30,
    //only for service calls
    PIN_DEFAULT,
    PIN_UNUSED
} PIN;

typedef enum {
    PIN_MODE_INPUT,       ///< Input
    PIN_MODE_OUTPUT       ///< Output
} PIN_MODE;

typedef enum {
    PIN_PULL_NOPULL    = GPIO_PIN_CNF_PULL_Disabled,     ///<  Pin pullup resistor disabled
    PIN_PULL_DOWN      = GPIO_PIN_CNF_PULL_Pulldown,   ///<  Pin pulldown resistor enabled
    PIN_PULL_UP        = GPIO_PIN_CNF_PULL_Pullup,       ///<  Pin pullup resistor enabled
} PIN_PULL;

typedef enum {
    PIN_SENSE_NO        = GPIO_PIN_CNF_SENSE_Disabled,   ///<  Pin sense level disabled.
    PIN_SENSE_LOW       = GPIO_PIN_CNF_SENSE_Low,      ///<  Pin sense low level.
    PIN_SENSE_HIGH      = GPIO_PIN_CNF_SENSE_High,    ///<  Pin sense high level.
} PIN_SENSE;
//---------------------------------- TIMER -------------------------------------
#define TIMER_CHANNEL_INVALID                        0xff

#define TIMER_MODE_CHANNEL_POS                       16
#define TIMER_MODE_CHANNEL_MASK                      (0xf << TIMER_MODE_CHANNEL_POS)

#if defined(NRF51) || defined(NRF52)
typedef enum {
    TIMER_0 = 0,
    TIMER_1,
    TIMER_2,
#if defined(NRF52)
    TIMER_3,
    TIMER_4,
#endif // NRF52
    TIMER_MAX
} TIMER_NUM;

typedef enum {
    TIMER_CC0 = 0,
    TIMER_CC1,
    TIMER_CC2,
    TIMER_CC3,
#if defined(NRF52)
    TIMER_CC4, /* TIMER3 TIMER4 only */
    TIMER_CC5, /* TIMER3 TIMER4 only */
#endif // NRF52
    TIMER_CC_MAX
} TIMER_CC;
#endif // NRF51 || NRF52

//------------------------------------ UART ------------------------------------
#if defined(NRF51) || defined(NRF52)
typedef enum {
    UART_0 = 0,
} UART_PORT;
#endif // NRF51

//------------------------------------ RTC -------------------------------------
#if defined(NRF51) || defined(NRF52)
typedef enum {
    RTC_0 = 0,
    RTC_1,
#if defined(NRF52)
    RTC_2,
#endif // NRF52
    RTC_MAX
} RTC_NUM;

typedef enum {
    RTC_CC0 = 0,
    RTC_CC1,
    RTC_CC2,
    RTC_CC3,
} RTC_CC;

#endif // NRF51 || NRF52

// ----------------------------------- RADIO -----------------------------------
#if defined(NRF51)
#define NRF_MAX_PACKET_LENGTH                   254
#endif // NRF51
#if defined(NRF52)
#define NRF_MAX_PACKET_LENGTH                   258
#endif // NRF52

typedef enum {
    RADIO_MODE_RF_1Mbit = 0,
    RADIO_MODE_RF_2Mbit,
    RADIO_MODE_RF_250Kbit,
    RADIO_MODE_BLE_1Mbit,
#if defined(NRF52)
    RADIO_MODE_BLE_2Mbit
#endif //
} RADIO_MODE;

#endif /* _NRF51_NRF_DRIVER_H_ */
