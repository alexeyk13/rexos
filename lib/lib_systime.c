/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "lib_systime.h"
#include "../userspace/systime.h"
#include "../userspace/types.h"

#define USEC_1S                            1000000ul
#define USEC_1MS                            1000ul
#define MSEC_1S                            1000ul

#define MAX_US_DELTA                        2146
#define MAX_MS_DELTA                        2147482


static int lib_systime_compare(SYSTIME* from, SYSTIME* to)
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

static void lib_systime_add(SYSTIME* from, SYSTIME* to, SYSTIME* res)
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

static void lib_systime_sub(SYSTIME* from, SYSTIME* to, SYSTIME* res)
{
    if (lib_systime_compare(from, to) > 0)
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

static void lib_us_to_systime(int us, SYSTIME* time)
{
    time->sec = us / USEC_1S;
    time->usec = us % USEC_1S;
}

static void lib_ms_to_systime(int ms, SYSTIME* time)
{
    time->sec = ms / MSEC_1S;
    time->usec = (ms % MSEC_1S) * USEC_1MS;
}

static int lib_systime_to_us(SYSTIME* time)
{
    return time->sec <= MAX_US_DELTA ? (int)(time->sec * USEC_1S + time->usec) : (int)(MAX_US_DELTA * USEC_1S);
}

static int lib_systime_to_ms(SYSTIME* time)
{
    return time->sec <= MAX_MS_DELTA ? (int)(time->sec * MSEC_1S + time->usec / USEC_1MS) : (int)(MAX_MS_DELTA * MSEC_1S);
}

static SYSTIME* lib_systime_elapsed(SYSTIME* from, SYSTIME* res)
{
    SYSTIME to;
    get_uptime(&to);
    lib_systime_sub(from, &to, res);
    return res;
}

static unsigned int lib_systime_elapsed_ms(SYSTIME* from)
{
    SYSTIME to;
    get_uptime(&to);
    lib_systime_sub(from, &to, &to);
    return lib_systime_to_ms(&to);
}

static unsigned int lib_systime_elapsed_us(SYSTIME* from)
{
    SYSTIME to;
    get_uptime(&to);
    lib_systime_sub(from, &to, &to);
    return lib_systime_to_us(&to);
}

const LIB_SYSTIME __LIB_SYSTIME = {
    lib_systime_compare,
    lib_systime_add,
    lib_systime_sub,
    lib_us_to_systime,
    lib_ms_to_systime,
    lib_systime_to_us,
    lib_systime_to_ms,
    lib_systime_elapsed,
    lib_systime_elapsed_ms,
    lib_systime_elapsed_us
};
