/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2016, Alexey Kramarenko
    All rights reserved.
*/

#ifndef STORAGE_H
#define STORAGE_H

#include "ipc.h"
#include "io.h"

//there is no reason to define something else
#define STORAGE_SECTOR_SIZE                                             512

#define STORAGE_FLAG_ERASE_ONLY                                         0x0
#define STORAGE_FLAG_WRITE                                              (1 << 0)
#define STORAGE_FLAG_VERIFY                                             (1 << 1)

typedef struct {
    unsigned int sector;
    unsigned int count;
    unsigned int flags;
} STORAGE_STACK;

bool storage_open(HAL hal, HANDLE process);
void storage_read(HAL hal, HANDLE process, IO* io, unsigned int sector, unsigned int count);
bool storage_read_sync(HAL hal, HANDLE process, IO* io, unsigned int sector, unsigned int count);
void storage_write(HAL hal, HANDLE process, IO* io, unsigned int sector);
bool storage_write_sync(HAL hal, HANDLE process, IO* io, unsigned int sector);
//TODO: erase, verify, write_verify

#endif // STORAGE_H
