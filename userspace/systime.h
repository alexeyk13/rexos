/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef SYSTIME_H
#define SYSTIME_H

#include "cc_macro.h"
#include "lib.h"
#include "svc.h"
#include "heap.h"
#include "ipc.h"

/** \addtogroup timer timer
    interface to system timer
    \{
 */

/** Callback, provided to kernel, for HPET functionality
 *  It's the only kernel exception of calling something directly outside of
 *  kernel. This exception is required for realtime functionality. Please return from
 *  callbacks ASAP and don't make any heavy stack load, cause you will use here kernel stack
  */
typedef struct {
    void (*start) (unsigned int us, void*);                                //!< start HPET timer with us value timeout
    void (*stop) (void*);                                                  //!< stop HPET timer
    unsigned int (*elapsed) (void*);                                       //!< return elapsed time from last start in us
} CB_SVC_TIMER;

/**
    \brief structure for holding system time units
*/
typedef struct _SYSTIME{
    unsigned int sec;                                                       //!< seconds
    unsigned int usec;                                                      //!< microseconds
}SYSTIME;

typedef struct {
    int (*time_compare)(SYSTIME*, SYSTIME*);
    void (*time_add)(SYSTIME*, SYSTIME*, SYSTIME*);
    void (*time_sub)(SYSTIME*, SYSTIME*, SYSTIME*);
    void (*us_to_time)(int, SYSTIME*);
    void (*ms_to_time)(int, SYSTIME*);
    int (*time_to_us)(SYSTIME*);
    int (*time_to_ms)(SYSTIME*);
    SYSTIME* (*time_elapsed)(SYSTIME*, SYSTIME*);
    unsigned int (*time_elapsed_ms)(SYSTIME*);
    unsigned int (*time_elapsed_us)(SYSTIME*);
} LIB_TIME;

#define TIMER_MODE_PERIODIC                                                (1 << 0)

/**
    \brief compare time.
    \param from: time from
    \param to: time to
    \retval if "from" < "to", return 1, \n
    if "from" > "to", return -1, \n
    if "from" == "t", return 0
*/
__STATIC_INLINE int time_compare(SYSTIME* from, SYSTIME* to)
{
    return ((const LIB_TIME*)__GLOBAL->lib[LIB_ID_TIME])->time_compare(from, to);
}

/**
    \brief res = from + to
    \param from: time from
    \param to: time to
    \param res: result time. Safe to be same as "from" or "to"
    \retval none
*/
__STATIC_INLINE void time_add(SYSTIME* from, SYSTIME* to, SYSTIME* res)
{
    ((const LIB_TIME*)__GLOBAL->lib[LIB_ID_TIME])->time_add(from, to, res);
}

/**
    \brief res = to - from
    \param from: time from
    \param to: time to
    \param res: result time. Safe to be same as "from" or "to"
    \retval none
*/
__STATIC_INLINE void time_sub(SYSTIME* from, SYSTIME* to, SYSTIME* res)
{
    ((const LIB_TIME*)__GLOBAL->lib[LIB_ID_TIME])->time_sub(from, to, res);
}

/**
    \brief convert time in microseconds to \ref SYSTIME structure
    \param us: microseconds
    \param time: pointer to allocated result \ref SYSTIME structure
    \retval none
*/
__STATIC_INLINE void us_to_time(int us, SYSTIME* time)
{
    ((const LIB_TIME*)__GLOBAL->lib[LIB_ID_TIME])->us_to_time(us, time);
}

/**
    \brief convert time in milliseconds to \ref SYSTIME structure
    \param ms: milliseconds
    \param time: pointer to allocated result \ref SYSTIME structure
    \retval none
*/
__STATIC_INLINE void ms_to_time(int ms, SYSTIME* time)
{
    ((const LIB_TIME*)__GLOBAL->lib[LIB_ID_TIME])->ms_to_time(ms, time);
}

/**
    \brief convert time from \ref SYSTIME structure to microseconds
    \param time: pointer to \ref SYSTIME structure. Maximal value: 0hr, 35 min, 46 seconds
    \retval time in microseconds
*/
__STATIC_INLINE int time_to_us(SYSTIME* time)
{
    return ((const LIB_TIME*)__GLOBAL->lib[LIB_ID_TIME])->time_to_us(time);
}

/**
    \brief convert time from \ref SYSTIME structure to milliseconds
    \param time: pointer to \ref SYSTIME structure. Maximal value: 24days, 20hr, 31 min, 22 seconds
    \retval time in milliseconds
*/
__STATIC_INLINE int time_to_ms(SYSTIME* time)
{
    return ((const LIB_TIME*)__GLOBAL->lib[LIB_ID_TIME])->time_to_ms(time);
}

/**
    \brief time, elapsed between "from" and now
    \param from: pointer to provided structure, containing base \ref SYSTIME
    \param res: pointer to provided structure, containing result \ref SYSTIME
    \retval same as res parameter
*/
__STATIC_INLINE SYSTIME* time_elapsed(SYSTIME* from, SYSTIME* res)
{
    return ((const LIB_TIME*)__GLOBAL->lib[LIB_ID_TIME])->time_elapsed(from, res);
}

/**
    \brief time, elapsed between "from" and now in milliseconds
    \param from: pointer to provided structure, containing base \ref SYSTIME
    \retval elapsed time in milliseconds
*/
__STATIC_INLINE unsigned int time_elapsed_ms(SYSTIME* from)
{
    return ((const LIB_TIME*)__GLOBAL->lib[LIB_ID_TIME])->time_elapsed_ms(from);
}

/**
    \brief time, elapsed between "from" and now in microseconds
    \param from: pointer to provided structure, containing base \ref SYSTIME
    \retval elapsed time in microseconds
*/
__STATIC_INLINE unsigned int time_elapsed_us(SYSTIME* from)
{
    return ((const LIB_TIME*)__GLOBAL->lib[LIB_ID_TIME])->time_elapsed_us(from);
}

/**
    \brief get uptime up to 1us
    \param uptime pointer to structure, holding result value
    \retval none
*/
__STATIC_INLINE void get_uptime(SYSTIME* uptime)
{
    svc_call(SVC_TIMER_GET_UPTIME, (unsigned int)uptime, 0, 0);
}

/**
    \brief produce kernel second pulse
    \param sb_svc_timer pointer to init structure
    \retval none
*/
__STATIC_INLINE void timer_setup(CB_SVC_TIMER* cb_svc_timer, void* cb_svc_timer_param)
{
    svc_call(SVC_TIMER_SETUP, (unsigned int)cb_svc_timer, (unsigned int)cb_svc_timer_param, 0);
}

/**
    \brief produce kernel second pulse
    \details Must be called in IRQ context, or call will fail
    \retval none
*/
__STATIC_INLINE void timer_second_pulse()
{
    __GLOBAL->svc_irq(SVC_TIMER_SECOND_PULSE, 0, 0, 0);
}

/**
    \brief produce kernel signal of HPET timeout
    \details Must be called in IRQ context, or call will fail
    \retval none
*/
__STATIC_INLINE void timer_hpet_timeout()
{
    __GLOBAL->svc_irq(SVC_TIMER_HPET_TIMEOUT, 0, 0, 0);
}


/**
    \brief create soft timer. Make sure, enalbed in kernel
    \param param: application provided param
    \param hal: HAL group
    \retval HANDLE of timer, or invalid handle
*/
__STATIC_INLINE HANDLE timer_create(unsigned int param, HAL hal)
{
    HANDLE handle;
    svc_call(SVC_TIMER_CREATE, (unsigned int)&handle, param, (unsigned int)hal);
    return handle;
}

/**
    \brief start soft timer
    \param timer soft timer handle
    \param time pointer to SYSTIME structure
    \param mode soft timer. Mode TIMER_MODE_PERIODIC is only supported for now.
    \retval none.
*/
__STATIC_INLINE void timer_start(HANDLE timer, SYSTIME* time, unsigned int mode)
{
    svc_call(SVC_TIMER_START, (unsigned int)timer, (unsigned int)time, mode);
}

/**
    \brief start soft timer in ms units
    \param timer soft timer handle
    \param time_ms time in ms units
    \param mode soft timer. Mode TIMER_MODE_PERIODIC is only supported for now.
    \retval none.
*/
__STATIC_INLINE void timer_start_ms(HANDLE timer, unsigned int time_ms, unsigned int mode)
{
    SYSTIME time;
    ms_to_time(time_ms, &time);
    timer_start(timer, &time, mode);
}

/**
    \brief start soft timer in us units
    \param timer soft timer handle
    \param time_ms time in ms units
    \param mode soft timer. Mode TIMER_MODE_PERIODIC is only supported for now.
    \retval none.
*/
__STATIC_INLINE void timer_start_us(HANDLE timer, unsigned int time_us, unsigned int mode)
{
    SYSTIME time;
    us_to_time(time_us, &time);
    timer_start(timer, &time, mode);
}

/**
    \brief stop soft timer
    \param timer soft timer handle
    \retval none.
*/
__STATIC_INLINE void timer_stop(HANDLE timer)
{
    svc_call(SVC_TIMER_STOP, (unsigned int)timer, 0, 0);
}

/**
    \brief stop soft timer, isr version
    \param timer soft timer handle
    \retval none.
*/
__STATIC_INLINE void timer_istop(HANDLE timer)
{
    __GLOBAL->svc_irq(SVC_TIMER_STOP, (unsigned int)timer, 0, 0);
}

/**
    \brief destroy soft timer
    \param timer soft timer handle
    \retval none.
*/
__STATIC_INLINE void timer_destroy(HANDLE timer)
{
    svc_call(SVC_TIMER_DESTROY, (unsigned int)timer, 0, 0);
}

/** \} */ // end of systime group

#endif // SYSTIME_H
