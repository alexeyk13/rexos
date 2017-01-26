/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef SYSTIME_H
#define SYSTIME_H

#include "types.h"
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
    int (*lib_systime_compare)(SYSTIME*, SYSTIME*);
    void (*lib_systime_add)(SYSTIME*, SYSTIME*, SYSTIME*);
    void (*lib_systime_sub)(SYSTIME*, SYSTIME*, SYSTIME*);
    void (*lib_us_to_systime)(int, SYSTIME*);
    void (*lib_ms_to_systime)(int, SYSTIME*);
    int (*lib_systime_to_us)(SYSTIME*);
    int (*lib_systime_to_ms)(SYSTIME*);
    SYSTIME* (*lib_systime_elapsed)(SYSTIME*, SYSTIME*);
    unsigned int (*lib_systime_elapsed_ms)(SYSTIME*);
    unsigned int (*lib_systime_elapsed_us)(SYSTIME*);
} LIB_SYSTIME;

/**
    \brief compare time.
    \param from: time from
    \param to: time to
    \retval if "from" < "to", return 1, \n
    if "from" > "to", return -1, \n
    if "from" == "t", return 0
*/
int systime_compare(SYSTIME* from, SYSTIME* to);

/**
    \brief res = from + to
    \param from: time from
    \param to: time to
    \param res: result time. Safe to be same as "from" or "to"
    \retval none
*/
void systime_add(SYSTIME* from, SYSTIME* to, SYSTIME* res);

/**
    \brief res = to - from
    \param from: time from
    \param to: time to
    \param res: result time. Safe to be same as "from" or "to"
    \retval none
*/
void systime_sub(SYSTIME* from, SYSTIME* to, SYSTIME* res);

/**
    \brief convert time in microseconds to \ref SYSTIME structure
    \param us: microseconds
    \param time: pointer to allocated result \ref SYSTIME structure
    \retval none
*/
void us_to_systime(int us, SYSTIME* time);

/**
    \brief convert time in milliseconds to \ref SYSTIME structure
    \param ms: milliseconds
    \param time: pointer to allocated result \ref SYSTIME structure
    \retval none
*/
void ms_to_systime(int ms, SYSTIME* time);

/**
    \brief convert time from \ref SYSTIME structure to microseconds
    \param time: pointer to \ref SYSTIME structure. Maximal value: 0hr, 35 min, 46 seconds
    \retval time in microseconds
*/
int systime_to_us(SYSTIME* time);

/**
    \brief convert time from \ref SYSTIME structure to milliseconds
    \param time: pointer to \ref SYSTIME structure. Maximal value: 24days, 20hr, 31 min, 22 seconds
    \retval time in milliseconds
*/
int systime_to_ms(SYSTIME* time);

/**
    \brief time, elapsed between "from" and now
    \param from: pointer to provided structure, containing base \ref SYSTIME
    \param res: pointer to provided structure, containing result \ref SYSTIME
    \retval same as res parameter
*/
SYSTIME* systime_elapsed(SYSTIME* from, SYSTIME* res);

/**
    \brief time, elapsed between "from" and now in milliseconds
    \param from: pointer to provided structure, containing base \ref SYSTIME
    \retval elapsed time in milliseconds
*/
unsigned int systime_elapsed_ms(SYSTIME* from);

/**
    \brief time, elapsed between "from" and now in microseconds
    \param from: pointer to provided structure, containing base \ref SYSTIME
    \retval elapsed time in microseconds
*/
unsigned int systime_elapsed_us(SYSTIME* from);

/**
    \brief get uptime up to 1us
    \param uptime pointer to structure, holding result value
    \retval none
*/
void get_uptime(SYSTIME* uptime);

/**
    \brief produce kernel second pulse
    \param sb_svc_timer pointer to init structure
    \retval none
*/
void systime_hpet_setup(CB_SVC_TIMER* cb_svc_timer, void* cb_svc_timer_param);

/**
    \brief produce kernel second pulse
    \details Must be called in IRQ context, or call will fail
    \retval none
*/
void systime_second_pulse();

/**
    \brief produce kernel signal of HPET timeout
    \details Must be called in IRQ context, or call will fail
    \retval none
*/
void systime_hpet_timeout();

/**
    \brief create soft timer. Make sure, enalbed in kernel
    \param param: application provided param
    \param hal: HAL group
    \retval HANDLE of timer, or invalid handle
*/
HANDLE timer_create(unsigned int param, HAL hal);

/**
    \brief start soft timer
    \param timer soft timer handle
    \param time pointer to SYSTIME structure
    \retval none.
*/
void timer_start(HANDLE timer, SYSTIME* time);

/**
    \brief start soft timer. ISR version
    \param timer soft timer handle
    \param time pointer to SYSTIME structure
    \retval none.
*/
void timer_istart(HANDLE timer, SYSTIME* time);

/**
    \brief start soft timer in ms units
    \param timer soft timer handle
    \param time_ms time in ms units
    \retval none.
*/
void timer_start_ms(HANDLE timer, unsigned int time_ms);

/**
    \brief start soft timer in ms units. ISR version
    \param timer soft timer handle
    \param time_ms time in ms units
    \retval none.
*/
void timer_istart_ms(HANDLE timer, unsigned int time_ms);

/**
    \brief start soft timer in us units
    \param timer soft timer handle
    \param time_ms time in ms units
    \retval none.
*/
void timer_start_us(HANDLE timer, unsigned int time_us);

/**
    \brief start soft timer in us units. ISR version
    \param timer soft timer handle
    \param time_ms time in ms units
    \retval none.
*/
void timer_istart_us(HANDLE timer, unsigned int time_us);

/**
    \brief stop soft timer
    \param timer soft timer handle
    \param param: application provided param
    \param hal: HAL group
    \retval none.
*/
void timer_stop(HANDLE timer, unsigned int param, HAL hal);

/**
    \brief stop soft timer. ISR version
    \details Remember, IPC_TIMEOUT can happened before stop
    \param timer soft timer handle
    \retval none.
*/
void timer_istop(HANDLE timer);

/**
    \brief destroy soft timer
    \param timer soft timer handle
    \retval none.
*/
void timer_destroy(HANDLE timer);

/** \} */ // end of systime group

#endif // SYSTIME_H
