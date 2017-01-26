/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef CC_MACRO_H
#define CC_MACRO_H

#if   defined ( __CC_ARM )
    #define __ASM            __asm                                      /*!< asm keyword for ARM Compiler          */
    #define __INLINE         __inline                                   /*!< inline keyword for ARM Compiler       */
    #define __STATIC_INLINE  static __inline
    #define __PACKED         __packed

#elif defined ( __ICCARM__ )
    #define __ASM            __asm                                      /*!< asm keyword for IAR Compiler          */
    #define __INLINE         inline                                     /*!< inline keyword for IAR Compiler. Only available in High optimization mode! */
    #define __STATIC_INLINE  static inline
    #define __PACKED         __attribute__((packed))

#elif defined ( __GNUC__ )
    #define __ASM            __asm                                      /*!< asm keyword for GNU Compiler          */
    #define __INLINE         inline                                     /*!< inline keyword for GNU Compiler       */
    #define __STATIC_INLINE  static inline
    #define __PACKED         __attribute__((packed))
    //read R/C register
    #define __REG_RC16(reg)     volatile uint16_t __attribute__ ((unused)) __reg = (reg)
    #define __REG_RC32(reg)     volatile uint32_t __attribute__ ((unused)) __reg = (reg)

#elif defined ( __TASKING__ )
  #define __ASM            __asm                                      /*!< asm keyword for TASKING Compiler      */
  #define __INLINE         inline                                     /*!< inline keyword for TASKING Compiler   */
  #define __STATIC_INLINE  static inline
  #define __PACKED         __attribute__((packed))
#endif



#endif // CC_MACRO_H

