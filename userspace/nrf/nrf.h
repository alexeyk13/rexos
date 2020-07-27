/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2019, RExOS team
    All rights reserved.

    author: RJ (jam_roma@yahoo.com)
*/


#ifndef NRF_H
#define NRF_H

//---------------------------------------------------------------------------- NRF 51 ----------------------------------------------------------------------------------------------------------
#if defined(NRF51822QFAA) || defined(NRF51822CEAA)
#define NRF5122xxAA
#endif // NRF51822QFAA || NRF51822CEAA

#if defined(NRF51822QFAB) || defined(NRF51822CDAB)
#define NRF5122xxAB
#endif // NRF51822QFAB || NRF51822CDAB

#if defined(NRF51822QFAC) || defined(NRF51822CFAC)
#define NRF5122xxAC
#endif // NRF51822QFAC || NRF51822CFAC

#if defined(NRF5122xxAA) || defined(NRF5122xxAB) ||  defined(NRF5122xxAC)
#define NRF51

#if defined (NRF5122xxAB)
#define FLASH_SIZE              0x20000
#define FLASH_PAGE_SIZE         0x400
#define FLASH_TOTAL_PAGE_CNT    0x80
#else
#define FLASH_SIZE              0x40000
#define FLASH_PAGE_SIZE         0x400
#define FLASH_TOTAL_PAGE_CNT    0x100
#endif // NRF5122xxAB

#if defined (NRF5122xxAC)
#define SRAM_SIZE               0x8000
#else

#if (NRF_SRAM_POWER_CONFIG)
#define SRAM_SIZE               0x2000
#define SRAM_PART_COUNT         2
#define SRAM_TOTAL_SIZE         0x4000
#else
#define SRAM_SIZE               0x4000
#endif // NRF_SRAM_POWER_CONFIG
#endif // NRF5122xxAC

#define UARTS_COUNT             1
#define SPI_COUNT               2
#define RTC_COUNT               2
#define TIMERS_COUNT            3
#define SOFTWARE_IRQ_COUNT      6
#define GPIO_COUNT              32

#define IRQ_VECTORS_COUNT       32
#endif // NRF5122xxAA || NRF5122xxAB || NRF5122xxAC

//---------------------------------------------------------------------------- NRF 52 ----------------------------------------------------------------------------------------------------------
#if defined(NRF52832QFAA)
#define NRF52832xxAA
#endif // NRF52832QFAA

#if defined(NRF52832xxAA)
#define NRF52

//512K
#define FLASH_SIZE              0x80000
//64K
#define SRAM_SIZE               0x10000

#define UARTS_COUNT             1
#define SPI_COUNT               3
#define RTC_COUNT               3
#define TIMERS_COUNT            5
#define SOFTWARE_IRQ_COUNT      6
#define GPIO_COUNT              32

#define IRQ_VECTORS_COUNT       37
#endif // NRF52832xxAA

#if defined(NRF51)
#ifndef CORTEX_M0
#define CORTEX_M0
#endif // CORTEX_M0
#endif // NRF51

#if defined(NRF52)
#ifndef CORTEX_M4
#define CORTEX_M4
#endif // CORTEX_M4
#endif // NRF52

#if defined(NRF51) || defined(NRF52)

#define EXODRIVERS
#ifndef FLASH_BASE
#define FLASH_BASE                0x00000000
#endif

#if !defined(LDS) && !defined(__ASSEMBLER__)

#include "nrf_config.h"

#if defined(NRF51)

#include "nrf51.h"
#include "nrf51_bitfields.h"
#include "nrf51_deprecated.h"
#elif defined(NRF52)

#include "nrf51_to_nrf52.h"
#include "nrf52.h"
#include "nrf52.h"
#include "nrf52_bitfields.h"
#endif /* NRF51 */

#endif //!defined(LDS) && !defined(__ASSEMBLER__)

#endif // NRF51 || NRF52

#endif /* NRF_H */
