/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef LPC_H
#define LPC_H

//------------------------------------------------------------------------ LPC11Uxx -------------------------------------------------------------------------------------
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

#if defined(LPC11U24FET48_301) || defined(LPC11U24FHI33_301) || defined(LPC11U24FBD48_301) || defined(LPC11U24FBD48_401) || \
    defined(LPC11U24FBD64_401) || defined(LPC11U24FHN33_401)
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

#if defined(LPC11U24FBD48_401) || defined(LPC11U24FBD64_401) || defined(LPC11U24FHN33_401) || defined(LPC11U35FBD48_401) || defined(LPC11U35FBD64_401) || \
    defined(LPC11U35FHN33_401) || defined(LPC11U36) || defined(LPC11U37FBD48_401) || defined(LPC11U37FBD64_401)
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
#define USB_COUNT                     1
#endif

//--------------------------------------------------------------------- LPC18xx -----------------------------------------------------------------------------------------
#if defined(LPC1810FET100) || defined(LPC1810FBD144)
#define LPC1810
#endif

#if defined(LPC1812JBD144) || defined(LPC1812JET100)
#define LPC1812
#endif

#if defined(LPC1813JBD144) || defined(LPC1813JET100)
#define LPC1813
#endif

#if defined(LPC1815JBD144) || defined(LPC1815JET100)
#define LPC1815
#endif

#if defined(LPC1817JBD144) || defined(LPC1817JET100)
#define LPC1817
#endif

#if defined(LPC1820FET100) || defined(LPC1820FBD144)
#define LPC1820
#endif

#if defined(LPC1822JBD144) || defined(LPC1822JET100)
#define LPC1822
#endif

#if defined(LPC1823JBD144) || defined(LPC1823JET100)
#define LPC1823
#endif

#if defined(LPC1825JBD144) || defined(LPC1825JET100)
#define LPC1825
#endif

#if defined(LPC1827JBD144) || defined(LPC1827JET100)
#define LPC1827
#endif

#if defined(LPC1830FET256) || defined(LPC1830FET180) || defined(LPC1830FET100) || defined(LPC1830FBD144)
#define LPC1830
#endif

#if defined(LPC1833FET256) || defined(LPC1833JET256) || defined(LPC1833JBD144) || defined(LPC1833JET100)
#define LPC1833
#endif

#if defined(LPC1837FET256) || defined(LPC1837JET256) || defined(LPC1837JBD144) || defined(LPC1837JET100)
#define LPC1837
#endif

#if defined(LPC1850FET256) || defined(LPC1850FET180)
#define LPC1850
#endif

#if defined(LPC1853FET256) || defined(LPC1853JET256) || defined(LPC1853JBD208)
#define LPC1853
#endif

#if defined(LPC1857FET256) || defined(LPC1857JET256) || defined(LPC1857JBD208)
#define LPC1857
#endif

#if defined(LPC1810) || defined(LPC1812) || defined(LPC1813) || defined(LPC1815) || defined(LPC1817)
#define LPC181x
#endif

#if defined(LPC1820) || defined(LPC1822) || defined(LPC1823) || defined(LPC1825) || defined(LPC1827)
#define LPC182x
#endif

#if defined(LPC1830) || defined(LPC1833) || defined(LPC1837)
#define LPC183x
#endif

#if defined(LPC1850) || defined(LPC1853) || defined(LPC1857)
#define LPC185x
#endif

#if defined(LPC181x) || defined(LPC182x) || defined(LPC183x) || defined(LPC185x)
#define LPC18xx
#define UARTS_COUNT                     4
#define I2C_COUNT                       2
#define IRQ_VECTORS_COUNT               52
#define SRAM1_BASE                      0x10080000
//40k
#define SRAM1_SIZE                      0xa000

#define AHB_SRAM1_BASE                  0x20000000

#if defined(LPC1810) || defined(LPC1812) || defined(LPC1813) || defined(LPC1820) || defined(LPC1822) || defined(LPC1823)
//16k
#define AHB_SRAM1_SIZE                  0x4000
#else
//32k+16k
#define AHB_SRAM1_SIZE                  0xc000
#define AHB_SRAM_ONE_PIECE
#endif

#define AHB_SRAM2_BASE                  0x2000C000
//16k
#define AHB_SRAM2_SIZE                  0x4000

#endif

//SRAM0 only, SRAM1 and any external memory is not used in system configuration
#if defined(LPC1812) || defined(LPC1813) || defined(LPC1815) || defined(LPC1817) || defined(LPC1822) || defined(LPC1823) || defined(LPC1825) ||\
    defined(LPC1827) || defined(LPC1833) || defined(LPC1837) || defined(LPC1853) || defined(LPC1857)
//32k
#define SRAM_SIZE                       0x8000
#endif

#if defined(LPC1810)
//64k
#define SRAM_SIZE                       0x10000
#endif

#if defined(LPC1820) || defined(LPC1830) || defined(LPC1850)
//96k
#define SRAM_SIZE                       0x18000
#define FLASH_SIZE_B                    0x0
#endif

#if defined(LPC1813) || defined(LPC1823) || defined(LPC1833) || defined(LPC1853)
//256k
#define FLASH_SIZE                      0x40000
#define FLASH_SIZE_B                    0x40000
#endif

#if defined(LPC1815) || defined(LPC1825)
//384k
#define FLASH_SIZE                      0x60000
#define FLASH_SIZE_B                    0x60000
#endif

#if defined(LPC1812) || defined(LPC1822)
//512k
#define FLASH_SIZE                      0x80000
#define FLASH_SIZE_B                    0x0
#endif

#if defined(LPC1817) || defined(LPC1827) || defined(LPC1837) || defined(LPC1857)
//512k
#define FLASH_SIZE                      0x80000
#define FLASH_SIZE_B                    0x80000
#endif

#if defined(LPC1810) || defined(LPC1820) || defined(LPC1830) || defined(LPC1850)
//compatibility mode
#define FLASH_SIZE                      SRAM_SIZE
#endif

#if defined(LPC181x) || defined(LPC182x)
#define USB_COUNT                       1
#endif

#if defined(LPC183x) || defined(LPC185x)
#define USB_COUNT                       2
#endif

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
#if defined(LPC11Uxx)
#ifndef CORTEX_M0
#define CORTEX_M0
#endif //CORTEX_M0
#endif //LPC11Uxx

#if defined(LPC18xx)
#ifndef CORTEX_M3
#define CORTEX_M3
#endif //CORTEX_M3
#endif //LPC18xx

#if defined(LPC11Uxx) || defined(LPC18xx)
#define LPC
#define EXODRIVERS
#endif

#if defined(LPC)
//shadow area for 18xx
#ifndef FLASH_BASE
#define FLASH_BASE                0x00000000
#endif //FLASH_BASE

#ifndef SRAM_BASE
#define SRAM_BASE                 0x10000000
#endif //SRAM_BASE

#endif //LPC

#if !defined(LDS) && !defined(__ASSEMBLER__)

#if defined(LPC)

#include "lpc_config.h"

#if defined(LPC11Uxx)
#include "lpc11uxx_bits.h"
#if defined(LPC11U6x)
#include "LPC11U6x.h"
#else
#include "LPC11Uxx.h"
#endif //defined(LPC11U6x)
#elif defined(LPC18xx)
#define CMSIS_BITPOSITIONS
#include "LPC18xx.h"
#include "lpc18xx_bits.h"
#endif //defined(LPC11Uxx)


#endif //LPC

#endif // !defined(LDS) && !defined(__ASSEMBLER__)

#endif //LPC_H
