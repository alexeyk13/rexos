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

#include "../userspace/types.h"
#include "../userspace/cc_macro.h"
#include "../userspace/core/core.h"

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

/**
    \brief compare time.
    \param from: time from
    \param to: time to
    \retval if "from" < "to", return 1, \n
    if "from" > "to", return -1, \n
    if "from" == "t", return 0
*/
int time_compare(TIME* from, TIME* to);

/**
    \brief res = from + to
    \param from: time from
    \param to: time to
    \param res: result time. Safe to be same as "from" or "to"
    \retval none
*/
void time_add(TIME* from, TIME* to, TIME* res);

/**
    \brief res = to - from
    \param from: time from
    \param to: time to
    \param res: result time. Safe to be same as "from" or "to"
    \retval none
*/
void time_sub(TIME* from, TIME* to, TIME* res);

/**
    \brief convert time in microseconds to \ref TIME structure
    \param us: microseconds
    \param time: pointer to allocated result \ref TIME structure
    \retval none
*/
void us_to_time(int us, TIME* time);

/**
    \brief convert time in milliseconds to \ref TIME structure
    \param ms: milliseconds
    \param time: pointer to allocated result \ref TIME structure
    \retval none
*/
void ms_to_time(int ms, TIME* time);

/**
    \brief convert time from \ref TIME structure to microseconds
    \param time: pointer to \ref TIME structure. Maximal value: 0hr, 35 min, 46 seconds
    \retval time in microseconds
*/
int time_to_us(TIME* time);

/**
    \brief convert time from \ref TIME structure to milliseconds
    \param time: pointer to \ref TIME structure. Maximal value: 24days, 20hr, 31 min, 22 seconds
    \retval time in milliseconds
*/
int time_to_ms(TIME* time);

/** \} */ // end of time group

#endif /*_TIME_H_*/
