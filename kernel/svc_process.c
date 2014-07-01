/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "svc_process.h"
#include "svc_malloc.h"
#include "string.h"
#include "svc_mutex.h"
#include "svc_event.h"
#include "svc_sem.h"
#include "kernel.h"
#include "../userspace/error.h"
#include "../lib/pool.h"

#define MAX_PROCESS_NAME_SIZE                                    128

#if (KERNEL_PROFILING)
#if (KERNEL_PROCESS_STAT)
const char *const STAT_LINE="--------------------------------------------------------------------------\n\r";
#else
const char *const STAT_LINE="----------------------------------------------------------------\n\r";
#endif
const char *const DAMAGED="     !!!DAMAGED!!!     ";
#endif //(KERNEL_PROFILING)

void svc_process_add_to_active_list(PROCESS* process)
{
    bool found = false;
    PROCESS* process_to_save = process;
    DLIST_ENUM de;
    PROCESS* cur;
#if (KERNEL_PROCESS_STAT)
    dlist_remove((DLIST**)&__KERNEL->wait_processes, (DLIST*)process);
#endif
    //process priority is less, than active, activate him
    if (process->current_priority < __KERNEL->processes->current_priority)
    {
#if (KERNEL_PROCESS_STAT)
        svc_timer_get_uptime(&process->uptime_start);
        time_sub(&__KERNEL->processes->uptime_start, &process->uptime_start, &__KERNEL->processes->uptime_start);
        time_add(&__KERNEL->processes->uptime_start, &__KERNEL->processes->uptime, &__KERNEL->processes->uptime);
#endif //KERNEL_PROCESS_STAT

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

void svc_process_remove_from_active_list(PROCESS* process)
{
    //freeze active task
    if (process == __KERNEL->processes)
    {
#if (KERNEL_PROCESS_STAT)
        svc_timer_get_uptime(&((PROCESS*)(((DLIST*)__KERNEL->processes)->next))->uptime_start);
        time_sub(&__KERNEL->processes->uptime_start, &((PROCESS*)(((DLIST*)__KERNEL->processes)->next))->uptime_start, &__KERNEL->processes->uptime_start);
        time_add(&__KERNEL->processes->uptime_start, &__KERNEL->processes->uptime, &__KERNEL->processes->uptime);
#endif //KERNEL_PROCESS_STAT
        dlist_remove_head((DLIST**)&__KERNEL->processes);
        __KERNEL->next_process = __KERNEL->processes;
        pend_switch_context();
    }
    else
        dlist_remove((DLIST**)&__KERNEL->processes, (DLIST*)process);
#if (KERNEL_PROCESS_STAT)
    dlist_add_tail((DLIST**)&__KERNEL->wait_processes, (DLIST*)process);
#endif
}

void svc_process_timeout(void* param)
{
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
        process->heap->error =  ERROR_TIMEOUT;
    svc_process_wakeup(process);
}

void svc_process_abnormal_exit()
{
#if (KERNEL_DEBUG)
    printk("Warning: abnormal process termination: %s\n\r", PROCESS_NAME(__KERNEL->processes->heap));
#endif
    if (__KERNEL->processes == __KERNEL->init)
        panic();
    else
        svc_process_destroy(__KERNEL->processes);
}

void svc_process_create(const REX* rex, PROCESS** process)
{
    *process = kmalloc(sizeof(PROCESS));
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
            (*process)->size = rex->size;
            (*process)->heap->handle = (HANDLE)(*process);
            (*process)->heap->stdout = __KERNEL->stdout_global;
            (*process)->heap->stdout_param = __KERNEL->stdout_global_param;
            (*process)->heap->stdin = __KERNEL->stdin_global;
            (*process)->heap->stdin_param = __KERNEL->stdin_global_param;
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

#if (KERNEL_PROCESS_STAT)
            dlist_add_tail((DLIST**)&__KERNEL->wait_processes, (DLIST*)(*process));
#endif
            if ((rex->flags & 1) == PROCESS_FLAGS_ACTIVE)
            {
                (*process)->flags |= PROCESS_MODE_ACTIVE;
                svc_process_add_to_active_list(*process);
            }
        }
        else
        {
            kfree(*process);
            (*process) = NULL;
            error(ERROR_OUT_OF_PAGED_MEMORY);
        }
    }
    else
        error(ERROR_OUT_OF_SYSTEM_MEMORY);
}

void svc_process_get_flags(PROCESS* process, unsigned int* flags)
{
    CHECK_MAGIC(process, MAGIC_PROCESS);
    (*flags) = process->flags;
}

void svc_process_set_flags(PROCESS* process, unsigned int flags)
{
    CHECK_MAGIC(process, MAGIC_PROCESS);
    if ((flags & 1) == PROCESS_FLAGS_ACTIVE)
    {
        switch (process->flags & PROCESS_MODE_MASK)
        {
        case PROCESS_MODE_FROZEN:
            svc_process_add_to_active_list(process);
        case PROCESS_MODE_WAITING_FROZEN:
            process->flags |= PROCESS_MODE_ACTIVE;
            break;
        }
    }
    else
    {
        switch (process->flags & PROCESS_MODE_MASK)
        {
        case PROCESS_MODE_ACTIVE:
            svc_process_remove_from_active_list(process);
        case PROCESS_MODE_WAITING:
            process->flags &= ~PROCESS_MODE_ACTIVE;
            break;
        }
    }
}

void svc_process_set_priority(PROCESS* process, unsigned int priority)
{
    CHECK_MAGIC(process, MAGIC_PROCESS);
    process->base_priority = priority;
    svc_process_set_current_priority(process, svc_mutex_calculate_owner_priority(process));
}

void svc_process_get_priority(PROCESS* process, unsigned int* priority)
{
    CHECK_MAGIC(process, MAGIC_PROCESS);
    *priority = process->base_priority;
}

void svc_process_destroy(PROCESS* process)
{
    CHECK_MAGIC(process, MAGIC_PROCESS);
    //we cannot destroy init process
    if (process == __KERNEL->init)
    {
#if (KERNEL_DEBUG)
        printk("Init process cannot be destroyed\n\r");
#endif
        error(ERROR_RESTRICTED_FOR_INIT);
        return;
    }
    //if process is running, freeze it first
    if ((process->flags & PROCESS_MODE_MASK) == PROCESS_MODE_ACTIVE)
    {
        svc_process_remove_from_active_list(process);
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
#if (KERNEL_PROCESS_STAT)
    dlist_remove((DLIST**)&__KERNEL->wait_processes, (DLIST*)process);
#endif
    //release memory, occupied by process
    stack_free(process->heap);
    kfree(process);
}

void svc_process_sleep(TIME* time, PROCESS_SYNC_TYPE sync_type, void *sync_object)
{
    PROCESS* process = __KERNEL->processes;
    CHECK_MAGIC(process, MAGIC_PROCESS);
    //init process cannot sleep or be locked by mutex
    if (process == __KERNEL->init)
    {
#if (KERNEL_DEBUG)
        printk("init process cannot sleep\n\r");
#endif
        process->flags |= sync_type;
        svc_process_timeout(sync_object);
        error(ERROR_RESTRICTED_FOR_INIT);
        return;
    }
    svc_process_remove_from_active_list(process);
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
            svc_process_add_to_active_list(process);
            break;
        }
    }
}

void svc_process_set_current_priority(PROCESS* process, unsigned int priority)
{
    CHECK_MAGIC(process, MAGIC_PROCESS);
    if (process->current_priority != priority)
    {
        switch (process->flags & PROCESS_MODE_MASK)
        {
        case PROCESS_MODE_ACTIVE:
            svc_process_remove_from_active_list(process);
            process->current_priority = priority;
            svc_process_add_to_active_list(process);
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

void svc_process_error(PROCESS* process, int error)
{
    CHECK_MAGIC(process, MAGIC_PROCESS);
    process->heap->error = error;
}

void svc_process_destroy_current()
{
    svc_process_destroy(__KERNEL->processes);
}

PROCESS* svc_process_get_current()
{
    return __KERNEL->processes;
}

void svc_process_init(const REX* rex)
{
    svc_process_create(rex, &__KERNEL->init);

    //activate init
    __KERNEL->init->flags = PROCESS_MODE_ACTIVE;
    dlist_add_head((DLIST**)&__KERNEL->processes, (DLIST*)__KERNEL->init);
    __KERNEL->next_process =__KERNEL->processes;
#if (KERNEL_PROCESS_STAT)
    dlist_clear((DLIST**)&__KERNEL->wait_processes);
#endif
    pend_switch_context();
}

#if (KERNEL_PROFILING)
void svc_process_switch_test()
{
    PROCESS* process = __KERNEL->processes;
    svc_process_remove_from_active_list(process);
    svc_process_add_to_active_list(process);
    pend_switch_context();
    //next process is now same as active process, it will simulate context switching
}

void process_stat(PROCESS* process)
{
    POOL_STAT stat;
    LIB_ENTER;
    pool_stat(&process->heap->pool, &stat, process->sp);
    LIB_EXIT;

    printk("%-16.16s ", PROCESS_NAME(process->heap));

    if (process->current_priority == (unsigned int)(-1))
        printk(" -init- ");
    else
        printk("%03d(%03d)", process->current_priority, process->base_priority);

    printk("  %4b   ", (unsigned int)process->heap + process->size - (unsigned int)process->sp);
    printk("%4b ", process->size);

    if (__KERNEL->error != ERROR_OK)
    {
        printk(DAMAGED);
        __KERNEL->error = ERROR_OK;
    }
    else
    {
        printk("%4b(%02d)  ", stat.used, stat.used_slots);
        printk("%4b/%4b(%02d)", stat.free, stat.largest_free, stat.free_slots);
    }

#if (KERNEL_PROCESS_STAT)
    printk("%3d:%02d.%03d", process->uptime.sec / 60, process->uptime.sec % 60, process->uptime.usec / 1000);
#endif
    printk("\n\r");
}

static inline void kernel_stat()
{
#if (KERNEL_PROCESS_STAT)
    TIME uptime;
#endif
    POOL_STAT stat;
    LIB_ENTER;
    pool_stat(&__KERNEL->pool, &stat, get_sp());
    LIB_EXIT;

    printk("%-16.16s         ", __KERNEL_NAME);

    printk("  %4b   ", KERNEL_BASE + KERNEL_SIZE - (unsigned int)get_sp());
    printk("%4b ", KERNEL_SIZE);

    if (__KERNEL->error != ERROR_OK)
    {
        printk(DAMAGED);
        __KERNEL->error = ERROR_OK;
    }
    else
    {
        printk("%4b(%02d)  ", stat.used, stat.used_slots);
        printk("%4b/%4b(%02d)", stat.free, stat.largest_free, stat.free_slots);
    }

#if (KERNEL_PROCESS_STAT)
    svc_timer_get_uptime(&uptime);
    printk("%3d:%02d.%03d", uptime.sec / 60, uptime.sec % 60, uptime.usec / 1000);
#endif
    printk("\n\r");
}

void svc_process_info()
{
    int cnt = 0;
    DLIST_ENUM de;
    PROCESS* cur;
#if (KERNEL_PROCESS_STAT)
    printk("    name         priority  stack  size   used         free       uptime\n\r");
#else
    printk("    name         priority  stack  size   used         free\n\r");
#endif
    printk(STAT_LINE);
    dlist_enum_start((DLIST**)&__KERNEL->processes, &de);
    while (dlist_enum(&de, (DLIST**)&cur))
    {
        process_stat(cur);
        ++cnt;
    }
#if (KERNEL_PROCESS_STAT)
    dlist_enum_start((DLIST**)&__KERNEL->wait_processes, &de);
    while (dlist_enum(&de, (DLIST**)&cur))
    {
        process_stat(cur);
        ++cnt;
    }
    printk("total %d processess\n\r", cnt);
#else
    printk("total %d active processess\n\r", cnt);
#endif
    printk(STAT_LINE);

    kernel_stat();
    printk(STAT_LINE);
}
#endif //KERNEL_PROFILING
