#ifndef KPROCESS_PRIVATE_H
#define KPROCESS_PRIVATE_H

#include "../userspace/dlist.h"
#include "../userspace/systime.h"
#include "../userspace/types.h"
#include "../userspace/irq.h"
#include "kernel_config.h"
#include "dbg.h"

typedef struct {
    int error;
    IRQ handler;
    void* param;
    HANDLE process;
#ifdef SOFT_NVIC
    bool pending;
#endif
}KIRQ;

typedef struct _KTIMER {
    DLIST list;
    SYSTIME time;
    void (*callback)(void*);
    void* param;
    bool active;
} KTIMER;

typedef struct {
    //process, we are waiting for. Can be INVALID_HANDLE, then waiting from any process
    HANDLE wait_process;
    unsigned int cmd, param1;
}KIPC;

typedef struct _KPROCESS {
    DLIST list;                                                        //list of processes - active, frozen, or owned by sync object
    PROCESS* process;                                                  //process userspace data pointer
    unsigned int* sp;                                                  //current sp(if saved)
    MAGIC;
    unsigned int size;
    unsigned long flags;
    unsigned base_priority;                                            //base priority
    KTIMER timer;                                                      //timer for process sleep and sync objects timeouts
    HANDLE sync_object;                                                //sync object we are waiting for
#if (KERNEL_PROCESS_STAT)
    SYSTIME uptime;
    SYSTIME uptime_start;
#endif //KERNEL_PROCESS_STAT
    KIPC kipc;
}KPROCESS;

#endif // KPROCESS_PRIVATE_H
