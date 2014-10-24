/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef _TIME_H_
#define _TIME_H_

/*
        time routines
 */

#include "lib.h"
#include "../svc.h"

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


typedef struct {
    time_t (*mktime)(struct tm*);
    struct tm* (*gmtime)(time_t, struct tm*);
    int (*time_compare)(TIME*, TIME*);
    void (*time_add)(TIME*, TIME*, TIME*);
    void (*time_sub)(TIME*, TIME*, TIME*);
    void (*us_to_time)(int, TIME*);
    void (*ms_to_time)(int, TIME*);
    int (*time_to_us)(TIME*);
    int (*time_to_ms)(TIME*);
    TIME* (*time_elapsed)(TIME*, TIME*);
    unsigned int (*time_elapsed_ms)(TIME*);
    unsigned int (*time_elapsed_us)(TIME*);
} LIB_TIME;


/** \addtogroup lib_time time
    time routines
    \{
 */

/**
    \brief POSIX analogue. Convert struct tm to time_t
    \param ts: time in struct \ref tm
    \retval time in \ref time_t
*/
__STATIC_INLINE time_t mktime(struct tm* ts)
{
    return ((const LIB_TIME*)__GLOBAL->lib->p_lib_time)->mktime(ts);
}

/**
    \brief POSIX analogue. Convert time_t to struct tm
    \param time: time in \ref time_t
    \param ts: result time in struct \ref tm
    \retval same as ts
*/
__STATIC_INLINE struct tm* gmtime(time_t time, struct tm* ts)
{
    return ((const LIB_TIME*)__GLOBAL->lib->p_lib_time)->gmtime(time, ts);
}

/**
    \brief compare time.
    \param from: time from
    \param to: time to
    \retval if "from" < "to", return 1, \n
    if "from" > "to", return -1, \n
    if "from" == "t", return 0
*/
__STATIC_INLINE int time_compare(TIME* from, TIME* to)
{
    return ((const LIB_TIME*)__GLOBAL->lib->p_lib_time)->time_compare(from, to);
}

/**
    \brief res = from + to
    \param from: time from
    \param to: time to
    \param res: result time. Safe to be same as "from" or "to"
    \retval none
*/
__STATIC_INLINE void time_add(TIME* from, TIME* to, TIME* res)
{
    ((const LIB_TIME*)__GLOBAL->lib->p_lib_time)->time_add(from, to, res);
}

/**
    \brief res = to - from
    \param from: time from
    \param to: time to
    \param res: result time. Safe to be same as "from" or "to"
    \retval none
*/
__STATIC_INLINE void time_sub(TIME* from, TIME* to, TIME* res)
{
    ((const LIB_TIME*)__GLOBAL->lib->p_lib_time)->time_sub(from, to, res);
}

/**
    \brief convert time in microseconds to \ref TIME structure
    \param us: microseconds
    \param time: pointer to allocated result \ref TIME structure
    \retval none
*/
__STATIC_INLINE void us_to_time(int us, TIME* time)
{
    ((const LIB_TIME*)__GLOBAL->lib->p_lib_time)->us_to_time(us, time);
}

/**
    \brief convert time in milliseconds to \ref TIME structure
    \param ms: milliseconds
    \param time: pointer to allocated result \ref TIME structure
    \retval none
*/
__STATIC_INLINE void ms_to_time(int ms, TIME* time)
{
    ((const LIB_TIME*)__GLOBAL->lib->p_lib_time)->ms_to_time(ms, time);
}

/**
    \brief convert time from \ref TIME structure to microseconds
    \param time: pointer to \ref TIME structure. Maximal value: 0hr, 35 min, 46 seconds
    \retval time in microseconds
*/
__STATIC_INLINE int time_to_us(TIME* time)
{
    return ((const LIB_TIME*)__GLOBAL->lib->p_lib_time)->time_to_us(time);
}

/**
    \brief convert time from \ref TIME structure to milliseconds
    \param time: pointer to \ref TIME structure. Maximal value: 24days, 20hr, 31 min, 22 seconds
    \retval time in milliseconds
*/
__STATIC_INLINE int time_to_ms(TIME* time)
{
    return ((const LIB_TIME*)__GLOBAL->lib->p_lib_time)->time_to_ms(time);
}

/**
    \brief time, elapsed between "from" and now
    \param from: pointer to provided structure, containing base \ref TIME
    \param res: pointer to provided structure, containing result \ref TIME
    \retval same as res parameter
*/
__STATIC_INLINE TIME* time_elapsed(TIME* from, TIME* res)
{
    return ((const LIB_TIME*)__GLOBAL->lib->p_lib_time)->time_elapsed(from, res);
}

/**
    \brief time, elapsed between "from" and now in milliseconds
    \param from: pointer to provided structure, containing base \ref TIME
    \retval elapsed time in milliseconds
*/
__STATIC_INLINE unsigned int time_elapsed_ms(TIME* from)
{
    return ((const LIB_TIME*)__GLOBAL->lib->p_lib_time)->time_elapsed_ms(from);
}

/**
    \brief time, elapsed between "from" and now in microseconds
    \param from: pointer to provided structure, containing base \ref TIME
    \retval elapsed time in microseconds
*/
__STATIC_INLINE unsigned int time_elapsed_us(TIME* from)
{
    return ((const LIB_TIME*)__GLOBAL->lib->p_lib_time)->time_elapsed_us(from);
}

/** \} */ // end of time group

#endif /*_TIME_H_*/
