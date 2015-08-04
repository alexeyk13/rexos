/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2015, Alexey Kramarenko
    All rights reserved.
*/

#include "kio.h"
#include "kprocess.h"
#include "kmalloc.h"
#include "kernel_config.h"

void kio_create(IO** io, unsigned int size)
{
    KIO* kio;
    KPROCESS* kprocess = kprocess_get_current();
    CHECK_ADDRESS(kprocess, io, sizeof(void*));
    kio = (KIO*)kmalloc(sizeof(KIO));
    if (kio != NULL)
    {
        DO_MAGIC(kio, MAGIC_KIO);
        kio->owner = kio->granted = kprocess;
        kio->kill_flag = false;
        (*io) = kio->io = (IO*)kmalloc(size + sizeof(IO));
        if (kio->io == NULL)
        {
            kfree(kio);
            kprocess_error(kprocess, ERROR_OUT_OF_PAGED_MEMORY);
        }
        kio->io->kio = (HANDLE)kio;
        kio->io->size = size + sizeof(IO);
    }
    else
        kprocess_error(kprocess, ERROR_OUT_OF_SYSTEM_MEMORY);
}

static void kio_destroy_internal(KIO* kio)
{
    CLEAR_MAGIC(kio);
    kfree(kio->io);
    kfree(kio);
}

bool kio_send(IO* io, KPROCESS* receiver)
{
    KPROCESS* kprocess = kprocess_get_current();
    KIO* kio = (KIO*)io->kio;
    CHECK_HANDLE(kio, sizeof(KIO));
    CHECK_MAGIC(kio, MAGIC_KIO);
    if (kprocess != kio->granted)
    {
        kprocess_error(kprocess, ERROR_ACCESS_DENIED);
        return false;
    }
    //user released IO
    if (kio->kill_flag)
    {
        kio_destroy_internal(kio);
        kprocess_error(kprocess, ERROR_SYNC_OBJECT_DESTROYED);
        return false;
    }
    kio->granted = receiver;
    return true;
}

void kio_destroy(IO *io)
{
    bool kill = true;
    if (io == NULL)
        return;
    KIO* kio = (KIO*)io->kio;
    KPROCESS* kprocess = kprocess_get_current();
    CHECK_HANDLE(kio, sizeof(KIO));
    CHECK_MAGIC(kio, MAGIC_KIO);

    if (kprocess != kio->owner)
    {
        kprocess_error(kprocess, ERROR_ACCESS_DENIED);
        return;
    }
    disable_interrupts();
    if (kio->granted != kio->owner)
    {
        kio->kill_flag = true;
        kill = false;
    }
    enable_interrupts();
    if (kill)
        kio_destroy_internal(kio);
}
