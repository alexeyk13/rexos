/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "svc_process.h"
#include "kernel_config.h"
#include "mem_kernel.h"
#include <stddef.h>
#include "string.h"
#include "mutex_kernel.h"
#include "event_kernel.h"
#include "sem_kernel.h"
#include "kernel.h"
#include "../userspace/core/sys_calls.h"
#include "../userspace/error.h"
#include "../lib/pool.h"
//remove this
#if (KERNEL_PROFILING)
#include "memmap.h"
#endif //KERNEL_PROFILING

#define MAX_PROCESS_NAME_SIZE                                    128

#define PROCESS_FLAGS_ACTIVE                                     (1 << 0)
#define PROCESS_FLAGS_WAITING                                    (1 << 1)
#define PROCESS_FLAGS_TIMER_ACTIVE                               (1 << 2)

#define PROCESS_MODE_MASK                                        0x3
#define PROCESS_MODE_FROZEN                                      (0)
#define PROCESS_MODE_ACTIVE                                      (PROCESS_FLAGS_ACTIVE)
#define PROCESS_MODE_WAITING_FROZEN                              (PROCESS_FLAGS_WAITING)
#define PROCESS_MODE_WAITING                                     (PROCESS_FLAGS_WAITING | PROCESS_FLAGS_ACTIVE)

#define PROCESS_SYNC_MASK                                        (0xf << 4)

#if (KERNEL_PROFILING)
#define PROCESS_NAME_PRINT_SIZE                                  16
extern void svc_stack_stat();
#endif //KERNEL_PROFILING

void svc_process_timeout(void* param)
{
    CRITICAL_ENTER;
    PROCESS* process = param;
    process->flags &= ~PROCESS_FLAGS_TIMER_ACTIVE;
    //say sync object to release us
    switch (process->flags & PROCESS_SYNC_MASK)
    {
    case PROCESS_SYNC_TIMER_ONLY:
        break;
    case PROCESS_SYNC_MUTEX:
        svc_mutex_lock_release((MUTEX*)process->sync_object, process);
        break;
    case PROCESS_SYNC_EVENT:
        svc_event_lock_release((EVENT*)process->sync_object, process);
        break;
    case PROCESS_SYNC_SEM:
        svc_sem_lock_release((SEM*)process->sync_object, process);
        break;
    default:
        ASSERT(false);
    }
    if ((process->flags & PROCESS_SYNC_MASK) != PROCESS_SYNC_TIMER_ONLY)
        svc_process_error(process, ERROR_TIMEOUT);
    svc_process_wakeup(process);
    CRITICAL_LEAVE;
}

void svc_process_create(const REX* rex, PROCESS** process)
{
    *process = sys_alloc(sizeof(PROCESS));
    memset((*process), 0, sizeof(PROCESS));
    //allocate process object
    if (*process != NULL)
    {
        (*process)->heap = stack_alloc(rex->size);
        if ((*process)->heap)
        {
            DO_MAGIC((*process), MAGIC_PROCESS);
            (*process)->base_priority = (*process)->current_priority = rex->priority;
            (*process)->sp = (void*)((unsigned int)(*process)->heap + rex->size);
            (*process)->timer.callback = svc_process_timeout;
            (*process)->timer.param = (*process);

#if (KERNEL_PROFILING)
            (*process)->heap_size = rex->size;
            memset((*process)->heap, MAGIC_UNINITIALIZED_BYTE, (*process)->heap_size);
#endif //KERNEL_PROFILING
            (*process)->heap->handle = (HANDLE)(*process);
            if (rex->name)
            {
                strncpy(PROCESS_NAME((*process)->heap), rex->name, MAX_PROCESS_NAME_SIZE);
                PROCESS_NAME((*process)->heap)[MAX_PROCESS_NAME_SIZE] = 0;
            }
            else
                strcpy(PROCESS_NAME((*process)->heap), "UNNAMED PROCESS");
            (*process)->heap->struct_size = sizeof(HEAP) + strlen(PROCESS_NAME((*process)->heap)) + 1;
            pool_init(&(*process)->heap->pool, (void*)(*process)->heap + (*process)->heap->struct_size);

            process_setup_context((*process), rex->fn);
        }
        else
        {
            sys_free(*process);
            (*process) = NULL;
            error(ERROR_OUT_OF_PAGED_MEMORY);
        }
    }
    else
        error(ERROR_OUT_OF_SYSTEM_MEMORY);
}

void process_add_to_active_list(PROCESS* process)
{
    bool found = false;
    PROCESS* process_to_save = process;
    DLIST_ENUM de;
    PROCESS* cur;
    //process priority is less, than active, activate him
    if (process->current_priority < __KERNEL->processes->current_priority)
    {
#if (KERNEL_PROFILING)
//        svc_get_uptime(&process->uptime_start);
//      time_sub(&_current_process->uptime_start, &process->uptime_start, &_current_process->uptime_start);
//        time_add(&_current_process->uptime_start, &_current_process->uptime, &_current_process->uptime);
#endif //KERNEL_PROFILING

        process_to_save = __KERNEL->processes;
        dlist_remove_head((DLIST**)&__KERNEL->processes);
        dlist_add_head((DLIST**)&__KERNEL->processes, (DLIST*)process);
        __KERNEL->next_process = process;
        pend_switch_context();
    }

    dlist_enum_start((DLIST**)&__KERNEL->processes, &de);
    while (dlist_enum(&de, (DLIST**)&cur))
        if (process_to_save->current_priority < cur->current_priority)
        {
            dlist_add_before((DLIST**)&__KERNEL->processes, (DLIST*)cur, (DLIST*)process_to_save);
            found = true;
            break;
        }
    if (!found)
        dlist_add_tail((DLIST**)&__KERNEL->processes, (DLIST*)process_to_save);
}

void process_remove_from_active_list(PROCESS* process)
{
    //freeze active task
    if (process == __KERNEL->processes)
    {
#if (KERNEL_PROFILING)
///        svc_get_uptime(&_active_processes[0]->uptime_start);
///        time_sub(&_current_process->uptime_start, &_active_processes[0]->uptime_start, &_current_process->uptime_start);
///        time_add(&_current_process->uptime_start, &_current_process->uptime, &_current_process->uptime);
#endif //KERNEL_PROFILING
        dlist_remove_head((DLIST**)&__KERNEL->processes);
        __KERNEL->next_process = __KERNEL->processes;
        pend_switch_context();
    }
    else
        dlist_remove((DLIST**)&__KERNEL->processes, (DLIST*)process);
    //TODO: add here to waiting list
}

static inline void svc_process_unfreeze(PROCESS* process)
{
    CHECK_MAGIC(process, MAGIC_PROCESS);
    switch (process->flags & PROCESS_MODE_MASK)
    {
    case PROCESS_MODE_FROZEN:
        process_add_to_active_list(process);
    case PROCESS_MODE_WAITING_FROZEN:
        process->flags |= PROCESS_MODE_ACTIVE;
        break;
    }
}

static inline void svc_process_freeze(PROCESS* process)
{
    CHECK_MAGIC(process, MAGIC_PROCESS);
    switch (process->flags & PROCESS_MODE_MASK)
    {
    case PROCESS_MODE_ACTIVE:
        process_remove_from_active_list(process);
    case PROCESS_MODE_WAITING:
        process->flags &= ~PROCESS_MODE_ACTIVE;
        break;
    }
}

void svc_process_error(PROCESS* process, int error)
{
    process->heap->error = error;
}

PROCESS* svc_process_get_current()
{
    return __KERNEL->processes;
}

void svc_process_set_current_priority(PROCESS* process, unsigned int priority)
{
    CHECK_MAGIC(process, MAGIC_PROCESS);
    if (process->current_priority != priority)
    {
        switch (process->flags & PROCESS_MODE_MASK)
        {
        case PROCESS_MODE_ACTIVE:
            process_remove_from_active_list(process);
            process->current_priority = priority;
            process_add_to_active_list(process);
            break;
        //if we are waiting for mutex, adjusting priority can affect on mutex owner
        case PROCESS_MODE_WAITING:
        case PROCESS_MODE_WAITING_FROZEN:
            process->current_priority = priority;
            if ((process->flags & PROCESS_SYNC_MASK) == PROCESS_SYNC_MUTEX)
                svc_process_set_current_priority(((MUTEX*)process->sync_object)->owner, svc_mutex_calculate_owner_priority(((MUTEX*)process->sync_object)->owner));
            break;
        default:
            process->current_priority = priority;
        }
    }
}

void svc_process_restore_current_priority(PROCESS* process)
{
    svc_process_set_current_priority(process, process->base_priority);
}

static inline void svc_process_set_priority(PROCESS* process, unsigned int priority)
{
    process->base_priority = priority;
    svc_process_set_current_priority(process, svc_mutex_calculate_owner_priority(process));
}

static inline void svc_process_get_priority(PROCESS* process, unsigned int* priority)
{
    *priority = process->base_priority;
}

static void svc_process_destroy(PROCESS* process)
{
    CHECK_MAGIC(process, MAGIC_PROCESS);
    //we cannot destroy init process
    if (process == __KERNEL->init)
    {
#if (KERNEL_DEBUG)
        printf("init process cannot be destroyed\n\r");
#endif
        panic();
    }
    //if process is running, freeze it first
    if ((process->flags & PROCESS_MODE_MASK) == PROCESS_MODE_ACTIVE)
    {
        process_remove_from_active_list(process);
        //we don't need to save context on exit
        if (__KERNEL->active_process == process)
            __KERNEL->active_process = NULL;
    }
    //if process is owned by any sync object, release them
    if  (process->flags & PROCESS_FLAGS_WAITING)
    {
        //if timer is still active, kill him
        if (process->flags & PROCESS_FLAGS_TIMER_ACTIVE)
            svc_timer_stop(&process->timer);
        //say sync object to release us
        switch (process->flags & PROCESS_SYNC_MASK)
        {
        case PROCESS_SYNC_TIMER_ONLY:
            break;
        case PROCESS_SYNC_MUTEX:
            svc_mutex_lock_release((MUTEX*)process->sync_object, process);
            break;
        case PROCESS_SYNC_EVENT:
            svc_event_lock_release((EVENT*)process->sync_object, process);
            break;
        case PROCESS_SYNC_SEM:
            svc_sem_lock_release((SEM*)process->sync_object, process);
            break;
        default:
            ASSERT(false);
        }
    }
    //release memory, occupied by process
    stack_free(process->heap);
    sys_free(process);
}

void svc_process_destroy_current()
{
    svc_process_destroy(__KERNEL->processes);
}

void svc_process_sleep(TIME* time, PROCESS_SYNC_TYPE sync_type, void *sync_object)
{
    PROCESS* process = __KERNEL->processes;
    CHECK_MAGIC(process, MAGIC_PROCESS);
    //init process cannot sleep or be locked by mutex
    if (process == __KERNEL->init)
    {
#if (KERNEL_DEBUG)
        printf("init process cannot sleep\n\r");
#endif
        panic();
    }
    process_remove_from_active_list(process);
    process->flags |= PROCESS_MODE_WAITING | sync_type;
    process->sync_object = sync_object;

    //adjust owner priority
    if (sync_type == PROCESS_SYNC_MUTEX && ((MUTEX*)sync_object)->owner->current_priority > process->current_priority)
        svc_process_set_current_priority(((MUTEX*)sync_object)->owner, process->current_priority);

    //create timer if not infinite
    if (time->sec || time->usec)
    {
        process->flags |= PROCESS_FLAGS_TIMER_ACTIVE;
        process->timer.time.sec = time->sec;
        process->timer.time.usec = time->usec;
        svc_timer_start(&process->timer);
    }
}

void svc_process_wakeup(PROCESS* process)
{
    CHECK_MAGIC(process, MAGIC_PROCESS);
    if  (process->flags & PROCESS_FLAGS_WAITING)
    {
        //if timer is still active, kill him
        if (process->flags & PROCESS_FLAGS_TIMER_ACTIVE)
            svc_timer_stop(&process->timer);
        process->flags &= ~(PROCESS_FLAGS_TIMER_ACTIVE | PROCESS_SYNC_MASK);

        switch (process->flags & PROCESS_MODE_MASK)
        {
        case PROCESS_MODE_WAITING_FROZEN:
            process->flags &= ~PROCESS_FLAGS_WAITING;
        case PROCESS_MODE_WAITING:
            process_add_to_active_list(process);
            break;
        }
    }
}

#if (KERNEL_PROFILING)
static inline void svc_process_switch_test()
{
    PROCESS* process = __KERNEL->processes;
    process_remove_from_active_list(process);
    process_add_to_active_list(process);
    pend_switch_context();
    //next process is now same as active process, it will simulate context switching
}
#endif //KERNEL_PROFILING

void svc_process_handler(unsigned int num, unsigned int param1, unsigned int param2)
{
    CHECK_CONTEXT(SUPERVISOR_CONTEXT | IRQ_CONTEXT);
    CRITICAL_ENTER;
    switch (num)
    {
    case SVC_PROCESS_CREATE:
        svc_process_create((REX*)param1, (PROCESS**)param2);
        break;
    case SVC_PROCESS_UNFREEZE:
        svc_process_unfreeze((PROCESS*)param1);
        if (__KERNEL->init)
            __KERNEL->killme2 = 1;
        break;
    case SVC_PROCESS_FREEZE:
        svc_process_freeze((PROCESS*)param1);
        break;
    case SVC_PROCESS_GET_PRIORITY:
        svc_process_get_priority((PROCESS*)param1, (unsigned int*)param2);
        break;
    case SVC_PROCESS_SET_PRIORITY:
        svc_process_set_priority((PROCESS*)param1, (unsigned int)param2);
        break;
    case SVC_PROCESS_DESTROY:
        svc_process_destroy((PROCESS*)param1);
        break;
    case SVC_PROCESS_SLEEP:
        svc_process_sleep((TIME*)param1, PROCESS_SYNC_TIMER_ONLY, NULL);
        break;
#if (KERNEL_PROFILING)
    case SVC_PROCESS_SWITCH_TEST:
//        svc_process_switch_test();
        break;
    case SVC_PROCESS_STAT:
//        svc_process_stat();
        break;
    case SVC_STACK_STAT:
//        svc_stack_stat();
        break;
#endif //KERNEL_PROFILING
    default:
        error(ERROR_INVALID_SVC);
    }
    CRITICAL_LEAVE;
}

void svc_process_init(const REX* rex)
{
    svc_process_create(rex, &__KERNEL->init);

    //activate init
    __KERNEL->init->flags = PROCESS_MODE_ACTIVE;
    dlist_add_head((DLIST**)&__KERNEL->processes, (DLIST*)__KERNEL->init);
    __KERNEL->next_process =__KERNEL->processes;
    pend_switch_context();
}

void svc_process_abnormal_exit()
{
    PROCESS* process;
    process = (PROCESS*)process_get_current();
#if (KERNEL_DEBUG)
    printf("Warning: abnormal process termination: %s\n\r", PROCESS_NAME(process->heap));
#endif
    svc_process_destroy(process);
}
