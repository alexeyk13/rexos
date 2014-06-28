/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef DBG_H
#define DBG_H

/** \addtogroup debug debug routines
    \{
    debug routines
 */

/*
        dbg.h: debug-specific
  */


#define MAGIC_TIMER                                    0xbecafcf5
#define MAGIC_PROCESS                                  0x7de32076
#define MAGIC_MUTEX                                    0xd0cc6e26
#define MAGIC_EVENT                                    0x57e198c7
#define MAGIC_SEM                                      0xabfd92d9

#define MAGIC_UNINITIALIZED                            0xcdcdcdcd
#define MAGIC_UNINITIALIZED_BYTE                       0xcd

#if !defined(LDS) && !defined(__ASSEMBLER__)

#include "kernel_config.h"
#include "../userspace/types.h"
#include "../userspace/cc_macro.h"
#include "../lib/printf.h"
#include "../mod/console/console.h"

__STATIC_INLINE void dbg_write(const char* const buf, int size)
{
        sys_call(SVC_DBG_WRITE, (unsigned int)buf, (unsigned int)size, 0);
}

__STATIC_INLINE void dbg_push()
{
    sys_call(SVC_DBG_PUSH, 0, 0, 0);
}


#if (KERNEL_DEBUG)


/**
    \brief halts system macro
    \details only works, if \ref KERNEL_DEBUG is set
    \retval no return
*/
#define HALT()                                           {dbg_push(); for (;;) {}}

/**
    \brief debug assertion
    \details only works, if \ref KERNEL_DEBUG is set.

    prints over debug console file name and line, caused assertion
    \param cond: assertion made if \b cond is \b false
    \retval no return if not \b cond, else none
*/
#define ASSERT(cond)                                    if (!(cond))    {printf("ASSERT at %s, line %d\n\r", __FILE__, __LINE__);    HALT();}

#else

#define HALT()
#define ASSERT(cond)

#endif

#if (KERNEL_CHECK_CONTEXT)
/**
    \brief context assertion
    \details only works, if \ref KERNEL_DEBUG and \ref KERNEL_CHECK_CONTEXT are set.

    prints over debug console file name and line, caused assertion
    \param value: \ref CONTEXT to check
    \retval no return if wrong context, else none
*/
#define CHECK_CONTEXT(value)                        if ((get_context() & (value)) == 0) {printf("WRONG CONTEXT at %s, line %d\n\r", __FILE__, __LINE__);    HALT();}
#else
#define CHECK_CONTEXT(value)
#endif

#if (KERNEL_MARKS)

/**
    \brief check, if object mark is right (object is valid)
    \details only works, if \ref KERNEL_DEBUG and \ref KERNEL_MARKS are set.
    \param obj: object to check
    \param magic_value: value to set. check \ref magic.h for details
    \param name: object text to display in case of wrong magic
    \retval no return if wrong magic, else none
*/
#define CHECK_MAGIC(obj, magic_value)    if ((obj)->magic != (magic_value)) {printf("INVALID MAGIC at %s, line %d\n\r", __FILE__, __LINE__);    HALT();}
/**
    \brief apply object magic on object creation
    \details only works, if \ref KERNEL_DEBUG and \ref KERNEL_MARKS are set.
    \param obj: object to check
    \param magic_value: value to set. check \ref magic.h for details
    \retval none
*/
#define DO_MAGIC(obj, magic_value)                    (obj)->magic = (magic_value)
/**
    \brief this macro must be put in object structure
*/
#define MAGIC                                          unsigned int magic

#else
#define CHECK_MAGIC(obj, magic_vale)
#define DO_MAGIC(obj, magic_value)
#define MAGIC
#endif

/** \} */ // end of debug group

//TODO: move to core dbg
#if (KERNEL_PROFILING)

/** \addtogroup profiling profiling
  \ref KERNEL_PROFILING option should be set to 1
    \{
 */

/**
    \brief thread switch test
    \details simulate thread switching. This function can be used for
    perfomance measurement
    \retval none
*/
__STATIC_INLINE void thread_switch_test()
{
    sys_call(SVC_PROCESS_SWITCH_TEST, 0, 0, 0);
}

/**
    \brief thread statistics
    \details print statistics over debug console for all active threads:
    - names
    - priority, active priority (can be temporally raised for sync objects)
    - stack usage: current/max/defined
    \retval none
*/
__STATIC_INLINE void thread_stat()
{
    sys_call(SVC_PROCESS_STAT, 0, 0, 0);
}

/**
    \brief system stacks statistics
    \details list of stacks depends on architecture.
    print statistics over debug console for all active threads:
    - names
    - stack usage: current/max/defined
    \retval none
*/
__STATIC_INLINE void stack_stat()
{
    sys_call(SVC_STACK_STAT, 0, 0, 0);
}

/** \} */ // end of profiling group

#endif //(KERNEL_PROFILING)

#endif // !defined(LDS) && !defined(__ASSEMBLER__)


#endif // DBG_H
