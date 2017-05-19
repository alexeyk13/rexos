/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef STM32_H
#define STM32_H

//---------------------------------------------------------------------------- STM32 F0 ----------------------------------------------------------------------------------------------------------
#if defined(STM32F030F4)
#define STM32F030x4
#endif

#if defined(STM32F030C6) || defined(STM32F030K6)
#define STM32F030x6
#endif

#if defined(STM32F030C8) || defined(STM32F030R8)
#define STM32F030x8
#endif

#if defined(STM32F030CC) || defined(STM32F030RC)
#define STM32F030xC
#endif

#if defined(STM32F031C4) || defined(STM32F031F4) || defined(STM32F031G4) || defined(STM32F031K4)
#define STM32F031x4
#endif

#if defined(STM32F031C6) || defined(STM32F031E6) || defined(STM32F031F6) || defined(STM32F031G6) || defined(STM32F031K6)
#define STM32F031x6
#endif

#if defined(STM32F038C6) || defined(STM32F038E6) || defined(STM32F038F6) || defined(STM32F038G6) || defined(STM32F038K6)
#define STM32F038x6
#endif

#if defined(STM32F042F4) || defined(STM32F042G4) || defined(STM32F042K4) || defined(STM32F042T4) || defined(STM32F042C4)
#define STM32F042x4
#endif

#if defined(STM32F042F6) || defined(STM32F042G6) || defined(STM32F042K6) || defined(STM32F042T6) || defined(STM32F042C6)
#define STM32F042x6
#endif

#if defined(STM32F048G6) || defined(STM32F048T6) || defined(STM32F048C6)
#define STM32F048x6
#endif

#if defined(STM32F051C4) || defined(STM32F031K4) || defined(STM32F031R4)
#define STM32F051x4
#endif

#if defined(STM32F051C6) || defined(STM32F031K6) || defined(STM32F031R6)
#define STM32F051x6
#endif

#if defined(STM32F051C8) || defined(STM32F051K8) || defined(STM32F051R8) || defined(STM32F051T8)
#define STM32F051x8
#endif

#if defined(STM32F058C8) || defined(STM32F058R8) || defined(STM32F058T8)
#define STM32F058x8
#endif

#if defined(STM32F070C6) || defined(STM32F070F6)
#define STM32F070x6
#endif

#if defined(STM32F070CB) || defined(STM32F070RB)
#define STM32F070xB
#endif

#if defined(STM32F071C8) || defined(STM32F071V8)
#define STM32F071x8
#endif

#if defined(STM32F071CB) || defined(STM32F071RB) || defined(STM32F071VB)
#define STM32F071xB
#endif

#if defined(STM32F072C8) || defined(STM32F072R8) || defined(STM32F072V8)
#define STM32F072x8
#endif

#if defined(STM32F072CB) || defined(STM32F072RB) || defined(STM32F072VB)
#define STM32F072xB
#endif

#if defined(STM32F078CB) || defined(STM32F078RB) || defined(STM32F078VB)
#define STM32F078xB
#endif

#if defined(STM32F091CB) || defined(STM32F091RB) || defined(STM32F091VB)
#define STM32F091xB
#endif

#if defined(STM32F091CC) || defined(STM32F091RC) || defined(STM32F091VC)
#define STM32F091xC
#endif

#if defined(STM32F098CC) || defined(STM32F098RC) || defined(STM32F098VC)
#define STM32F098xC
#endif

#if defined(STM32F030x4) || defined(STM32F030x6) ||defined(STM32F030x8) || defined(STM32F030xC)
#define STM32F030
#if defined(STM32F030x4) || defined(STM32F030x6)
#define SRAM_SIZE           0x1000
#define UARTS_COUNT         1
#define I2C_COUNT           1
#define IRQ_VECTORS_COUNT   28
#elif defined(STM32F030x8)
#define SRAM_SIZE           0x2000
#define UARTS_COUNT         2
#define I2C_COUNT           2
#define IRQ_VECTORS_COUNT   29
#else
#define SRAM_SIZE           0x8000
#define UARTS_COUNT         6
#define I2C_COUNT           2
#define IRQ_VECTORS_COUNT   30
#endif
#endif

#if defined(STM32F031x4) || defined(STM32F031x6)
#define STM32F031
#define SRAM_SIZE           0x1000
#define UARTS_COUNT         1
#define IRQ_VECTORS_COUNT   28
#define I2C_COUNT           1
#endif

#if defined(STM32F038x6)
#define STM32F038
#define SRAM_SIZE           0x1000
#define UARTS_COUNT         1
#define IRQ_VECTORS_COUNT   28
#define I2C_COUNT           1
#endif

#if defined(STM32F042x4) || defined(STM32F042x6)
#define STM32F042
#define SRAM_SIZE           0x1800
#define UARTS_COUNT         2
#define IRQ_VECTORS_COUNT   32
#define I2C_COUNT           1
#endif

#if defined(STM32F048x6)
#define STM32F048
#define SRAM_SIZE           0x1800
#define UARTS_COUNT         2
#define IRQ_VECTORS_COUNT   32
#define I2C_COUNT           1
#endif


#if defined(STM32F051x4) || defined(STM32F051x6) || defined(STM32F051x8)
#define STM32F051
#define SRAM_SIZE           0x2000
#if defined(STM32F051x4)
#define UARTS_COUNT         1
#else
#define UARTS_COUNT         2
#endif
#define IRQ_VECTORS_COUNT   31
#if defined(STM32F051C8) || defined(STM32F051R8)
#define I2C_COUNT           2
#else
#define I2C_COUNT           1
#endif
#endif

#if defined(STM32F058x8)
#define STM32F058
#define SRAM_SIZE           0x2000
#define UARTS_COUNT         2
#define I2C_COUNT           2
#define IRQ_VECTORS_COUNT   31
#endif

#if defined(STM32F070x6) || defined(STM32F070xB)
#define STM32F070
#if defined(STM32F070x6)
#define SRAM_SIZE           0x1800
#define UARTS_COUNT         2
#define I2C_COUNT           1
#else
#define SRAM_SIZE           0x4000
#define UARTS_COUNT         4
#define I2C_COUNT           2
#endif
#define IRQ_VECTORS_COUNT   32
#endif

#if defined(STM32F071x8) || defined(STM32F071xB)
#define STM32F071
#define SRAM_SIZE           0x4000
#define UARTS_COUNT         4
#define IRQ_VECTORS_COUNT   31
#define I2C_COUNT           2
#endif

#if defined(STM32F072x8) || defined(STM32F072xB)
#define STM32F072
#define SRAM_SIZE           0x4000
#define UARTS_COUNT         4
#define IRQ_VECTORS_COUNT   32
#define I2C_COUNT           2
#endif

#if defined(STM32F078xB)
#define STM32F078
#define SRAM_SIZE           0x4000
#define UARTS_COUNT         4
#define IRQ_VECTORS_COUNT   32
#define I2C_COUNT           2
#endif

#if defined(STM32F091xB) || defined(STM32F091xC)
#define STM32F091
#define SRAM_SIZE           0x8000
#if defined(STM32F091CC) || defined(STM32F091CB)
#define UARTS_COUNT         6
#else
#define UARTS_COUNT         8
#endif
#define IRQ_VECTORS_COUNT   31
#define I2C_COUNT           2
#endif

#if defined(STM32F098xC)
#define STM32F098
#define SRAM_SIZE           0x8000
#if defined(STM32F098CC)
#define UARTS_COUNT         6
#else
#define UARTS_COUNT         8
#endif
#define IRQ_VECTORS_COUNT   31
#define I2C_COUNT           2
#endif

#if defined(STM32F030) || defined(STM32F031) || defined(STM32F038)
#define STM32F03x
#endif

#if defined(STM32F042) || defined(STM32F048)
#define STM32F04x
#endif

#if defined(STM32F051) || defined(STM32F058)
#define STM32F05x
#endif

#if defined(STM32F071) || defined(STM32F072) || defined(STM32F078)
#define STM32F07x
#endif

#if defined(STM32F091) || defined(STM32F098)
#define STM32F09x
#endif

#if defined(STM32F031) || defined(STM32F051) || defined(STM32F071) || defined(STM32F091)
#define STM32F0x1
#endif

#if defined(STM32F042) || defined(STM32F072)
#define STM32F0x2
#endif

#if defined(STM32F038) || defined(STM32F048) || defined(STM32F058) || defined(STM32F078) || defined(STM32F078)
#define STM32F0x8
#endif

#if defined(STM32F030x4) || defined(STM32F031x4) || defined(STM32F042x4) || defined(STM32F051x4)
#define FLASH_SIZE          0x4000
#endif

#if defined(STM32F030x6) || defined(STM32F031x6) || defined(STM32F038x6) || defined(STM32F042x6) || defined(STM32F048x6) || defined(STM32F070x6) || defined(STM32F051x6)
#define FLASH_SIZE          0x8000
#endif

#if defined(STM32F030x8) || defined(STM32F051x8) || defined(STM32F058x8) || defined(STM32F071x8) || defined(STM32F072x8)
#define FLASH_SIZE          0x10000
#endif

#if defined(STM32F070xB) || defined(STM32F071xB) || defined(STM32F072xB) || defined(STM32F078xB) || defined(STM32F091xB)
#define FLASH_SIZE          0x20000
#endif

#if defined(STM32F030xC) || defined(STM32F091xC) || defined(STM32F098xC)
#define FLASH_SIZE          0x40000
#endif

#if defined(STM32F0x0) || defined(STM32F0x1) || defined(STM32F0x2) || defined(STM32F0x8)
#define STM32F0
//All models have A, B, C, F
#define GPIO_COUNT          6
#endif

//required for ST header compatibility
#if defined(STM32F030x4)
#define STM32F030x6
#endif

#if defined(STM32F031x4)
#define STM32F031x6
#endif

#if defined(STM32F038x6)
#define STM32F038xx
#endif

#if defined(STM32F042x4)
#define STM32F042x6
#endif

#if defined(STM32F048x6)
#define STM32F048xx
#endif

#if defined(STM32F051x4) || defined(STM32F051x6)
#define STM32F051x8
#endif

#if defined(STM32F058x8)
#define STM32F058xx
#endif

#if defined(STM32F071x8)
#define STM32F071xB
#endif

#if defined(STM32F072x8)
#define STM32F072xB
#endif

#if defined(STM32F078xB)
#define STM32F078xx
#endif

#if defined(STM32F091xB)
#define STM32F091xC
#endif

#if defined(STM32F098xC)
#define STM32F098xx
#endif

//---------------------------------------------------------------------------- STM32 F1 ----------------------------------------------------------------------------------------------------------
#if defined(STM32F100C4) || defined(STM32F100C6) || defined(STM32F100C8) || defined(STM32F100CB) \
 || defined(STM32F100R4) || defined(STM32F100R6) || defined(STM32F100R8) || defined(STM32F100RB) || defined(STM32F100RC) || defined(STM32F100RD) || defined(STM32F100RE) \
 || defined(STM32F100V8) || defined(STM32F100VB) || defined(STM32F100VC) || defined(STM32F100VD) || defined(STM32F100VE) \
 || defined(STM32F100ZC) || defined(STM32F100ZD) || defined(STM32F100ZE)
#define STM32F100
#endif

#if defined(STM32F101C4) || defined(STM32F101C6) || defined(STM32F101C8) || defined(STM32F101CB) \
 || defined(STM32F101R4) || defined(STM32F101R6) || defined(STM32F101R8) || defined(STM32F101RB) || defined(STM32F101RC) || defined(STM32F101RD) || defined(STM32F101RE) \
 || defined(STM32F101RF) || defined(STM32F101RG) \
 || defined(STM32F101T4) || defined(STM32F101T6) || defined(STM32F101T8) || defined(STM32F101TB) \
 || defined(STM32F101V8) || defined(STM32F101VB) || defined(STM32F101VC) || defined(STM32F101VD) || defined(STM32F101VE) || defined(STM32F101VF) || defined(STM32F101VG)  \
 || defined(STM32F101ZC) || defined(STM32F101ZD) || defined(STM32F101ZE) || defined(STM32F101ZG)
#define STM32F101
#endif

#if defined(STM32F102C4) || defined(STM32F102C6) || defined(STM32F102C8) || defined(STM32F102CB) \
 || defined(STM32F102R4) || defined(STM32F102R6) || defined(STM32F102R8) || defined(STM32F102RB)
#define STM32F102
#endif

#if defined(STM32F103C4) || defined(STM32F103C6) || defined(STM32F103C8) || defined(STM32F103CB) \
 || defined(STM32F103R4) || defined(STM32F103R6) || defined(STM32F103R8) || defined(STM32F103RB) || defined(STM32F103RC) || defined(STM32F103RD) || defined(STM32F103RE) \
 || defined(STM32F103RF) || defined(STM32F103RG) \
 || defined(STM32F103T4) || defined(STM32F103T6) || defined(STM32F103T8) || defined(STM32F103TB) \
 || defined(STM32F103V8) || defined(STM32F103VB) || defined(STM32F103VC) || defined(STM32F103VD) || defined(STM32F103VE) || defined(STM32F103VF) || defined(STM32F103VG)  \
 || defined(STM32F103ZC) || defined(STM32F103ZD) || defined(STM32F103ZE) || defined(STM32F103ZF) || defined(STM32F103ZG)
#define STM32F103
#endif

#if defined(STM32F105R8) || defined(STM32F105RB) || defined(STM32F105RC) \
 || defined(STM32F105V8) || defined(STM32F105VB) || defined(STM32F105VC)
#define STM32F105
#endif

#if defined(STM32F107RB) || defined(STM32F107RC) \
 || defined(STM32F107VB) || defined(STM32F107VC)
#define STM32F107
#endif

#if defined(STM32F100C4) || defined(STM32F100R4)
#define FLASH_SIZE        0x4000
#define STM32F10X_LD_VL
#endif //16k LD_VL

#if defined(STM32F101C4) || defined(STM32F101R4) || defined(STM32F101T4) || defined(STM32F102C4) || defined(STM32F102R4) || defined(STM32F103C4) || defined(STM32F103R4) || defined(STM32F103T4)
#define FLASH_SIZE        0x4000
#define STM32F10X_LD
#endif //16k LD

#if defined(STM32F100C6) || defined(STM32F100R6)
#define FLASH_SIZE        0x8000
#define STM32F10X_LD_VL
#endif //32k LD_VL

#if defined(STM32F101C6) || defined(STM32F101R6) || defined(STM32F101T6) || defined(STM32F102C6) || defined(STM32F102R6) || defined(STM32F103C6) || defined(STM32F103R6) || defined(STM32F103T6)
#define FLASH_SIZE        0x8000
#define STM32F10X_LD
#endif //32k LD

#if defined(STM32F100C8) || defined(STM32F100R8) || defined(STM32F100V8)
#define FLASH_SIZE        0x10000
#define STM32F10X_MD_VL
#endif //64k MD_VL

#if defined(STM32F101C8) || defined(STM32F101R8) || defined(STM32F101T8) || defined(STM32F101V8) || defined(STM32F102C8) || defined(STM32F102R8) || defined(STM32F103C8) || defined(STM32F103R8) \
 || defined(STM32F103T8) || defined(STM32F103V8)
#define FLASH_SIZE        0x10000
#define STM32F10X_MD
#endif //64k MD

#if defined(STM32F105R8) || defined(STM32F105V8)
#define FLASH_SIZE        0x10000
#define STM32F10X_CL
#endif //64k CL

#if defined(STM32F100CB) || defined(STM32F100RB) || defined(STM32F100VB)
#define FLASH_SIZE        0x20000
#define STM32F10X_MD_VL
#endif //128k MD_VL

#if defined(STM32F101CB) || defined(STM32F101RB) || defined(STM32F101TB) || defined(STM32F101VB) || defined(STM32F102CB) || defined(STM32F102RB) || defined(STM32F103CB) || defined(STM32F103RB) \
 || defined(STM32F103TB) || defined(STM32F103VB)
#define FLASH_SIZE        0x20000
#define STM32F10X_MD
#endif //128k MD

#if defined(STM32F105RB) || defined(STM32F105VB) || defined(STM32F107RB) || defined(STM32F107VB)
#define FLASH_SIZE        0x20000
#define STM32F10X_CL
#endif //128k CL

#if defined(STM32F100RC) || defined(STM32F100VC) || defined(STM32F100ZC)
#define FLASH_SIZE        0x40000
#define STM32F10X_HD_VL
#endif //256k HD_VL

#if defined(STM32F101RC) || defined(STM32F101VC) || defined(STM32F101ZC) || defined(STM32F103RC) || defined(STM32F103VC) || defined(STM32F103ZC)
#define FLASH_SIZE        0x40000
#define STM32F10X_HD
#endif //256k HD

#if defined(STM32F105RC) || defined(STM32F105VC) || defined(STM32F107RC) || defined(STM32F107VC)
#define FLASH_SIZE        0x40000
#define STM32F10X_CL
#endif //256k CL

#if defined(STM32F100RD) || defined(STM32F100VD) || defined(STM32F100ZD)
#define FLASH_SIZE        0x60000
#define STM32F10X_HD_VL
#endif //384k HD_VL

#if defined(STM32F101RD) || defined(STM32F101VD) || defined(STM32F101ZD) || defined(STM32F103RD) || defined(STM32F103VD) || defined(STM32F103ZD)
#define FLASH_SIZE        0x60000
#define STM32F10X_HD
#endif //384k HD

#if defined(STM32F100RE) || defined(STM32F100VE) || defined(STM32F100ZE)
#define FLASH_SIZE        0x80000
#define STM32F10X_HD_VL
#endif //512k HD_VL

#if defined(STM32F101RE) || defined(STM32F101VE) || defined(STM32F101ZE) || defined(STM32F103RE) || defined(STM32F103VE) || defined(STM32F103ZE)
#define FLASH_SIZE        0x80000
#define STM32F10X_HD
#endif //512k HD

#if defined(STM32F101RF) || defined(STM32F101VF) || defined(STM32F103RF) || defined(STM32F103VF) || defined(STM32F103ZF)
#define FLASH_SIZE        0xC0000
#define STM32F10X_XL
#endif //768k XL

#if defined(STM32F101RG) || defined(STM32F101VG) || defined(STM32F101ZG) || defined(STM32F103RG) || defined(STM32F103VG) || defined(STM32F103ZG)
#define FLASH_SIZE        0x100000
#define STM32F10X_XL
#endif //1m XL

#if defined(STM32F10X_LD) || defined(STM32F10X_MD)
#define IRQ_VECTORS_COUNT   43
#elif defined(STM32F10X_LD_VL) || defined(STM32F10X_MD_VL)
#define IRQ_VECTORS_COUNT   56
#elif defined(STM32F10X_HD) || defined(STM32F10X_XL)
#define IRQ_VECTORS_COUNT   60
#elif defined(STM32F10X_HD_VL)
#define IRQ_VECTORS_COUNT   61
#elif defined(STM32F10X_CL)
#define IRQ_VECTORS_COUNT   68
#endif

#if defined(STM32F100)
#if defined(STM32F10X_LD_VL)
#define SRAM_SIZE        0x1000
#elif defined(STM32F10X_MD_VL)
#define SRAM_SIZE        0x2000
#elif (FLASH_SIZE == 0x40000)
#define SRAM_SIZE        0x6000
#elif defined(STM32F10X_HD_VL)
#define SRAM_SIZE        0x8000
#endif //STM32F100

#elif defined(STM32F101) || defined(STM32F102)
#if (FLASH_SIZE == 0x4000)
#define SRAM_SIZE        0x1000
#elif (FLASH_SIZE == 0x8000)
#define SRAM_SIZE        0x1800
#elif (FLASH_SIZE == 0x10000)
#define SRAM_SIZE        0x2800
#elif (FLASH_SIZE == 0x20000)
#define SRAM_SIZE        0x4000
#elif (FLASH_SIZE == 0x40000)
#define SRAM_SIZE        0x8000
#elif defined(STM32F10X_HD)
#define SRAM_SIZE        0xC000
#elif defined(STM32F10X_XL)
#define SRAM_SIZE        0x14000
#endif //STM32F101 || STM32F102

#elif defined(STM32F103)
#if (FLASH_SIZE == 0x4000)
#define SRAM_SIZE        0x1800
#elif (FLASH_SIZE == 0x8000)
#define SRAM_SIZE        0x2800
#elif defined(STM32F10X_MD)
#define SRAM_SIZE        0x5000
#elif (FLASH_SIZE == 0x40000)
#define SRAM_SIZE        0xC000
#elif defined(STM32F10X_HD)
#define SRAM_SIZE        0x10000
#elif defined(STM32F10X_XL)
#define SRAM_SIZE        0x18000
#endif //STM32F103

#elif defined(STM32F105) || defined(STM32F107)
#define SRAM_SIZE        0x10000

#endif  //SRAM_SIZE

#if defined(STM32F100) || defined(STM32F101) || defined(STM32F102) || defined(STM32F103) || defined(STM32F105) || defined(STM32F107)
#define STM32F1
//it's what defined in STM32F1xxx.h. Real count may be less.
#define GPIO_COUNT                                              7
#define DAC_CHANNELS_COUNT                                      2

#if defined(STM32F10X_LD) || defined(STM32F10X_LD_VL)
#define UARTS_COUNT                                             2
#elif defined(STM32F10X_MD) || defined(STM32F10X_MD_VL)
#define UARTS_COUNT                                             3
#define I2C_COUNT                                               2
#define SPI_COUNT                                               2
#define CAN_COUNT                                               1
#else
#define UARTS_COUNT                                             5
#endif

#endif

//---------------------------------------------------------------------------- stm 32 F2 ----------------------------------------------------------------------------------------------------------
#if defined(STM32F205RB) || defined(STM32F205RC) || defined(STM32F205RE) || defined(STM32F205RF) || defined(STM32F205RG) || defined(STM32F205VG) || defined(STM32F205ZG) || defined(STM32F205VF) \
 || defined(STM32F205ZF) || defined(STM32F205VE) || defined(STM32F205ZE) || defined(STM32F205VC) || defined(STM32F205ZC) || defined(STM32F205VB)

#define STM32F205
#endif

#if defined(STM32F207VC) || defined(STM32F207VE) || defined(STM32F207VF) || defined(STM32F207VG) || defined(STM32F207ZC) || defined(STM32F207ZE) || defined(STM32F207ZF) || defined(STM32F207ZG) \
    || defined(STM32F207IC) || defined(STM32F207IE) || defined(STM32F207IF) || defined(STM32F207IG)
#define STM32F207
#endif

#if defined(STM32F215RE) || defined(STM32F215RG) || defined(STM32F215VG) || defined(STM32F215ZG) || defined(STM32F215VE) || defined(STM32F215ZE)
#define STM32F215
#endif

#if defined(STM32F217VE) || defined(STM32F217VG) || defined(STM32F217ZE) || defined(STM32F217ZG) || defined(STM32F217IE) || defined(STM32F217IG)
#define STM32F217
#endif

#if defined(STM32F205) || defined(STM32F207) || defined(STM32F215) || defined(STM32F217)
#define STM32F2
#define IRQ_VECTORS_COUNT   81
#define GPIO_COUNT          9
#define UARTS_COUNT         6

#if defined(STM32F205RB) || defined(STM32F205VB)
#define FLASH_SIZE        0x20000
#endif //128K

#if defined(STM32F205RC) || defined(STM32F205VC) || defined(STM32F205ZC) || defined(STM32F207VC) || defined(STM32F207ZC) || defined(STM32F207IC)
#define FLASH_SIZE        0x40000
#endif //256K

#if defined(STM32F205RE) || defined(STM32F205VE) || defined(STM32F205ZE) || defined(STM32F215RE) || defined(STM32F215VE) || defined(STM32F215ZE) || defined(STM32F207VE) || defined(STM32F207ZE) \
    || defined(STM32F207IE)  || defined(STM32F217VE) || defined(STM32F217ZE) || defined(STM32F207IE)
#define FLASH_SIZE        0x80000
#endif //512K

#if defined(STM32F205RF) || defined(STM32F205VF) || defined(STM32F205ZF) || defined(STM32F207VF) || defined(STM32F207ZF) || defined(STM32F207IF)
#define FLASH_SIZE        0xc0000
#endif //768K

#if defined(STM32F205RG) || defined(STM32F205VG) || defined(STM32F205ZG) || defined(STM32F215RG) || defined(STM32F215VG) || defined(STM32F215ZG) || defined(STM32F207VG) || defined(STM32F207ZG) \
    || defined(STM32F207IG)  || defined(STM32F217VG) || defined(STM32F217ZG) || defined(STM32F207IG)
#define FLASH_SIZE        0x100000
#endif //1M

#if defined(STM32F215) || defined(STM32F207) || defined(STM32F217) || (FLASH_SIZE > 0x40000)
#define SRAM_SIZE        0x20000
#elif (FLASH_SIZE==0x20000)
#define SRAM_SIZE        0x10000
#elif (FLASH_SIZE==0x40000)
#define SRAM_SIZE        0x18000
#endif
#endif //defined(STM32F205) || defined(STM32F207) || defined(STM32F215) || defined(STM32F217)

//---------------------------------------------------------------------------- stm 32 F4 ----------------------------------------------------------------------------------------------------------
#if defined(STM32F401CB) || defined(STM32F401CC) || defined(STM32F401CE) || defined(STM32F401RB) || defined(STM32F401RC) || defined(STM32F401RE) \
 || defined(STM32F401VB) || defined(STM32F401VC) || defined(STM32F401VE)
#define STM32F401

#if defined(STM32F401CE) || defined(STM32F401RE) || defined(STM32F401VE)
//96K
#define SRAM_SIZE           0x18000
#else
//64K
#define SRAM_SIZE           0x10000
#endif
#endif

#if defined(STM32F405OE) || defined(STM32F405OG) || defined(STM32F405RG) || defined(STM32F405VG) || defined(STM32F405ZG)
#define STM32F405
//192K
#define SRAM_SIZE           0x30000
#endif

#if defined(STM32F407IE) || defined(STM32F407IG) || defined(STM32F407VE) || defined(STM32F407VG) || defined(STM32F407ZE) || defined(STM32F407ZG)
#define STM32F407
//192K
#define SRAM_SIZE           0x30000
#endif

#if defined(STM32F411CC) || defined(STM32F411CE) || defined(STM32F411RC) || defined(STM32F411RE) || defined(STM32F411VC) || defined(STM32F411VE)
#define STM32F411
//128K
#define SRAM_SIZE           0x20000
#endif

#if defined(STM32F415OG) || defined(STM32F415RG) || defined(STM32F415VG) || defined(STM32F415ZG)
#define STM32F415
//192K
#define SRAM_SIZE           0x30000
#endif

#if defined(STM32F417IE) || defined(STM32F417IG) || defined(STM32F417VE) || defined(STM32F417VG) || defined(STM32F417ZE) || defined(STM32F417ZG)
#define STM32F417
//192K
#define SRAM_SIZE           0x30000
#endif

#if defined(STM32F427IG) || defined(STM32F427II) || defined(STM32F427VG) || defined(STM32F427VI) || defined(STM32F427ZG) || defined(STM32F427ZI)
#define STM32F427
//256K
#define SRAM_SIZE           0x40000
#endif

#if defined(STM32F429BE) || defined(STM32F429BG) || defined(STM32F429BI) || defined(STM32F429IE) || defined(STM32F429IG) || defined(STM32F429II) \
 || defined(STM32F429NE) || defined(STM32F429NG) || defined(STM32F429NI) || defined(STM32F429VE) || defined(STM32F429VG) || defined(STM32F429VI) \
 || defined(STM32F429ZE) || defined(STM32F429ZG) || defined(STM32F429ZI)
#define STM32F429
//256K
#define SRAM_SIZE           0x40000
#endif

#if defined(STM32F439BG) || defined(STM32F439BI) || defined(STM32F439IG) || defined(STM32F439II) || defined(STM32F439NG) || defined(STM32F439NI) \
 || defined(STM32F439VG) || defined(STM32F439VI) || defined(STM32F439ZG) || defined(STM32F439ZI)
#define STM32F439
//256K
#define SRAM_SIZE           0x40000
#endif

#if defined(STM32F437IG) || defined(STM32F437II) || defined(STM32F437VG) || defined(STM32F437VI) || defined(STM32F437ZG) || defined(STM32F437ZI)
#define STM32F437
//256K
#define SRAM_SIZE           0x40000
#endif

#if defined(STM32F401CB) || defined(STM32F401RB) || defined(STM32F401VB)
//128K
#define FLASH_SIZE          0x20000
#endif

#if defined(STM32F401CC) || defined(STM32F401RC) || defined(STM32F401VC) \
 || defined(STM32F411CC) || defined(STM32F411RC) || defined(STM32F411VC)
//256K
#define FLASH_SIZE          0x40000
#endif

#if defined(STM32F401CE) || defined(STM32F401RE) || defined(STM32F401VE) \
|| defined(STM32F405OE) \
|| defined(STM32F407IE) || defined(STM32F407VE) || defined(STM32F407ZE) \
|| defined(STM32F411CE) || defined(STM32F411RE) || defined(STM32F411VE) \
|| defined(STM32F417IE) || defined(STM32F417VE) || defined(STM32F417ZE) \
|| defined(STM32F429BE) || defined(STM32F429IE) || defined(STM32F429NE) || defined(STM32F429VE) || defined(STM32F429ZE)
//512K
#define FLASH_SIZE          0x80000
#endif

#if defined(STM32F405OG) || defined(STM32F405RG) || defined(STM32F405VG) || defined(STM32F405ZG) \
|| defined(STM32F407IG) || defined(STM32F407VG) || defined(STM32F407ZG) \
|| defined(STM32F415OG) || defined(STM32F415RG) || defined(STM32F415VG) || defined(STM32F415ZG) \
|| defined(STM32F417IG) || defined(STM32F417VG) || defined(STM32F417ZG) \
|| defined(STM32F427IG) || defined(STM32F427VG) || defined(STM32F427ZG) \
|| defined(STM32F429BG) || defined(STM32F429IG) || defined(STM32F429NG) || defined(STM32F429VG) || defined(STM32F429ZG) \
|| defined(STM32F437IG) || defined(STM32F437VG) || defined(STM32F437ZG) \
|| defined(STM32F439BG) || defined(STM32F439IG) || defined(STM32F439NG) || defined(STM32F439VG) || defined(STM32F439ZG)
//1M
#define FLASH_SIZE          0x100000
#endif

#if defined(STM32F427II) || defined(STM32F427VI) || defined(STM32F427ZI) \
 || defined(STM32F429BI) || defined(STM32F429II) || defined(STM32F429NI) || defined(STM32F429VI) || defined(STM32F429ZI) \
 || defined(STM32F437II) || defined(STM32F437VI) || defined(STM32F437ZI) \
 || defined(STM32F439BI) || defined(STM32F439II) || defined(STM32F439NI) || defined(STM32F439VI) || defined(STM32F439ZI)
//2M
#define FLASH_SIZE          0x200000
#endif

#if defined(STM32F401) || defined(STM32F405) || defined(STM32F407) || defined(STM32F411) || defined(STM32F415) || defined(STM32F417) || defined(STM32F427) || defined(STM32F429)  \
 || defined(STM32F437) || defined(STM32F439)
#define STM32F4
#define GPIO_COUNT          11

#if defined(STM32F40_41xxx)
#define UARTS_COUNT                                             6
#else defined(STM32F4)
#define UARTS_COUNT                                             8
#endif


#if defined(STM32F401) || defined(STM32F405) || defined(STM32F407) || defined(STM32F411) || defined(STM32F415) || defined(STM32F417)
#define STM32F40_41xxx
#if defined(STM32F401)
#define STM32F401xx
#define IRQ_VECTORS_COUNT   85
#else
#define IRQ_VECTORS_COUNT   82
#endif
#elif defined(STM32F427) || defined(STM32F437)
#define STM32F427_437xx
#define IRQ_VECTORS_COUNT   91
#elif defined(STM32F429) || defined(STM32F439)
#define STM32F429_439xx
#define IRQ_VECTORS_COUNT   91
#endif

#endif

//---------------------------------------------------------------------------- STM32 L0 ----------------------------------------------------------------------------------------------------------
#if defined(STM32L051C6) || defined(STM32L051C8) || defined(STM32L051K6) || defined(STM32L051K8) || defined(STM32L051R6) || defined(STM32L051R8) \
 || defined(STM32L051T6) || defined(STM32L051T8)
#define STM32L051xx
#endif

#if defined(STM32L052C6) || defined(STM32L052C8) || defined(STM32L052K6) || defined(STM32L052K8) || defined(STM32L052R6) || defined(STM32L052R8) \
 || defined(STM32L052T6) || defined(STM32L052T8)
#define STM32L052xx
#endif

#if defined(STM32L053C6) || defined(STM32L053C8) || defined(STM32L053R6) || defined(STM32L053R8)
#define STM32L053xx
#endif

#if defined(STM32L062K8)
#define STM32L062xx
#endif

#if defined(STM32L063C8) || defined(STM32L063R8)
#define STM32L063xx
#endif

#if defined(STM32L051C6) || defined(STM32L051K6) || defined(STM32L051R6) || defined(STM32L051T6) \
 || defined(STM32L052C6) || defined(STM32L052K6) || defined(STM32L052R6) || defined(STM32L052T6) \
 || defined(STM32L053C6) || defined(STM32L053R6)
#define FLASH_SIZE          0x8000
#endif

#if defined(STM32L051C8) || defined(STM32L051K8) || defined(STM32L051R8) || defined(STM32L051T8) \
 || defined(STM32L052C8) || defined(STM32L052K8) || defined(STM32L052R8) || defined(STM32L052T8) \
 || defined(STM32L053C8) || defined(STM32L053R8) || defined(STM32L062K8) || defined(STM32L063C8) \
 || defined(STM32L063R8)
#define FLASH_SIZE          0x10000
#endif

#if defined(STM32L051xx) || defined(STM32L052xx) || defined(STM32L053xx) || defined(STM32L061xx) || defined(STM32L062xx) || defined(STM32L063xx)
#define STM32L0
#define SRAM_SIZE           0x2000
#define GPIO_COUNT          3
#define UARTS_COUNT         2
#define DAC_CHANNELS_COUNT  1

#if defined(STM32L051xx) || defined(STM32L061xx)
#define IRQ_VECTORS_COUNT   30
#else
#define IRQ_VECTORS_COUNT   32
#endif

#endif


//---------------------------------------------------------------------------- STM32 L1 ----------------------------------------------------------------------------------------------------------
#if defined(STM32L151C6) || defined(STM32L151C8) || defined(STM32L151CB)
#define STM32L151xC
#endif

#if defined(STM32L151R6) || defined(STM32L151R8) || defined(STM32L151RB)
#define STM32L151xR
#endif

#if defined(STM32L151V8) || defined(STM32L151VB)
#define STM32L151xV
#endif

#if defined(STM32L151C6) || defined(STM32L151R6)
#define FLASH_SIZE          0x8000
#elif defined(STM32L151C8) || defined(STM32L151R8)
#define FLASH_SIZE          0x10000
#elif defined(STM32L151CB) || defined(STM32L151RB)
#define FLASH_SIZE          0x20000
#endif

#if defined(STM32L151C6) || defined(STM32L151C8) || defined(STM32L151V8)
#define SRAM_SIZE           0x2800
#elif defined(STM32L151CB) || defined(STM32L151RB) || defined(STM32L151VB)
#define SRAM_SIZE           0x4000
#endif


#if defined(STM32L151xC) || defined(STM32L151xR) || defined(STM32L151xV)
#define STM32L1
#define EEPROM_SIZE         0x1000
#define TIM_COUNT           8
#define SPI_COUNT           2
#define I2C_COUNT           2
#define UARTS_COUNT         3
#define GPIO_COUNT          4
#define DAC_CHANNELS_COUNT  2
#endif

#if defined(STM32L151xC)
#define IRQ_VECTORS_COUNT   45
#endif

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#if defined(STM32F1) || defined(STM32F2) || defined(STM32F4) || defined(STM32L1)
#define STM32
#ifndef CORTEX_M3
#define CORTEX_M3
#endif
#endif // STM32F1 || STM32F2 || STM32F4 || STM32L1

#if defined(STM32L0) || defined(STM32F0)
#define STM32
#ifndef CORTEX_M0
#define CORTEX_M0
#endif
#endif //STM32L0 || STM32F0

#if defined(STM32)
#ifndef FLASH_BASE
#define FLASH_BASE                0x08000000
#endif
#endif

#if !defined(LDS) && !defined(__ASSEMBLER__)

#if defined(STM32)

//fucking morons in ST forgot to check if variable is already defined
#undef SRAM_BASE
#undef FLASH_BASE

#include "stm32_config.h"

#if defined(STM32F0)
#include "stm32f0xx.h"
#elif defined(STM32F1)
#include "stm32f10x.h"
#elif defined(STM32F2)
#include "stm32f2xx.h"
#elif defined(STM32F4)
#include "stm32f4xx.h"
#elif defined(STM32L0)
#include "stm32l0xx.h"
#elif defined(STM32L1)
#include "stm32l1xx.h"
#endif

#endif //!defined(LDS) && !defined(__ASSEMBLER__)

#ifdef USB_EP_BULK
#undef USB_EP_BULK
#endif
#ifdef USB_EP_CONTROL
#undef USB_EP_CONTROL
#endif
#ifdef USB_EP_ISOCHRONOUS
#undef USB_EP_ISOCHRONOUS
#endif
#ifdef USB_EP_INTERRUPT
#undef USB_EP_INTERRUPT
#endif

#endif //STM32

#endif //STM32_H
