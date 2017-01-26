/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "kio.h"
#include "kprocess.h"
#include "kstdlib.h"
#include "kernel_config.h"

typedef struct {
    DLIST list;
    MAGIC;
    IO* io;
    HANDLE owner;
    HANDLE granted;
    bool kill_flag;
}KIO;

static void kio_destroy_internal(KIO* kio)
{
    CLEAR_MAGIC(kio);
    kfree(kio->io);
    kfree(kio);
}

IO* kio_create(unsigned int size)
{
    KIO* kio;
    HANDLE process = kprocess_get_current();
    kio = (KIO*)kmalloc(sizeof(KIO));
    if (kio != NULL)
    {
        DO_MAGIC(kio, MAGIC_KIO);
        kio->owner = kio->granted = process;
        kio->kill_flag = false;
        kio->io = (IO*)kmalloc(size + sizeof(IO));
        if ((kio->io) == NULL)
        {
            kfree(kio);
            return NULL;
        }
        kio->io->kio = (HANDLE)kio;
        kio->io->size = size + sizeof(IO);
    }
    return kio->io;
}

bool kio_send(HANDLE process, IO* io, HANDLE receiver)
{
    KIO* kio = (KIO*)(io->kio);
    //sent from untrusted environment
    CHECK_MAGIC(kio, MAGIC_KIO);
    if (process != kio->granted)
    {
        error(ERROR_ACCESS_DENIED);
        return false;
    }
    //user released IO
    if ((kio->kill_flag) && (receiver == kio->owner))
    {
        kio_destroy_internal(kio);
        return true;
    }
    kio->granted = receiver;
    return true;
}

void kio_destroy(IO *io)
{
    bool kill_flag = false;
    if (io == NULL)
        return;
    KIO* kio = (KIO*)io->kio;
    CHECK_MAGIC(kio, MAGIC_KIO);

    if (kio->owner != kprocess_get_current())
    {
        error(ERROR_ACCESS_DENIED);
        return;
    }
    disable_interrupts();
    if (kio->granted != kio->owner)
    {
        kio->kill_flag = true;
        kill_flag = true;
    }
    enable_interrupts();
    if (kill_flag)
        error(ERROR_BUSY);
    else
        kio_destroy_internal(kio);
}
