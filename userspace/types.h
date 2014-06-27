/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef _TYPES_H_
#define _TYPES_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifndef NULL
#define NULL								0
#endif

#define HANDLE								unsigned int
#define INVALID_HANDLE                      NULL

#define INFINITE							0x0

#define WORD_SIZE							sizeof(unsigned int)
#define WORD_SIZE_BITS                      (WORD_SIZE * 8)

/** \addtogroup lib_time time
    time routines
    \{
 */


//In 2037, please change this to unsigned long long. In 32 bits mcu changing this can significally decrease perfomance
/**
    \brief time_t POSIX analogue
*/
typedef unsigned long time_t;

/**
    \brief POSIX analogue of struct tm
    \our struct tm is shorter, than POSIX. RExOS didn't use tm_wday, tm_yday, tm_isdst and negative values for perfomance reasons, tm_year - is absolute value
*/
struct tm {
    unsigned char tm_sec;                   //!< seconds after the minute [0, 59]
    unsigned char tm_min;                   //!< minutes after the hour [0, 59]
    unsigned char tm_hour;                  //!< hours since midnight [0, 23]
    unsigned char tm_mday;                  //!< day of the month [1, 31]
    unsigned char tm_mon;                   //!< months since January [0, 11]
    unsigned short tm_year;                 //!< years since 0
};

/**
    \brief structure for holding time units
*/
typedef struct {
    time_t sec;                             //!< seconds
    unsigned int usec;                      //!< microseconds
}TIME;

/** \} */ // end of lib_time group


#endif /*_TYPES_H_*/
