#ifndef TIMER_H
#define TIMER_H

#include "types.h"
#include "time.h"
#include "cc_macro.h"
#include "svc.h"
#include "heap.h"

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


#define TIMER_MODE_PERIODIC                                                (1 << 0)

/**
    \brief get uptime up to 1us
    \param uptime pointer to structure, holding result value
    \retval none
*/
__STATIC_INLINE void get_uptime(TIME* uptime)
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
    \param app_handle application provided handle
    \retval HANDLE of timer, or invalid handle
*/
__STATIC_INLINE HANDLE timer_create(HANDLE app_handle)
{
    HANDLE handle;
    svc_call(SVC_TIMER_CREATE, (unsigned int)&handle, app_handle, 0);
    return handle;
}

/**
    \brief start soft timer
    \param timer soft timer handle
    \param time pointer to TIME structure
    \param mode soft timer. Mode TIMER_MODE_PERIODIC is only supported for now.
    \retval none.
*/
__STATIC_INLINE void timer_start(HANDLE timer, TIME* time, unsigned int mode)
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
    TIME time;
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
    TIME time;
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
    \brief destroy soft timer
    \param timer soft timer handle
    \retval none.
*/
__STATIC_INLINE void timer_destroy(HANDLE timer)
{
    svc_call(SVC_TIMER_DESTROY, (unsigned int)timer, 0, 0);
}

/** \} */ // end of timer group

#endif // TIMER_H
