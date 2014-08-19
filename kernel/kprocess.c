/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "kprocess.h"
#include "kmalloc.h"
#include "string.h"
#include "kstream.h"
#include "kernel.h"
#include "kdirect.h"
#if (KERNEL_MES)
#include "kmutex.h"
#include "kevent.h"
#include "ksem.h"
#endif
#include "../userspace/error.h"
#include "../lib/pool.h"
#include "../userspace/ipc.h"

#define MAX_PROCESS_NAME_SIZE                                    128

#if (KERNEL_PROFILING)
#if (KERNEL_PROCESS_STAT)
const char *const STAT_LINE="--------------------------------------------------------------------------\n\r";
#else
const char *const STAT_LINE="----------------------------------------------------------------\n\r";
#endif
const char *const DAMAGED="     !!!DAMAGED!!!     ";
#endif //(KERNEL_PROFILING)

static inline void switch_to_process(PROCESS* process)
{
    __KERNEL->next_process = process;
    __GLOBAL->heap = process->heap;
    pend_switch_context();
}

void kprocess_add_to_active_list(PROCESS* process)
{
    bool found = false;
    PROCESS* process_to_save = process;
    DLIST_ENUM de;
    PROCESS* cur;
#if (KERNEL_PROCESS_STAT)
    dlist_remove((DLIST**)&__KERNEL->wait_processes, (DLIST*)process);
#endif
    //process priority is less, than active, activate him
#if (KERNEL_MES)
    if (process->current_priority < __KERNEL->processes->current_priority)
#else
    if (process->base_priority < __KERNEL->processes->base_priority)
#endif //KERNEL_MES
    {
#if (KERNEL_PROCESS_STAT)
        ktimer_get_uptime(&process->uptime_start);
        time_sub(&__KERNEL->processes->uptime_start, &process->uptime_start, &__KERNEL->processes->uptime_start);
        time_add(&__KERNEL->processes->uptime_start, &__KERNEL->processes->uptime, &__KERNEL->processes->uptime);
#endif //KERNEL_PROCESS_STAT

        process_to_save = __KERNEL->processes;
        dlist_remove_head((DLIST**)&__KERNEL->processes);
        dlist_add_head((DLIST**)&__KERNEL->processes, (DLIST*)process);
        switch_to_process(process);
    }

    dlist_enum_start((DLIST**)&__KERNEL->processes, &de);
    while (dlist_enum(&de, (DLIST**)&cur))
#if (KERNEL_MES)
        if (process_to_save->current_priority < cur->current_priority)
#else
        if (process_to_save->base_priority < cur->base_priority)
#endif //KERNEL_MES
        {
            dlist_add_before((DLIST**)&__KERNEL->processes, (DLIST*)cur, (DLIST*)process_to_save);
            found = true;
            break;
        }
    if (!found)
        dlist_add_tail((DLIST**)&__KERNEL->processes, (DLIST*)process_to_save);
}

void kprocess_remove_from_active_list(PROCESS* process)
{
    //freeze active task
    if (process == __KERNEL->processes)
    {
#if (KERNEL_PROCESS_STAT)
        ktimer_get_uptime(&((PROCESS*)(((DLIST*)__KERNEL->processes)->next))->uptime_start);
        time_sub(&__KERNEL->processes->uptime_start, &((PROCESS*)(((DLIST*)__KERNEL->processes)->next))->uptime_start, &__KERNEL->processes->uptime_start);
        time_add(&__KERNEL->processes->uptime_start, &__KERNEL->processes->uptime, &__KERNEL->processes->uptime);
#endif //KERNEL_PROCESS_STAT
        dlist_remove_head((DLIST**)&__KERNEL->processes);
        switch_to_process(__KERNEL->processes);
    }
    else
        dlist_remove((DLIST**)&__KERNEL->processes, (DLIST*)process);
#if (KERNEL_PROCESS_STAT)
    dlist_add_tail((DLIST**)&__KERNEL->wait_processes, (DLIST*)process);
#endif
}

void kprocess_timeout(void* param)
{
    PROCESS* process = param;
    process->flags &= ~PROCESS_FLAGS_TIMER_ACTIVE;
    //say sync object to release us
    switch (process->flags & PROCESS_SYNC_MASK)
    {
    case PROCESS_SYNC_TIMER_ONLY:
    case PROCESS_SYNC_IPC:
        break;
    case PROCESS_SYNC_STREAM:
        kstream_lock_release((STREAM_HANDLE*)process->sync_object, process);
        break;
#if (KERNEL_MES)
    case PROCESS_SYNC_MUTEX:
        kmutex_lock_release((MUTEX*)process->sync_object, process);
        break;
    case PROCESS_SYNC_EVENT:
        kevent_lock_release((EVENT*)process->sync_object, process);
        break;
    case PROCESS_SYNC_SEM:
        ksem_lock_release((SEM*)process->sync_object, process);
        break;
#endif //KERNEL_MES
    default:
        ASSERT(false);
    }
    if ((process->flags & PROCESS_SYNC_MASK) != PROCESS_SYNC_TIMER_ONLY)
        process->heap->error =  ERROR_TIMEOUT;
    kprocess_wakeup(process);
}

void kprocess_abnormal_exit()
{
#if (KERNEL_INFO)
    printk("Warning: abnormal process termination: %s\n\r", PROCESS_NAME(kprocess_get_current()->heap));
#endif
    if (kprocess_get_current() == __KERNEL->init)
        panic();
    else
        kprocess_destroy_current();
}

void kprocess_create(const REX* rex, PROCESS** process)
{
    *process = kmalloc(sizeof(PROCESS) + rex->ipc_size * sizeof(IPC));
    memset((*process), 0, sizeof(PROCESS) + rex->ipc_size * sizeof(IPC));
    //allocate process object
    if (*process != NULL)
    {
        (*process)->heap = paged_alloc(rex->size);
        if ((*process)->heap)
        {
            DO_MAGIC((*process), MAGIC_PROCESS);
            (*process)->base_priority = rex->priority;
#if (KERNEL_MES)
            (*process)->current_priority = rex->priority;
#endif //KERNEL_MES
            (*process)->sp = (void*)((unsigned int)(*process)->heap + rex->size);
            (*process)->timer.callback = kprocess_timeout;
            (*process)->timer.param = (*process);
            (*process)->size = rex->size;
            (*process)->blocks_count = 0;
            kipc_init((HANDLE)*process, rex->ipc_size);
            kdirect_init(*process);
            (*process)->heap->handle = (HANDLE)(*process);
            (*process)->heap->system = (HANDLE)__KERNEL->system;
            (*process)->heap->stdout = (*process)->heap->stdin = INVALID_HANDLE;
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
                kprocess_add_to_active_list(*process);
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

void kprocess_get_flags(PROCESS* process, unsigned int* flags)
{
    CHECK_HANDLE(process, sizeof(PROCESS));
    CHECK_MAGIC(process, MAGIC_PROCESS);
    CHECK_ADDRESS(process, flags, sizeof(unsigned int));
    (*flags) = process->flags;
}

void kprocess_set_flags(PROCESS* process, unsigned int flags)
{
    CHECK_HANDLE(process, sizeof(PROCESS));
    CHECK_MAGIC(process, MAGIC_PROCESS);
    if ((flags & 1) == PROCESS_FLAGS_ACTIVE)
    {
        switch (process->flags & PROCESS_MODE_MASK)
        {
        case PROCESS_MODE_FROZEN:
            kprocess_add_to_active_list(process);
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
            kprocess_remove_from_active_list(process);
        case PROCESS_MODE_WAITING:
            process->flags &= ~PROCESS_MODE_ACTIVE;
            break;
        }
    }
}

void kprocess_set_priority(PROCESS* process, unsigned int priority)
{
    CHECK_HANDLE(process, sizeof(PROCESS));
    CHECK_MAGIC(process, MAGIC_PROCESS);
#if (KERNEL_MES)
    process->base_priority = priority;
    kprocess_set_current_priority(process, kmutex_calculate_owner_priority(process));
#else
    if (process->base_priority != priority)
    {
        process->base_priority = priority;
        if ((process->flags & PROCESS_MODE_MASK) == PROCESS_MODE_ACTIVE)
        {
            kprocess_remove_from_active_list(process);
            kprocess_add_to_active_list(process);
        }
    }

#endif

}

void kprocess_get_priority(PROCESS* process, unsigned int* priority)
{
    CHECK_HANDLE(process, sizeof(PROCESS));
    CHECK_MAGIC(process, MAGIC_PROCESS);
    CHECK_ADDRESS(process, priority, sizeof(unsigned int));
    *priority = process->base_priority;
}

void kprocess_destroy(PROCESS* process)
{
    if ((HANDLE)process == INVALID_HANDLE)
        return;
    CHECK_HANDLE(process, sizeof(PROCESS));
    CHECK_MAGIC(process, MAGIC_PROCESS);
    CLEAR_MAGIC(process);
    //we cannot destroy init process
    if (process == __KERNEL->init)
    {
#if (KERNEL_INFO)
        printk("Init process cannot be destroyed\n\r");
#endif
        error(ERROR_RESTRICTED_FOR_INIT);
        return;
    }
    //if process is running, freeze it first
    if ((process->flags & PROCESS_MODE_MASK) == PROCESS_MODE_ACTIVE)
    {
        kprocess_remove_from_active_list(process);
        //we don't need to save context on exit
        if (__KERNEL->active_process == process)
            __KERNEL->active_process = NULL;
    }
    //if process is owned by any sync object, release them
    if  (process->flags & PROCESS_FLAGS_WAITING)
    {
        //if timer is still active, kill him
        if (process->flags & PROCESS_FLAGS_TIMER_ACTIVE)
            ktimer_stop(&process->timer);
        //say sync object to release us
        switch (process->flags & PROCESS_SYNC_MASK)
        {
        case PROCESS_SYNC_TIMER_ONLY:
        case PROCESS_SYNC_IPC:
            break;
        case PROCESS_SYNC_STREAM:
            kstream_lock_release((STREAM_HANDLE*)process->sync_object, process);
            break;
#if (KERNEL_MES)
        case PROCESS_SYNC_MUTEX:
            kmutex_lock_release((MUTEX*)process->sync_object, process);
            break;
        case PROCESS_SYNC_EVENT:
            kevent_lock_release((EVENT*)process->sync_object, process);
            break;
        case PROCESS_SYNC_SEM:
            ksem_lock_release((SEM*)process->sync_object, process);
            break;
#endif //KERNEL_MES
        default:
            ASSERT(false);
        }
    }
#if (KERNEL_PROCESS_STAT)
    dlist_remove((DLIST**)&__KERNEL->wait_processes, (DLIST*)process);
#endif
    //release memory, occupied by process
    paged_free(process->heap);
    kfree(process);
}

void kprocess_sleep(PROCESS* process, TIME* time, PROCESS_SYNC_TYPE sync_type, void *sync_object)
{
    CHECK_HANDLE(process, sizeof(PROCESS));
    CHECK_MAGIC(process, MAGIC_PROCESS);
    ASSERT ((process->flags & PROCESS_FLAGS_WAITING) == 0);
    //init process cannot sleep or be locked by mutex
    if (process == __KERNEL->init)
    {
        process->flags |= sync_type;
        kprocess_timeout(sync_object);
        error(ERROR_RESTRICTED_FOR_INIT);
        return;
    }
    kprocess_remove_from_active_list(process);
    process->flags |= PROCESS_FLAGS_WAITING | sync_type;
    process->sync_object = sync_object;

#if (KERNEL_MES)
    //adjust owner priority
    if (sync_type == PROCESS_SYNC_MUTEX && ((MUTEX*)sync_object)->owner->current_priority > process->current_priority)
        kprocess_set_current_priority(((MUTEX*)sync_object)->owner, process->current_priority);
#endif //KERNEL_MES

    //create timer if not infinite
    if (time->sec || time->usec)
    {
        process->flags |= PROCESS_FLAGS_TIMER_ACTIVE;
        process->timer.time.sec = time->sec;
        process->timer.time.usec = time->usec;
        ktimer_start(&process->timer);
    }
}

void kprocess_sleep_current(TIME* time, PROCESS_SYNC_TYPE sync_type, void *sync_object)
{
    PROCESS* process = kprocess_get_current();
    if (process != NULL)
        kprocess_sleep(process, time, sync_type, sync_object);
}

void kprocess_wakeup(PROCESS* process)
{
    CHECK_HANDLE(process, sizeof(PROCESS));
    CHECK_MAGIC(process, MAGIC_PROCESS);
    if  (process->flags & PROCESS_FLAGS_WAITING)
    {
        //if timer is still active, kill him
        if (process->flags & PROCESS_FLAGS_TIMER_ACTIVE)
            ktimer_stop(&process->timer);
        process->flags &= ~(PROCESS_FLAGS_TIMER_ACTIVE | PROCESS_SYNC_MASK);

        switch (process->flags & PROCESS_MODE_MASK)
        {
        case PROCESS_MODE_WAITING:
            kprocess_add_to_active_list(process);
        case PROCESS_MODE_WAITING_FROZEN:
            process->flags &= ~PROCESS_FLAGS_WAITING;
            break;
        }
    }
}

#if (KERNEL_MES)
void kprocess_set_current_priority(PROCESS* process, unsigned int priority)
{
    CHECK_HANDLE(process, sizeof(PROCESS));
    CHECK_MAGIC(process, MAGIC_PROCESS);
    if (process->current_priority != priority)
    {
        switch (process->flags & PROCESS_MODE_MASK)
        {
        case PROCESS_MODE_ACTIVE:
            kprocess_remove_from_active_list(process);
            process->current_priority = priority;
            kprocess_add_to_active_list(process);
            break;
        //if we are waiting for mutex, adjusting priority can affect on mutex owner
        case PROCESS_MODE_WAITING:
        case PROCESS_MODE_WAITING_FROZEN:
            process->current_priority = priority;
            if ((process->flags & PROCESS_SYNC_MASK) == PROCESS_SYNC_MUTEX)
                kprocess_set_current_priority(((MUTEX*)process->sync_object)->owner, kmutex_calculate_owner_priority(((MUTEX*)process->sync_object)->owner));
            break;
        default:
            process->current_priority = priority;
        }
    }
}
#endif //KERNEL_MES

void kprocess_error(PROCESS* process, int error)
{
    CHECK_HANDLE(process, sizeof(PROCESS));
    CHECK_MAGIC(process, MAGIC_PROCESS);
    process->heap->error = error;
}

void kprocess_error_current(int error)
{
    PROCESS* process = kprocess_get_current();
    if (process != NULL)
        kprocess_error(process, error);
}

void kprocess_destroy_current()
{
    PROCESS* process = kprocess_get_current();
    if (process != NULL)
        kprocess_destroy(process);
}

PROCESS* kprocess_get_current()
{
    return (__KERNEL->context >= 0) ?  __KERNEL->irqs[__KERNEL->context].process : __KERNEL->active_process;
}

int kprocess_block_open(PROCESS* process, void* data, unsigned int size)
{
    int index = -1;
    CHECK_HANDLE_RET(process, sizeof(PROCESS), index);
    CHECK_MAGIC_RET(process, MAGIC_PROCESS, index);
    if (process->blocks_count < KERNEL_BLOCKS_COUNT)
    {
        index = process->blocks_count++;
        process->blocks[index].data = data;
        process->blocks[index].size = size;
    }
    else
        kprocess_error(process, ERROR_TOO_MANY_OPEN_BLOCKS);
    return index;
}

void kprocess_block_close(PROCESS* process, int index)
{
    CHECK_HANDLE(process, sizeof(PROCESS));
    CHECK_MAGIC(process, MAGIC_PROCESS);
    if (--process->blocks_count != index)
    {
        process->blocks[index].data = process->blocks[process->blocks_count - 1].data;
        process->blocks[index].size = process->blocks[process->blocks_count - 1].size;
    }
}

void kprocess_init(const REX* rex)
{
    kprocess_create(rex, &__KERNEL->init);

    //activate init
    __KERNEL->init->flags = PROCESS_MODE_ACTIVE;
    dlist_add_head((DLIST**)&__KERNEL->processes, (DLIST*)__KERNEL->init);
#if (KERNEL_PROCESS_STAT)
    dlist_clear((DLIST**)&__KERNEL->wait_processes);
#endif
    switch_to_process(__KERNEL->processes);
}

#if (KERNEL_PROFILING)
void kprocess_switch_test()
{
    PROCESS* process = kprocess_get_current();
    kprocess_remove_from_active_list(process);
    kprocess_add_to_active_list(process);
    switch_to_process(process);
    //next process is now same as active process, it will simulate context switching
}

void process_stat(PROCESS* process)
{
    POOL_STAT stat;
    LIB_ENTER;
    __GLOBAL->lib->pool_stat(&process->heap->pool, &stat, process->sp);
    LIB_EXIT;

    printk("%-16.16s ", PROCESS_NAME(process->heap));

#if (KERNEL_MES)
    if (process->current_priority == (unsigned int)(-1))
        printk(" -init- ");
    else
        printk("%03d(%03d)", process->current_priority, process->base_priority);
#else
    if (process->base_priority == (unsigned int)(-1))
        printk(" -init- ");
    else
        printk("%03d     ", process->base_priority);
#endif
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
    __GLOBAL->lib->pool_stat(&__KERNEL->pool, &stat, get_sp());
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
    ktimer_get_uptime(&uptime);
    printk("%3d:%02d.%03d", uptime.sec / 60, uptime.sec % 60, uptime.usec / 1000);
#endif
    printk("\n\r");
}

void kprocess_info()
{
    int cnt = 0;
    DLIST_ENUM de;
    PROCESS* cur;
#if (KERNEL_PROCESS_STAT)
    printk("\n\r    name         priority  stack  size   used         free       uptime\n\r");
#else
    printk("\n\r    name         priority  stack  size   used         free\n\r");
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
