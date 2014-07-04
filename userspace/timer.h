#ifndef TIMER_H
#define TIMER_H

#include "lib/types.h"
#include "cc_macro.h"
#include "svc.h"

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
    void (*start) (unsigned int us);                                //!< start HPET timer with us value timeout
    void (*stop) (void);                                            //!< stop HPET timer
    unsigned int (*elapsed) (void);                                 //!< return elapsed time from last start in us
} CB_SVC_TIMER;


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
__STATIC_INLINE void timer_setup(CB_SVC_TIMER* cb_svc_timer)
{
    svc_call(SVC_TIMER_SETUP, (unsigned int)cb_svc_timer, 0, 0);
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

/** \} */ // end of timer group

#endif // TIMER_H
