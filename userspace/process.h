/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef PROCESS_H
#define PROCESS_H

/** \addtogroup process process
    process is the main object in kernel.

    process can be in one of the following states:
    - active
    - waiting (for sync object)
    - frozen
    - waiting frozen

    After creation process is in the frozen state

    Process is also specified by:
    - it's name
    - priority
    - stack size
    \{
 */

#include "core/core.h"
#include "systime.h"
#include "rb.h"

#define PROCESS_FLAGS_ACTIVE                                     (1 << 0)
#define PROCESS_FLAGS_WAITING                                    (1 << 1)

#define PROCESS_MODE_MASK                                        0x3
#define PROCESS_MODE_FROZEN                                      (0)
#define PROCESS_MODE_ACTIVE                                      (PROCESS_FLAGS_ACTIVE)
#define PROCESS_MODE_WAITING_FROZEN                              (PROCESS_FLAGS_WAITING)
#define PROCESS_MODE_WAITING                                     (PROCESS_FLAGS_WAITING | PROCESS_FLAGS_ACTIVE)

#define PROCESS_SYNC_MASK                                        (0xf << 4)

typedef enum {
    PROCESS_SYNC_TIMER_ONLY =    (0x0 << 4),
    PROCESS_SYNC_IPC =           (0x4 << 4),
    PROCESS_SYNC_STREAM =        (0x5 << 4)
}PROCESS_SYNC_TYPE;

#define REX_FLAG_PERSISTENT_NAME                                 (1 << 24)

typedef struct {
    const char* name;
    unsigned int size;
    unsigned int priority;
    unsigned int flags;
    void (*fn) (void);
}REX;

typedef struct {
    int error;
    POOL pool;
    //stdout/stdin handle. System specific
    HANDLE stdout, stdin;
    const char* name;
    RB ipcs;
    //follow:
    //IPC queue
    //name holder (if not persistent)
} PROCESS;

// will be aligned to pass MPU requirements
typedef struct {
    PROCESS* process;
    void (*svc_irq)(unsigned int, unsigned int, unsigned int, unsigned int);
    const void** lib;
} GLOBAL;

#define __GLOBAL                                                 ((GLOBAL*)(SRAM_BASE))

#define __PROCESS                                                ((PROCESS*)(((GLOBAL*)(SRAM_BASE))->process))

/**
    \brief creates process object. By default, process is frozen after creation
    \param rex: pointer to process header
    \retval process HANDLE on success, or INVALID_HANDLE on failure
*/
HANDLE process_create(const REX* rex);

/**
    \brief get current process
    \param rex: pointer to process header
    \retval process HANDLE on success, or INVALID_HANDLE on failure
*/
HANDLE process_get_current();

/**
    \brief get name of current process
    \retval process name
*/
const char* process_name();

/**
    \brief get process last error
    \retval error code
*/
int get_last_error();

/**
    \brief set error code for current process
    \param error: error code to set
    \retval none
*/
void error(int error);

/**
    \brief get current process. Isr version
    \param rex: pointer to process header
    \retval process HANDLE on success, or INVALID_HANDLE on failure
*/
HANDLE process_iget_current();

/**
    \brief get process flags
    \param process: handle of created process
    \retval process flags
*/
unsigned int process_get_flags(HANDLE process);

/**
    \brief set process flags
    \param process: handle of created process
    \param flags: set of flags. Only PROCESS_FLAGS_ACTIVE is supported
    \retval process flags
*/
void process_set_flags(HANDLE process, unsigned int flags);

/**
    \brief unfreeze process
    \param process: handle of created process
    \retval none
*/
void process_unfreeze(HANDLE process);

/**
    \brief freeze process
    \param process: handle of created process
    \retval none
*/
void process_freeze(HANDLE process);

/**
    \brief get priority
    \param process: handle of created process
    \retval process base priority
*/
unsigned int process_get_priority(HANDLE process);

/**
    \brief get current process priority
    \param process: handle of created process
    \retval process base priority
*/
unsigned int process_get_current_priority();

/**
    \brief set priority
    \param process: handle of created process
    \param priority: new priority. 0 - highest, init -1 - lowest. Cannot be called for init process.
    \retval none
*/
void process_set_priority(HANDLE process, unsigned int priority);

/**
    \brief set currently running process priority
    \param priority: new priority. 0 - highest, init -1 - lowest. Cannot be called for init process.
    \retval none
*/
void process_set_current_priority(unsigned int priority);

/**
    \brief destroys process
    \param process: previously created process
    \retval none
*/
void process_destroy(HANDLE process);

/**
    \brief destroys current process
    \details this function should be called instead of leaving process function
    \retval none
*/
void process_exit();

/**
    \brief put current process in waiting state
    \param time: pointer to SYSTIME structure
    \retval none
*/
void sleep(SYSTIME* time);

/**
    \brief put current process in waiting state
    \param ms: time to sleep in milliseconds
    \retval none
*/
void sleep_ms(unsigned int ms);

/**
    \brief put current process in waiting state
    \param us: time to sleep in microseconds
    \retval none
*/
void sleep_us(unsigned int us);

/**
    \brief process switch test. Only for kernel debug/perfomance testing reasons
    \param us: time to sleep in microseconds
    \retval none
*/
void process_switch_test();

/**
    \brief process info for all processes. Only for kernel debug/perfomance testing reasons
    \retval none
*/
void process_info();

/** \} */ // end of process group

#endif // PROCESS_H
