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
    PROCESS* process = kprocess_get_current();
    CHECK_ADDRESS(process, io, sizeof(void*));
    kio = (KIO*)kmalloc(sizeof(KIO));
    if (kio != NULL)
    {
        DO_MAGIC(kio, MAGIC_KIO);
        kio->owner = kio->granted = process;
        kio->kill_flag = false;
        (*io) = kio->io = (IO*)kmalloc(size + sizeof(IO));
        if (kio->io == NULL)
        {
            kfree(kio);
            kprocess_error(process, ERROR_OUT_OF_PAGED_MEMORY);
        }
        kio->io->kio = (HANDLE)kio;
        kio->io->size = size + sizeof(IO);
    }
    else
        kprocess_error(process, ERROR_OUT_OF_SYSTEM_MEMORY);
}

static void kio_destroy_internal(KIO* kio)
{
    CLEAR_MAGIC(kio);
    kfree(kio->io);
    kfree(kio);
}

static bool kio_send_internal(PROCESS* process, KIO* kio, IPC* ipc)
{
    if (process != kio->granted)
    {
        kprocess_error(process, ERROR_ACCESS_DENIED);
        return false;
    }
    //user released IO
    if (kio->kill_flag)
    {
        kio_destroy_internal(kio);
        return false;
    }
    kio->granted = (PROCESS*)ipc->process;
    kipc_post_process(ipc, (HANDLE)process);
    return true;
}

void kio_send(KIO* kio, IPC* ipc)
{
    PROCESS* process = kprocess_get_current();
    CHECK_HANDLE(kio, sizeof(KIO));
    CHECK_MAGIC(kio, MAGIC_KIO);
    CHECK_ADDRESS(process, ipc, sizeof(IPC));
    kio_send_internal(process, kio, ipc);
}

void kio_call(KIO* kio, IPC* ipc, SYSTIME* time)
{
    PROCESS* process = kprocess_get_current();
    CHECK_HANDLE(kio, sizeof(KIO));
    CHECK_MAGIC(kio, MAGIC_KIO);
    CHECK_ADDRESS(process, ipc, sizeof(IPC));
    HANDLE wait_process = (HANDLE)(ipc->process);
    if (kio_send_internal(process, kio, ipc))
        kipc_read_process(process, ipc, time, wait_process, ipc->cmd);
}

void kio_destroy(KIO *kio)
{
    bool kill = true;
    if ((HANDLE)kio == INVALID_HANDLE)
        return;
    PROCESS* process = kprocess_get_current();
    CHECK_HANDLE(kio, sizeof(KIO));
    CHECK_MAGIC(kio, MAGIC_KIO);

    if (process != kio->owner)
    {
        kprocess_error(process, ERROR_ACCESS_DENIED);
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
