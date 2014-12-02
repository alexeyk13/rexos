/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef LPC_H
#define LPC_H

//---------------------------------------------------------------------------- LPC11Uxx ----------------------------------------------------------------------------------------------------------
#if defined(LPC11U12FBD48_201) || defined(LPC11U12FHN33_201) || defined(LPC11U22FBD48_201)
#define LPC11U12
#endif

#if defined(LPC11U13FBD48_201)
#define LPC11U13
#endif

#if defined(LPC11U14FBD48_201) || defined(LPC11U14FET48_201) || defined(LPC11U14FHI33_201) || defined(LPC11U14FHN33_201)
#define LPC11U14
#endif

#if defined(LPC11U22FBD48_301)
#define LPC11U22
#endif

#if defined(LPC11U23FBD48_301)
#define LPC11U23
#endif

#if defined(LPC11U24FET48_301) || defined(LPC11U24FHI33_301) || defined(LPC11U24FBD48_301) || defined(LPC11U24FBD48_401)|| defined(LPC11U24FBD64_401) || defined(LPC11U24FHN33_401)
#define LPC11U24
#endif

#if defined(LPC11U34FBD48_311) || defined(LPC11U34FHN33_311) || defined(LPC11U34FBD48_421) || defined(LPC11U34FHN33_421)
#define LPC11U34
#endif

#if defined(LPC11U35FBD48_401) || defined(LPC11U35FBD64_401) || defined(LPC11U35FHN33_401) || defined(LPC11U35FET48_501) || defined(LPC11U35FHI33_501)
#define LPC11U35
#endif

#if defined(LPC11U36FBD48_401) || defined(LPC11U36FBD64_401)
#define LPC11U36
#endif

#if defined(LPC11U37FBD48_401) || defined(LPC11U37FBD64_401) || defined(LPC11U37HFBD64_501)
#define LPC11U37
#endif

#if defined(LPC11U12) || defined(LPC11U13) || defined(LPC11U14)
#define LPC_PARTNO_201
#endif

#if defined(LPC11U22) || defined(LPC11U23) || defined(LPC11U24FET48_301) || defined(LPC11U24FHI33_301) || defined(LPC11U24FBD48_301)
#define LPC_PARTNO_301
#endif

#if defined(LPC11U34FBD48_311) || defined(LPC11U34FHN33_311)
#define LPC_PARTNO_311
#endif

#if defined(LPC11U24FBD48_401) || defined(LPC11U24FBD64_401) || defined(LPC11U24FHN33_401) || defined(LPC11U35FBD48_401) || defined(LPC11U35FBD64_401) || defined(LPC11U35FHN33_401) || \
    defined(LPC11U36) || defined(LPC11U37FBD48_401) || defined(LPC11U37FBD64_401)
#define LPC_PARTNO_401
#endif

#if defined(LPC11U34FBD48_421) || defined(LPC11U34FHN33_421)
#define LPC_PARTNO_421
#endif

#if defined(LPC11U35FET48_501) || defined(LPC11U35FHI33_501) || defined(LPC11U37HFBD64_501)
#define LPC_PARTNO_501
#endif

#if defined(LPC11U66JBD48)
#define LPC11U66
#endif

#if defined(LPC11U67JBD100) || defined(LPC11U67JBD48) || defined(LPC11U67JBD64)
#define LPC11U67
#endif

#if defined(LPC11U68JBD100) || defined(LPC11U68JBD48) || defined(LPC11U68JBD64)
#define LPC11U68
#endif

#if defined(LPC11U66) || defined(LPC11U67) || defined(LPC11U68)
#define LPC11U6x
#define I2C_COUNT                       2
#endif

#if defined(LPC11U12) || defined(LPC11U22)
//16k
#define FLASH_SIZE                      0x4000
#endif

#if defined(LPC11U13) || defined(LPC11U23)
//24k
#define FLASH_SIZE                      0x6000
#endif

#if defined(LPC11U14) || defined(LPC11U24)
//32k
#define FLASH_SIZE                      0x8000
#endif

#if defined(LPC11U34) && defined(LPC_PARTNO_311)
//40k
#define FLASH_SIZE                      0xA000
#endif

#if defined(LPC11U34) && defined(LPC_PARTNO_421)
//48k
#define FLASH_SIZE                      0xC000
#endif

#if defined(LPC11U35) || defined(LPC11U66)
//64k
#define FLASH_SIZE                      0x10000
#endif

#if defined(LPC11U36)
//96k
#define FLASH_SIZE                      0x18000
#endif

#if defined(LPC11U37) || defined(LPC11U67)
//128k
#define FLASH_SIZE                      0x20000
#endif

#if defined(LPC11U68)
//256k
#define FLASH_SIZE                      0x40000
#endif

#if defined(LPC_PARTNO_201)
//4k
#define SRAM_SIZE                       0x1000
#endif

#if defined(LPC_PARTNO_301)
//6k
#define SRAM_SIZE                       0x1800
#endif

#if defined(LPC_PARTNO_311) || defined(LPC_PARTNO_401) || defined(LPC_PARTNO_421) || defined(LPC_PARTNO_501) || defined(LPC11U66)
//8k
#define SRAM_SIZE                       0x2000
#endif

#if defined(LPC11U67)
//16k
#define SRAM_SIZE                       0x4000
#endif

#if defined(LPC11U68)
//32k
#define SRAM_SIZE                       0x8000
#endif

#if defined(LPC11U12) || defined(LPC11U13) || defined(LPC11U14) || defined(LPC11U22) || defined(LPC11U23) || defined(LPC11U24) || defined(LPC11U34) || \
    defined(LPC11U35) || defined(LPC11U36) || defined(LPC11U37)
#define UARTS_COUNT                   1
#define I2C_COUNT                     1
#endif

#if defined(LPC11U66) || defined(LPC11U67JBD48) || defined(LPC11U67JBD64) || defined(LPC11U68JBD48) || defined(LPC11U68JBD64)
#define UARTS_COUNT                   3
#endif

#if defined(LPC11U67JBD100) || defined(LPC11U68JBD100)
#define UARTS_COUNT                   5
#endif

#if defined(LPC11U12) || defined(LPC11U13) || defined(LPC11U14) || defined(LPC11U22) || defined(LPC11U23) || defined(LPC11U24) || defined(LPC11U34) || \
    defined(LPC11U35) || defined(LPC11U36) || defined(LPC11U37) || defined(LPC11U66) || defined(LPC11U67) || defined(LPC11U68)
#define LPC11Uxx
#endif

#if defined(LPC11Uxx)
#define IRQ_VECTORS_COUNT             31
#define GPIO_COUNT                    2
#endif

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#if defined(LPC11Uxx)
#define LPC
#ifndef CORTEX_M0
#define CORTEX_M0
#endif //CORTEX_M0
#endif //LPC11Uxx

#if defined(CORTEX_M3) || defined(CORTEX_M0)
#ifndef CORTEX_M
#define CORTEX_M
#endif //CORTEX_M
#endif //defined(CORTEX_M3) || defined(CORTEX_M0)

#if defined(LPC)

#ifndef FLASH_BASE
#define FLASH_BASE                0x00000000
#endif //FLASH_BASE

#ifndef SRAM_BASE
#define SRAM_BASE                 0x10000000
#endif //SRAM_BASE

#endif //LPC

#if !defined(LDS) && !defined(__ASSEMBLER__)

#if defined(LPC)

#if defined(LPC11Uxx)
#include "lpc11uxx_bits.h"
#include "lpc_config.h"
#if defined(LPC11U6x)
#include "LPC11U6x.h"
#else
#include "LPC11Uxx.h"
#endif //defined(LPC11U6x)
#endif //defined(LPC11Uxx)
#endif //LPC

#endif // !defined(LDS) && !defined(__ASSEMBLER__)

#endif //LPC_H
