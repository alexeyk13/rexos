/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "lib_time.h"
#include "../userspace/systime.h"
#include "../userspace/types.h"

#define USEC_1S                            1000000ul
#define USEC_1MS                            1000ul
#define MSEC_1S                            1000ul

#define MAX_US_DELTA                        2146
#define MAX_MS_DELTA                        2147482


static int __time_compare(SYSTIME* from, SYSTIME* to)
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

static void __time_add(SYSTIME* from, SYSTIME* to, SYSTIME* res)
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

static void __time_sub(SYSTIME* from, SYSTIME* to, SYSTIME* res)
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

static void __us_to_time(int us, SYSTIME* time)
{
    time->sec = us / USEC_1S;
    time->usec = us % USEC_1S;
}

static void __ms_to_time(int ms, SYSTIME* time)
{
    time->sec = ms / MSEC_1S;
    time->usec = (ms % MSEC_1S) * USEC_1MS;
}

static int __time_to_us(SYSTIME* time)
{
    return time->sec <= MAX_US_DELTA ? (int)(time->sec * USEC_1S + time->usec) : (int)(MAX_US_DELTA * USEC_1S);
}

static int __time_to_ms(SYSTIME* time)
{
    return time->sec <= MAX_MS_DELTA ? (int)(time->sec * MSEC_1S + time->usec / USEC_1MS) : (int)(MAX_MS_DELTA * MSEC_1S);
}

static SYSTIME* __time_elapsed(SYSTIME* from, SYSTIME* res)
{
    SYSTIME to;
    get_uptime(&to);
    __time_sub(from, &to, res);
    return res;
}

static unsigned int __time_elapsed_ms(SYSTIME* from)
{
    SYSTIME to;
    get_uptime(&to);
    __time_sub(from, &to, &to);
    return __time_to_ms(&to);
}

static unsigned int __time_elapsed_us(SYSTIME* from)
{
    SYSTIME to;
    get_uptime(&to);
    __time_sub(from, &to, &to);
    return __time_to_us(&to);
}

const LIB_TIME __LIB_TIME = {
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
