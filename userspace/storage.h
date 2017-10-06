/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef STORAGE_H
#define STORAGE_H

#include "ipc.h"
#include "io.h"

#define STORAGE_FLAG_ERASE_ONLY                                         0x0
#define STORAGE_FLAG_WRITE                                              (1 << 0)
#define STORAGE_FLAG_VERIFY                                             (1 << 1)

#define STORAGE_FLAG_IGNORE_ACTIVITY_ON_REQUEST                         (1 << 31)
#define STORAGE_MASK_MODE                                               0x7fffffff

typedef enum {
    STORAGE_GET_MEDIA_DESCRIPTOR = IPC_USER,
    STORAGE_NOTIFY_STATE_CHANGE,
    STORAGE_CANCEL_NOTIFY_STATE_CHANGE,
    STORAGE_NOTIFY_ACTIVITY,
    STORAGE_REQUEST_EJECT,
    STORAGE_IPC_MAX
} STORAGE_IPCS;

typedef struct {
    uint32_t num_sectors;
    uint32_t num_sectors_hi;
    uint32_t sector_size;
    //null-terminated serial number is following
} STORAGE_MEDIA_DESCRIPTOR;

#define STORAGE_MEDIA_SERIAL(p)                                         ((char*)((uint8_t*)(p) + sizeof(STORAGE_MEDIA_DESCRIPTOR)))

typedef struct {
    unsigned int sector;
    unsigned int flags;
} STORAGE_STACK;

bool storage_open(HAL hal, HANDLE process, HANDLE user);
void storage_close(HAL hal, HANDLE process, HANDLE user);
void storage_get_media_descriptor(HAL hal, HANDLE process, HANDLE user, IO* io);
void storage_request_notify_state_change(HAL hal, HANDLE process, HANDLE user);
void storage_cancel_notify_state_change(HAL hal, HANDLE process, HANDLE user);
STORAGE_MEDIA_DESCRIPTOR* storage_get_media_descriptor_sync(HAL hal, HANDLE process, HANDLE user, IO* io);
void storage_read(HAL hal, HANDLE process, HANDLE user, IO* io, unsigned int sector, unsigned int size);
bool storage_read_sync(HAL hal, HANDLE process, HANDLE user, IO* io, unsigned int sector, unsigned int size);
void storage_write(HAL hal, HANDLE process, HANDLE user, IO* io, unsigned int sector);
bool storage_write_sync(HAL hal, HANDLE process, HANDLE user, IO* io, unsigned int sector);
void storage_erase(HAL hal, HANDLE process, HANDLE user, IO* io, unsigned int sector, unsigned int size);
bool storage_erase_sync(HAL hal, HANDLE process, HANDLE user, IO* io, unsigned int sector, unsigned int size);
void storage_verify(HAL hal, HANDLE process, HANDLE user, IO* io, unsigned int sector);
bool storage_verify_sync(HAL hal, HANDLE process, HANDLE user, IO* io, unsigned int sector);
void storage_write_verify(HAL hal, HANDLE process, HANDLE user, IO* io, unsigned int sector);
bool storage_write_verify_sync(HAL hal, HANDLE process, HANDLE user, IO* io, unsigned int sector);
void storage_request_activity_notify(HAL hal, HANDLE process, HANDLE user);
void storage_request_eject(HAL hal, HANDLE process, HANDLE user);

#endif // STORAGE_H
