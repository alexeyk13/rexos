/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
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
#define MAGIC_STREAM                                   0xf4eb741c
#define MAGIC_STREAM_HANDLE                            0x250b73c2
#define MAGIC_KIO                                      0x890f6c75
#define MAGIC_HEAP                                     0xd0cc6e26

#define MAGIC_UNINITIALIZED                            0xcdcdcdcd
#define MAGIC_UNINITIALIZED_BYTE                       0xcd

#define	KERNEL_STACK_MAX                               0x200


#if !defined(LDS) && !defined(__ASSEMBLER__)

#include "kernel_config.h"
#include "../userspace/stdlib.h"
#include "../userspace/cc_macro.h"

/**
    \brief halts system macro
    \details only works, if \ref KERNEL_DEBUG is set
    \retval no return
*/
#define HALT()                                           {for (;;) {}}

#if (KERNEL_MARKS)

/**
    \brief check, if object mark is right (object is valid)
    \details only works, if \ref KERNEL_DEBUG and \ref KERNEL_MARKS are set.
    \param obj: object to check
    \param magic_value: value to set. check \ref magic.h for details
    \param name: object text to display in case of wrong magic
    \retval no return if wrong magic, else none
*/
#define CHECK_MAGIC(obj, magic_value)    if (((HANDLE)(obj) != KERNEL_HANDLE) && (((HANDLE)(obj) == INVALID_HANDLE) || ((HANDLE)(obj) == ANY_HANDLE) || \
                                         ((obj)->magic != (magic_value)))) {printk("INVALID MAGIC at %s, line %d\n", __FILE__, __LINE__);    panic();}

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

#define CLEAR_MAGIC(obj)                              (obj)->magic = 0
/**
    \brief this macro must be put in object structure
*/
#define MAGIC                                         unsigned int magic

#else
#define CHECK_MAGIC(obj, magic_value)
#define DO_MAGIC(obj, magic_value)
#define CLEAR_MAGIC(obj)
#define MAGIC
#endif //KERNEL_MARKS

#if (KERNEL_ADDRESS_CHECKING)
#define CHECK_ADDRESS(process, address, sz)     if (!kprocess_check_address((process), (address), (sz))) \
                                                    {printk("INVALID ADDRESS at %s, line %d, process: %s\n", __FILE__, __LINE__, kprocess_name((HANDLE)(process)));    panic();}
#define CHECK_IO_ADDRESS(process, ipc)          if (((IPC*)(ipc))->cmd & HAL_IO_FLAG) { \
                                                    if (!kprocess_check_address((process), (HANDLE*)(ipc)->param2, sizeof(HANDLE))) \
                                                        {printk("INVALID IO ADDRESS at %s, line %d, process: %s\n", __FILE__, __LINE__, kprocess_name((HANDLE)(process)));    panic();}
#else
#define CHECK_ADDRESS(process, address, size)
#define CHECK_IO_ADDRESS(process, ipc)
#endif //KERNEL_ADDRESS_CHECKING

/**
    \brief format string, using kernel stdio as handler
    \param fmt: format (see global description)
    \param ...: list of arguments
    \retval none
*/
void printk(const char *const fmt, ...);

/**
    \brief print minidump
    \param addr: starting address
    \param size: size of dump
    \retval none
*/
void dump(unsigned int addr, unsigned int size);


#endif // !defined(LDS) && !defined(__ASSEMBLER__)

#endif // DBG_H
