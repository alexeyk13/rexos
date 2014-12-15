/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "lib_time.h"
#include "../userspace/timer.h"
#include "../userspace/types.h"

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

#define USEC_1S                            1000000ul
#define USEC_1MS                            1000ul
#define MSEC_1S                            1000ul

#define MAX_US_DELTA                        2146
#define MAX_MS_DELTA                        2147482

static time_t __mktime(struct tm* ts)
{
    register time_t days_from_epoch;
    days_from_epoch = (ts->tm_year - EPOCH_YEAR) * 365;
    days_from_epoch += (ts->tm_year - EPOCH_YEAR) / 4 + ((ts->tm_year % 4) && ts->tm_year % 4 < EPOCH_YEAR % 4);
    days_from_epoch -= (ts->tm_year - EPOCH_YEAR) / 100 + ((ts->tm_year % 100) && ts->tm_year % 100 < EPOCH_YEAR % 100);
    days_from_epoch += (ts->tm_year - EPOCH_YEAR) / 400 + ((ts->tm_year % 400) && ts->tm_year % 400 < EPOCH_YEAR % 400);

    days_from_epoch += YDAY[IS_LEAP_YEAR(ts->tm_year)][ts->tm_mon] + ts->tm_mday - 1;
    return days_from_epoch * SECS_IN_DAY + (ts->tm_hour * 60 + ts->tm_min) * 60 + ts->tm_sec;
}

static struct tm* __gmtime(time_t time, struct tm* ts)
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

static int __time_compare(TIME* from, TIME* to)
{
    int res = -1;
    if (to->sec > from->sec)
        res = 1;
    else if (to->sec == from->sec)
    {
        if (to->usec > from->usec)
            res = 1;
        else if (to->usec == from->usec)
            res = 0;
        //else res = -1
    }//else res = -1
    return res;
}

static void __time_add(TIME* from, TIME* to, TIME* res)
{
    res->sec = to->sec + from->sec;
    res->usec = to->usec + from->usec;
    //loan
    while (res->usec >= USEC_1S)
    {
        ++res->sec;
        res->usec -= USEC_1S;
    }
}

static void __time_sub(TIME* from, TIME* to, TIME* res)
{
    if (__time_compare(from, to) > 0)
    {
        res->sec = to->sec - from->sec;
        //borrow
        if (to->usec >= from->usec)
            res->usec = to->usec - from->usec;
        else
        {
            res->usec = USEC_1S - (from->usec - to->usec);
            --res->sec;
        }
    }
    else
        res->sec = res->usec = 0;
}

static void __us_to_time(int us, TIME* time)
{
    time->sec = us / USEC_1S;
    time->usec = us % USEC_1S;
}

static void __ms_to_time(int ms, TIME* time)
{
    time->sec = ms / MSEC_1S;
    time->usec = (ms % MSEC_1S) * USEC_1MS;
}

static int __time_to_us(TIME* time)
{
    return time->sec <= MAX_US_DELTA ? (int)(time->sec * USEC_1S + time->usec) : (int)(MAX_US_DELTA * USEC_1S);
}

static int __time_to_ms(TIME* time)
{
    return time->sec <= MAX_MS_DELTA ? (int)(time->sec * MSEC_1S + time->usec / USEC_1MS) : (int)(MAX_MS_DELTA * MSEC_1S);
}

static TIME* __time_elapsed(TIME* from, TIME* res)
{
    TIME to;
    get_uptime(&to);
    __time_sub(from, &to, res);
    return res;
}

static unsigned int __time_elapsed_ms(TIME* from)
{
    TIME to;
    get_uptime(&to);
    __time_sub(from, &to, &to);
    return __time_to_ms(&to);
}

static unsigned int __time_elapsed_us(TIME* from)
{
    TIME to;
    get_uptime(&to);
    __time_sub(from, &to, &to);
    return __time_to_us(&to);
}

const LIB_TIME __LIB_TIME = {
    __mktime,
    __gmtime,
    __time_compare,
    __time_add,
    __time_sub,
    __us_to_time,
    __ms_to_time,
    __time_to_us,
    __time_to_ms,
    __time_elapsed,
    __time_elapsed_ms,
    __time_elapsed_us
};
