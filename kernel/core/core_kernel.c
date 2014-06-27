/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "core_kernel.h"
#include "kernel_config.h"
#include "../dbg.h"

/** \addtogroup core_kernel kernel part of core-related functionality
    \{
 */

/**
    \brief default handler for irq
    \details This function should be called on unhandled irq exception
    \param vector: vector number
    \retval none
*/
void default_irq_handler(int vector)
{
#if (KERNEL_DEBUG)
    printf("Warning: irq vector %d without handler\n\r", vector);
#endif
}

/**
    \brief kernel panic. reset core or halt, depending on kernel_config
    \param none
    \retval none
*/
void panic()
{
#if (KERNEL_DEBUG)
    printf("Kernel panic\n\r");
    dump(SRAM_BASE, 0x200);
    dbg_push();
#endif
#if (KERNEL_HALT_ON_FATAL_ERROR)
    HALT();
#else
    reset();
#endif //KERNEL_HALT_ON_FATAL_ERROR
}

/** \} */ // end of core_kernel group
