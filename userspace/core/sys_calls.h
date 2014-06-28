/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef SYS_CALLS_H
#define SYS_CALLS_H

/*
    List of all SVC
    Hight eight bit is SVC group. All SVC are 16 bit long.
 */

typedef enum {
    SVC_PROCESS = 0x0,
    SVC_PROCESS_CREATE,
    SVC_PROCESS_UNFREEZE,
    SVC_PROCESS_FREEZE,
    SVC_PROCESS_GET_PRIORITY,
    SVC_PROCESS_SET_PRIORITY,
    SVC_PROCESS_DESTROY,
    SVC_PROCESS_SLEEP,
    //profiling
    SVC_PROCESS_SWITCH_TEST,
    SVC_PROCESS_STAT,
    SVC_STACK_STAT,

    SVC_MUTEX = 0x100,
    SVC_MUTEX_CREATE,
    SVC_MUTEX_LOCK,
    SVC_MUTEX_UNLOCK,
    SVC_MUTEX_DESTROY,

    SVC_EVENT = 0x200,
    SVC_EVENT_CREATE,
    SVC_EVENT_PULSE,
    SVC_EVENT_SET,
    SVC_EVENT_IS_SET,
    SVC_EVENT_CLEAR,
    SVC_EVENT_WAIT,
    SVC_EVENT_DESTROY,

    SVC_SEM = 0x300,
    SVC_SEM_CREATE,
    SVC_SEM_WAIT,
    SVC_SEM_SIGNAL,
    SVC_SEM_DESTROY,

    SVC_MEM = 0x400,
    //profiling
    SVC_MEM_STAT,

    SVC_TIMER = 0x500,
    SVC_TIMER_GET_UPTIME,
    SVC_TIMER_SECOND_PULSE,
    SVC_TIMER_HPET_TIMEOUT,
    SVC_TIMER_SETUP,


    SVC_DBG = 0x600,
    SVC_DBG_WRITE,
    SVC_DBG_PUSH
}SYS_CALLS;

#endif // SYS_CALLS_H
