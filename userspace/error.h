/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#ifndef ERROR_H
#define ERROR_H

/*
    error.h - error handling
*/

#define ERROR_OK                                        0
#define ERROR_IN_PROGRESS                               -1
#define ERROR_NOT_SUPPORTED                             -2
#define ERROR_NOT_FOUND                                 -3
#define ERROR_NOT_ACTIVE                                -4
#define ERROR_NOT_CONFIGURED                            -5
#define ERROR_ALREADY_CONFIGURED                        -6
#define ERROR_INVALID_PARAMS                            -7
#define ERROR_INVALID_MAGIC                             -8
#define ERROR_TIMEOUT                                   -9
#define ERROR_INVALID_SVC                               -10
#define ERROR_SYNC_OBJECT_DESTROYED                     -11
#define ERROR_STUB_CALLED                               -12
#define ERROR_OUT_OF_RANGE                              -13
#define ERROR_ACCESS_DENIED                             -14
#define ERROR_OVERFLOW                                  -15
#define ERROR_UNDERFLOW                                 -16
#define ERROR_NAK                                       -17
#define ERROR_INVALID_STATE                             -18

#define ERROR_FILE_PATH_NOT_FOUND                       -19
#define ERROR_FILE_SHARING_VIOLATION                    -20
#define ERROR_FILE_ACCESS_DENIED                        -21
#define ERROR_FILE_INVALID_NAME                         -22
#define ERROR_FILE_PATH_ALREADY_EXISTS                  -23
#define ERROR_IO_BUFFER_TOO_SMALL                       -24
#define ERROR_IO_CANCELLED                              -25
#define ERROR_FOLDER_NOT_EMPTY                          -26
#define ERROR_NOT_MOUNTED                               -27
#define ERROR_ALREADY_MOUNTED                           -28

#define ERROR_OUT_OF_MEMORY                             -29
#define ERROR_OUT_OF_SYSTEM_MEMORY                      -30
#define ERROR_OUT_OF_PAGED_MEMORY                       -31
#define ERROR_POOL_CORRUPTED                            -32
#define ERROR_POOL_RANGE_CHECK_FAILED                   -33

#define ERROR_UART_NOISE                                -34
#define ERROR_UART_FRAME                                -35
#define ERROR_UART_PARITY                               -36
#define ERROR_UART_BREAK                                -37

#define ERROR_USB_STALL                                 -38

#define ERROR_INVALID_ALIGN                             -39
#define ERROR_HARDWARE                                  -40

#endif // ERROR_H
