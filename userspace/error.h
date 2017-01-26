/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef ERROR_H
#define ERROR_H

/*
    error.h - error handling
*/

//not actually error, just flag
#define ERROR_SYNC                                      1

#define ERROR_GENERAL                                   0
#define ERROR_OK                                        (ERROR_GENERAL - 0)
#define ERROR_IN_PROGRESS                               (ERROR_GENERAL - 1)
#define ERROR_NOT_SUPPORTED                             (ERROR_GENERAL - 2)
#define ERROR_NOT_FOUND                                 (ERROR_GENERAL - 3)
#define ERROR_NOT_ACTIVE                                (ERROR_GENERAL - 4)
#define ERROR_NOT_CONFIGURED                            (ERROR_GENERAL - 5)
#define ERROR_ALREADY_CONFIGURED                        (ERROR_GENERAL - 6)
#define ERROR_INVALID_PARAMS                            (ERROR_GENERAL - 7)
#define ERROR_INVALID_MAGIC                             (ERROR_GENERAL - 8)
#define ERROR_INVALID_SVC                               (ERROR_GENERAL - 9)
#define ERROR_SYNC_OBJECT_DESTROYED                     (ERROR_GENERAL - 10)
#define ERROR_OUT_OF_RANGE                              (ERROR_GENERAL - 11)
#define ERROR_ACCESS_DENIED                             (ERROR_GENERAL - 12)
#define ERROR_OVERFLOW                                  (ERROR_GENERAL - 13)
#define ERROR_UNDERFLOW                                 (ERROR_GENERAL - 14)
#define ERROR_INVALID_STATE                             (ERROR_GENERAL - 15)
#define ERROR_INVALID_ALIGN                             (ERROR_GENERAL - 16)
#define ERROR_DEADLOCK                                  (ERROR_GENERAL - 17)
#define ERROR_INVALID_MODE                              (ERROR_GENERAL - 18)
#define ERROR_INVALID_LENGTH                            (ERROR_GENERAL - 19)
#define ERROR_TOO_MANY_HANDLES                          (ERROR_GENERAL - 20)
#define ERROR_BUSY                                      (ERROR_GENERAL - 21)

#define ERROR_IO                                        -100
#define ERROR_FILE_SHARING_VIOLATION                    (ERROR_IO - 1)
#define ERROR_FILE_INVALID_NAME                         (ERROR_IO - 2)
#define ERROR_FILE_PATH_ALREADY_EXISTS                  (ERROR_IO - 3)
#define ERROR_IO_BUFFER_TOO_SMALL                       (ERROR_IO - 4)
#define ERROR_IO_CANCELLED                              (ERROR_IO - 5)
#define ERROR_FOLDER_NOT_EMPTY                          (ERROR_IO - 6)
#define ERROR_IO_ASYNC_COMPLETE                         (ERROR_IO - 7)
#define ERROR_IO_FAIL                                   (ERROR_IO - 8)
#define ERROR_CORRUPTED                                 (ERROR_IO - 9)
#define ERROR_EOF                                       (ERROR_IO - 10)
#define ERROR_FULL                                      (ERROR_IO - 11)

#define ERROR_MEMORY                                    -200
#define ERROR_OUT_OF_MEMORY                             (ERROR_MEMORY - 1)
#define ERROR_OUT_OF_SYSTEM_MEMORY                      (ERROR_MEMORY - 2)
#define ERROR_OUT_OF_PAGED_MEMORY                       (ERROR_MEMORY - 3)
#define ERROR_POOL_CORRUPTED                            (ERROR_MEMORY - 4)
#define ERROR_POOL_RANGE_CHECK_FAILED                   (ERROR_MEMORY - 5)

#define ERROR_COMM                                      -300
#define ERROR_INVALID_FRAME                             (ERROR_COMM - 1)
#define ERROR_CRC                                       (ERROR_COMM - 2)
#define ERROR_NOT_RESPONDING                            (ERROR_COMM - 3)
#define ERROR_CONNECTION_REFUSED                        (ERROR_COMM - 4)
#define ERROR_CONNECTION_CLOSED                         (ERROR_COMM - 5)
#define ERROR_LINE_NOISE                                (ERROR_COMM - 6)
#define ERROR_INVALID_PARITY                            (ERROR_COMM - 7)
#define ERROR_COMM_BREAK                                (ERROR_COMM - 8)
#define ERROR_NAK                                       (ERROR_COMM - 9)
#define ERROR_TIMEOUT                                   (ERROR_COMM - 10)
#define ERROR_CHAR_LOSS                                 (ERROR_COMM - 11)

#define ERROR_HARDWARE                                  -400


#endif // ERROR_H
