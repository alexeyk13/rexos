/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef TIME_H
#define TIME_H

/*
        time routines
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

/** \} */ // end of lib_time group


/** \addtogroup lib_time time
    time routines
    \{
 */

/**
    \brief POSIX analogue. Convert struct tm to time_t
    \param ts: time in struct \ref tm
    \retval time in \ref time_t
*/
time_t mktime(struct tm* ts);

/**
    \brief POSIX analogue. Convert time_t to struct tm
    \param time: time in \ref time_t
    \param ts: result time in struct \ref tm
    \retval same as ts
*/
struct tm* gmtime(time_t time, struct tm* ts);

#endif // TIME_H
