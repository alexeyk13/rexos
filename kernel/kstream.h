/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef KSTREAM_H
#define KSTREAM_H

#include "../userspace/ipc.h"
#include "kprocess.h"

//called from kprocess
void kstream_lock_release(HANDLE h, HANDLE process);

//called from kernel/exodrivers/svc
HANDLE kstream_create(unsigned int size);
HANDLE kstream_open(HANDLE process, HANDLE s);
void kstream_close(HANDLE process, HANDLE h);
unsigned int kstream_get_size(HANDLE s);
unsigned int kstream_get_free(HANDLE s);
void kstream_listen(HANDLE process, HANDLE s, unsigned int param, HAL hal);
void kstream_stop_listen(HANDLE process, HANDLE s);
unsigned int kstream_write_no_block(HANDLE h, char* buf, unsigned int size_max);
void kstream_write(HANDLE process, HANDLE h, char* buf, unsigned int size);
unsigned int kstream_read_no_block(HANDLE h, char* buf, unsigned int size_max);
void kstream_read(HANDLE process, HANDLE h, char* buf, unsigned int size);
void kstream_flush(HANDLE s);
void kstream_destroy(HANDLE s);

#endif // KSTREAM_H
