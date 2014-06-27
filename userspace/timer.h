#ifndef TIMER_H
#define TIMER_H

#include "types.h"
#include "cc_macro.h"
#include "core/core.h"
#include "core/sys_calls.h"

/** \addtogroup timer timer
    interface to system timer
    \{
 */

//temporaily solution until msg will be ready
typedef struct {
    void (*start) (unsigned int us);
    void (*stop) (void);
    unsigned int (*elapsed) (void);
} CB_SVC_TIMER;


/**
    \brief get uptime up to 1us
    \param uptime pointer to structure, holding result value
    \retval none
*/
__STATIC_INLINE void get_uptime(TIME* uptime)
{
    sys_call(SVC_TIMER_GET_UPTIME, (unsigned int)uptime, 0, 0);
}

/**
    \brief produce kernel second pulse
    \param sb_svc_timer pointer to init structure
    \retval none
*/
__STATIC_INLINE void timer_init(CB_SVC_TIMER* cb_svc_timer)
{
    sys_call(SVC_TIMER_INIT, (unsigned int)cb_svc_timer, 0, 0);
}

/**
    \brief produce kernel second pulse
    \retval none
*/
__STATIC_INLINE void timer_second_pulse()
{
    sys_call(SVC_TIMER_SECOND_PULSE, 0, 0, 0);
}

/**
    \brief produce kernel signal of HPET timeout
    \retval none
*/
__STATIC_INLINE void timer_hpet_timeout()
{
    sys_call(SVC_TIMER_HPET_TIMEOUT, 0, 0, 0);
}

/** \} */ // end of timer group

#endif // TIMER_H
