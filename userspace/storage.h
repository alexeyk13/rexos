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

typedef struct {
    unsigned int sector;
    unsigned int count;
} STORAGE_STACK;

bool storage_open(HAL hal, HANDLE process);
void storage_read(HAL hal, HANDLE process, IO* io, unsigned int sector, unsigned int count);
bool storage_read_sync(HAL hal, HANDLE process, IO* io, unsigned int sector, unsigned int count);

#endif // STORAGE_H
