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
#define MAGIC_STREAM                                   0xf4eb741c
#define MAGIC_STREAM_HANDLE                            0x250b73c2
#define MAGIC_BLOCK                                    0x890f6c75
#define MAGIC_KIO                                      0x890f6c75

#define MAGIC_UNINITIALIZED                            0xcdcdcdcd
#define MAGIC_UNINITIALIZED_BYTE                       0xcd

#define	KERNEL_STACK_MAX                               0x200


#if !defined(LDS) && !defined(__ASSEMBLER__)

#include "kernel_config.h"
#include "../userspace/stdlib.h"
#include "../userspace/cc_macro.h"

#if (KERNEL_DEVELOPER_MODE)


/**
    \brief halts system macro
    \details only works, if \ref KERNEL_DEBUG is set
    \retval no return
*/
#define HALT()                                           {for (;;) {}}

/**
    \brief debug assertion
    \details only works, if \ref KERNEL_DEBUG is set.

    prints over debug console file name and line, caused assertion
    \param cond: assertion made if \b cond is \b false
    \retval no return if not \b cond, else none
*/
#define ASSERT(cond)                                    if (!(cond))    {printk("ASSERT at %s, line %d\n", __FILE__, __LINE__);    HALT();}

#else

#define HALT()
#define ASSERT(cond)

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
#if (KERNEL_DEVELOPER_MODE)
#define CHECK_MAGIC(obj, magic_value)    if ((obj)->magic != (magic_value)) {printk("INVALID MAGIC at %s, line %d\n", __FILE__, __LINE__);    HALT();}
#else
#define CHECK_MAGIC(obj, magic_value)    if ((obj)->magic != (magic_value)) {kprocess_error_current(ERROR_INVALID_MAGIC); return;}
#endif
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

#if (KERNEL_HANDLE_CHECKING)
#if (KERNEL_DEVELOPER_MODE)
#define CHECK_HANDLE(handle, size)     if ((HANDLE)(handle) == INVALID_HANDLE || (HANDLE)(handle) == ANY_HANDLE || (HANDLE)(handle) == KERNEL_HANDLE || \
                                          ((unsigned int)(handle) < (SRAM_BASE) + (KERNEL_GLOBAL_SIZE)) \
                                          || ((unsigned int)(handle) + (size) >= (SRAM_BASE) + (SRAM_SIZE))) \
                                            {printk("INVALID HANDLE at %s, line %d, caller process: %s\n", __FILE__, __LINE__, kprocess_name(kprocess_get_current()));    HALT();}
#else
#define CHECK_HANDLE(handle, size)     if ((HANDLE)(handle) == INVALID_HANDLE || (HANDLE)(handle) == ANY_HANDLE || (HANDLE)(handle) == KERNEL_HANDLE || \
                                          ((unsigned int)(handle) < (SRAM_BASE) + (KERNEL_GLOBAL_SIZE)) \
                                          || ((unsigned int)(handle) + (size) >= (SRAM_BASE) + (SRAM_SIZE))) \
                                            {kprocess_error_current(ERROR_INVALID_MAGIC); return;}
#endif
#else
#define CHECK_HANDLE(handle, size)
#endif //KERNEL_HANDLE_CHECKING

#if (KERNEL_ADDRESS_CHECKING)
#if (KERNEL_DEVELOPER_MODE)
#define CHECK_ADDRESS(process, address, sz)     if (!kprocess_check_address((KPROCESS*)(process), (address), (sz))) \
                                                    {printk("INVALID ADDRESS at %s, line %d, process: %s\n", __FILE__, __LINE__, kprocess_name((KPROCESS*)(process)));    HALT();}
#define CHECK_ADDRESS_READ(process, address, sz)     if (!kprocess_check_address_read((KPROCESS*)(process), (address), (sz))) \
                                                          {printk("INVALID READ ADDRESS at %s, line %d, process: %s\n", __FILE__, __LINE__, kprocess_name((KPROCESS*)(process)));    HALT();}
#else
#define CHECK_ADDRESS(process, address, sz)     if (!kprocess_check_address((KPROCESS*)(process), (address), (sz))) \
                                                     {kprocess_error_current(ERROR_ACCESS_DENIED); return;}
#define CHECK_ADDRESS_READ(process, address, sz)     if (!kprocess_check_address_read((KPROCESS*)(process), (address), (sz))) \
                                                          {kprocess_error_current(ERROR_ACCESS_DENIED); return;}
#endif //KERNEL_DEVELOPER_MODE
#else
#define CHECK_ADDRESS(process, address, size)
#define CHECK_ADDRESS_READ(process, address, size)
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
