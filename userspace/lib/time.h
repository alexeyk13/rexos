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
#include "../process.h"

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
    return __GLOBAL->lib->mktime(ts);
}

/**
    \brief POSIX analogue. Convert time_t to struct tm
    \param time: time in \ref time_t
    \param ts: result time in struct \ref tm
    \retval same as ts
*/
__STATIC_INLINE struct tm* gmtime(time_t time, struct tm* ts)
{
    return __GLOBAL->lib->gmtime(time, ts);
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
    return __GLOBAL->lib->time_compare(from, to);
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
    __GLOBAL->lib->time_add(from, to, res);
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
    __GLOBAL->lib->time_sub(from, to, res);
}

/**
    \brief convert time in microseconds to \ref TIME structure
    \param us: microseconds
    \param time: pointer to allocated result \ref TIME structure
    \retval none
*/
__STATIC_INLINE void us_to_time(int us, TIME* time)
{
    __GLOBAL->lib->us_to_time(us, time);
}

/**
    \brief convert time in milliseconds to \ref TIME structure
    \param ms: milliseconds
    \param time: pointer to allocated result \ref TIME structure
    \retval none
*/
__STATIC_INLINE void ms_to_time(int ms, TIME* time)
{
    __GLOBAL->lib->ms_to_time(ms, time);
}

/**
    \brief convert time from \ref TIME structure to microseconds
    \param time: pointer to \ref TIME structure. Maximal value: 0hr, 35 min, 46 seconds
    \retval time in microseconds
*/
__STATIC_INLINE int time_to_us(TIME* time)
{
    return __GLOBAL->lib->time_to_us(time);
}

/**
    \brief convert time from \ref TIME structure to milliseconds
    \param time: pointer to \ref TIME structure. Maximal value: 24days, 20hr, 31 min, 22 seconds
    \retval time in milliseconds
*/
__STATIC_INLINE int time_to_ms(TIME* time)
{
    return __GLOBAL->lib->time_to_ms(time);
}

/**
    \brief time, elapsed between "from" and now
    \param from: pointer to provided structure, containing base \ref TIME
    \param res: pointer to provided structure, containing result \ref TIME
    \retval same as res parameter
*/
__STATIC_INLINE TIME* time_elapsed(TIME* from, TIME* res)
{
    return __GLOBAL->lib->time_elapsed(from, res);
}

/**
    \brief time, elapsed between "from" and now in milliseconds
    \param from: pointer to provided structure, containing base \ref TIME
    \retval elapsed time in milliseconds
*/
__STATIC_INLINE unsigned int time_elapsed_ms(TIME* from)
{
    return __GLOBAL->lib->time_elapsed_ms(from);
}

/**
    \brief time, elapsed between "from" and now in microseconds
    \param from: pointer to provided structure, containing base \ref TIME
    \retval elapsed time in microseconds
*/
__STATIC_INLINE unsigned int time_elapsed_us(TIME* from)
{
    return __GLOBAL->lib->time_elapsed_us(from);
}

/** \} */ // end of time group

#endif /*_TIME_H_*/
