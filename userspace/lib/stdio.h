#ifndef STDIO_H
#define STDIO_H

#include "lib.h"

/** \addtogroup stdio embedded uStdio
 */

/**
    \brief format string, using \ref dbg_write as handler
    \param fmt: format (see global description)
    \param ...: list of arguments
    \retval none
*/
void printf(const char *const fmt, ...);

/**
    \brief format string to \b str
    \param str: resulting string
    \param fmt: format (see global description)
    \param ...: list of arguments
    \retval none
*/
void sprintf(char* str, const char *const fmt, ...);

/** \} */ // end of stdlib group


#endif // STDIO_H
