/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "time.h"

//time_t = 0
#define EPOCH_YEAR                        1970

//is year is leap?
#define IS_LEAP_YEAR(year)                (!((year) % 4) && (((year) % 100) || !((year) % 400)))
#define YEARSIZE(year)                    (IS_LEAP_YEAR(year) ? 366 : 365)

//seconds in day
#define SECS_IN_DAY                        (24l * 60l * 60l)

const unsigned short MDAY[2][12] =    {{ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
                                                 { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }};

const unsigned short YDAY[2][12] =    {{  0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334},
                                                 {  0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335}};

time_t mktime(struct tm* ts)
{
    register time_t days_from_epoch;
    days_from_epoch = (ts->tm_year - EPOCH_YEAR) * 365;
    days_from_epoch += (ts->tm_year - EPOCH_YEAR) / 4 + ((ts->tm_year % 4) && ts->tm_year % 4 < EPOCH_YEAR % 4);
    days_from_epoch -= (ts->tm_year - EPOCH_YEAR) / 100 + ((ts->tm_year % 100) && ts->tm_year % 100 < EPOCH_YEAR % 100);
    days_from_epoch += (ts->tm_year - EPOCH_YEAR) / 400 + ((ts->tm_year % 400) && ts->tm_year % 400 < EPOCH_YEAR % 400);

    days_from_epoch += YDAY[IS_LEAP_YEAR(ts->tm_year)][ts->tm_mon] + ts->tm_mday - 1;
    return days_from_epoch * SECS_IN_DAY + (ts->tm_hour * 60 + ts->tm_min) * 60 + ts->tm_sec;
}

struct tm* gmtime(time_t time, struct tm* ts)
{
    register time_t val = time;
    //first - decode time
    ts->tm_sec = val % 60;
    val /= 60;
    ts->tm_min = val % 60;
    val /= 60;
    ts->tm_hour = val % 24;
    val /= 24;

    //year between start date
    ts->tm_year = EPOCH_YEAR;
    while (val >= YEARSIZE(ts->tm_year))
    {
        val -= YEARSIZE(ts->tm_year);
        ts->tm_year++;
    }

    //decode month
    ts->tm_mon = 0;
    while (val >= MDAY[IS_LEAP_YEAR(ts->tm_year)][ts->tm_mon])
    {
        val -= MDAY[IS_LEAP_YEAR(ts->tm_year)][ts->tm_mon];
        ts->tm_mon++;
    }
    ts->tm_mday = val + 1;
    return ts;
}

