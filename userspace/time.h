/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef TIME_H
#define TIME_H

#include <stdbool.h>

/*
        time routines
 */

/** \addtogroup time time
    time routines
    \{
 */

#define SEC_IN_DAY                          86400ul
#define MSEC_IN_DAY                         86400000ul

//01.01.2015
#define EPOCH_DATE                          735598
//01.01.1970
#define UNIX_EPOCH_DATE                     719162

typedef struct {
    long day;                               //!< day since 01.01.0000 AD. BC is negative: 31.12.-1 is -1
    unsigned int ms;                        //!< milliseconds since midnight
} TIME;

/**
    \brief POSIX analogue of struct tm
    \our struct tm is shorter, than POSIX. RExOS didn't use tm_wday, tm_yday, tm_isdst
*/
struct tm {
    unsigned short tm_msec;                 //!< milliseconds after second [0, 999]
    unsigned char tm_sec;                   //!< seconds after the minute [0, 59]
    unsigned char tm_min;                   //!< minutes after the hour [0, 59]
    unsigned char tm_hour;                  //!< hours since midnight [0, 23]
    unsigned char tm_mday;                  //!< day of the month [1, 31]
    unsigned char tm_mon;                   //!< months since January [1, 12]
    int tm_year;                            //!< years since AD, BC supported as negative value
};

/**
    \brief return true if is leap year
    \param year: year to check
    \retval true if so
*/
bool is_leap_year(long year);

/**
    \brief return max day number (starting from 1) in year
    \param year: year to check
    \param mon: month number from 1
    \retval max day in month
*/
unsigned short year_month_max_day(long year, unsigned short mon);

/**
    \brief POSIX analogue. Convert struct tm to time_t
    \param ts: time in struct \ref tm
    \param time: return time in \ref TIME
    \retval time in \ref TIME
*/
TIME* mktime(struct tm* ts, TIME* time);

/**
    \brief POSIX analogue. Convert time_t to struct tm
    \param time: time in \ref TIME
    \param ts: result time in struct \ref tm
    \retval same as ts
*/
struct tm* gmtime(TIME* time, struct tm* ts);

/** \} */ // end of time group


#endif // TIME_H
