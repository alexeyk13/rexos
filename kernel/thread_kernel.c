/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#include "thread_kernel.h"
#include "kernel_config.h"
#include "mem_kernel.h"
#include <stddef.h>
#include "string.h"
#include "mutex_kernel.h"
#include "event_kernel.h"
#include "sem_kernel.h"
#include "core/core_kernel.h"
#include "../userspace/core/sys_calls.h"
#include "../userspace/core/core.h"
#include "../userspace/error.h"
#include "../lib/pool.h"
#include "magic.h"
#if (KERNEL_PROFILING)
#include "memmap.h"
#endif //KERNEL_PROFILING

#define MAX_PROCESS_NAME_SIZE                                   128

#define THREAD_MODE_MASK                                        0x3

#define THREAD_MODE_FROZEN                                      (0 << 0)
#define THREAD_MODE_RUNNING                                     (1 << 0)
#define THREAD_MODE_WAITING                                     (2 << 0)
#define THREAD_MODE_WAITING_FROZEN                              (3 << 0)
#define THREAD_MODE_WAITING_SYNC_OBJECT                         (1 << 1)

#define THREAD_TIMER_ACTIVE                                     (1 << 2)

#define THREAD_SYNC_MASK                                        (0xf << 4)

#define THREAD_NAME_PRINT_SIZE                                  16

#if (KERNEL_PROFILING)
extern void svc_stack_stat();
#endif //KERNEL_PROFILING

#define IDLE_PRIORITY                                           ((unsigned int)-1)

void svc_thread_timeout(void* param)
{
    CRITICAL_ENTER;
    THREAD* thread = param;
    thread->flags &= ~THREAD_TIMER_ACTIVE;
    //say sync object to release us
    switch (thread->flags & THREAD_SYNC_MASK)
    {
    case THREAD_SYNC_TIMER_ONLY:
        break;
    case THREAD_SYNC_MUTEX:
        svc_mutex_lock_release((MUTEX*)thread->sync_object, thread);
        break;
    case THREAD_SYNC_EVENT:
        svc_event_lock_release((EVENT*)thread->sync_object, thread);
        break;
    case THREAD_SYNC_SEM:
        svc_sem_lock_release((SEM*)thread->sync_object, thread);
        break;
    default:
        ASSERT(false);
    }
    if ((thread->flags & THREAD_SYNC_MASK) != THREAD_SYNC_TIMER_ONLY)
        svc_thread_error(thread, ERROR_TIMEOUT);
    svc_thread_wakeup(thread);
    CRITICAL_LEAVE;
}

void svc_thread_create(THREAD_CALL* tc, THREAD** thread)
{
    *thread = sys_alloc(sizeof(THREAD));
    memset((*thread), 0, sizeof(THREAD));
    //allocate thread object
    if (*thread != NULL)
    {
        (*thread)->heap = stack_alloc(tc->heap_size * sizeof(unsigned int));
        if ((*thread)->heap)
        {
            DO_MAGIC((*thread), MAGIC_THREAD);
            (*thread)->base_priority = (*thread)->current_priority = tc->priority;
            (*thread)->sp = (void*)((unsigned int)(*thread)->heap + tc->heap_size * sizeof(unsigned int));
            (*thread)->timer.callback = svc_thread_timeout;
            (*thread)->timer.param = (*thread);

#if (KERNEL_PROFILING)
            (*thread)->heap_size = tc->heap_size;
            memset((*thread)->heap, MAGIC_UNINITIALIZED_BYTE, (*thread)->heap_size * sizeof(unsigned int));
#endif //KERNEL_PROFILING
            (*thread)->heap->handle = (HANDLE)(*thread);
            if (tc->name)
            {
                strncpy(PROCESS_NAME((*thread)->heap), tc->name, MAX_PROCESS_NAME_SIZE);
                PROCESS_NAME((*thread)->heap)[MAX_PROCESS_NAME_SIZE] = 0;
            }
            else
                strcpy(PROCESS_NAME((*thread)->heap), "UNNAMED PROCESS");
            (*thread)->heap->struct_size = sizeof(HEAP) + strlen(PROCESS_NAME((*thread)->heap)) + 1;
            pool_init(&(*thread)->heap->pool, (void*)(*thread)->heap + (*thread)->heap->struct_size);

            thread_setup_context((*thread), tc->fn);
        }
        else
        {
            sys_free(*thread);
            (*thread) = NULL;
            error(ERROR_OUT_OF_PAGED_MEMORY);
        }
    }
    else
        error(ERROR_OUT_OF_SYSTEM_MEMORY);
}

static inline void push_last_in_list()
{
    if (__KERNEL->thread_list_size == THREAD_CACHE_SIZE)
    {
        DLIST* cur;
        while (__KERNEL->active_threads[THREAD_CACHE_SIZE - 1] != NULL)
        {
            cur = __KERNEL->active_threads[THREAD_CACHE_SIZE - 1]->list.prev;
            dlist_remove_tail((DLIST**)&(__KERNEL->active_threads[THREAD_CACHE_SIZE - 1]));
            dlist_add_head((DLIST**)&__KERNEL->threads_uncached, cur);
        }
    }
    else
        ++__KERNEL->thread_list_size;
}

static inline void pop_last_from_list()
{
    if (__KERNEL->threads_uncached != NULL)
    {
        __KERNEL->active_threads[THREAD_CACHE_SIZE - 1] = NULL;
        int priority = __KERNEL->threads_uncached->current_priority;
        THREAD* cur;
        while (__KERNEL->threads_uncached != NULL && __KERNEL->threads_uncached->current_priority == priority)
        {
            cur = __KERNEL->threads_uncached;
            dlist_remove_head((DLIST**)&__KERNEL->threads_uncached);
            dlist_add_tail((DLIST**)&(__KERNEL->active_threads[__KERNEL->thread_list_size]), (DLIST*)cur);
        }
    }
    else
    {
        --__KERNEL->thread_list_size;
    }
}

void thread_add_to_active_list(THREAD* thread)
{
    THREAD* thread_to_save = thread;
    //thread priority is less, than active, activate him
    if (thread->current_priority < __KERNEL->current_thread->current_priority)
    {
#if (KERNEL_PROFILING)
//        svc_get_uptime(&thread->uptime_start);
//      time_sub(&_current_thread->uptime_start, &thread->uptime_start, &_current_thread->uptime_start);
//        time_add(&_current_thread->uptime_start, &_current_thread->uptime, &_current_thread->uptime);
#endif //KERNEL_PROFILING

        thread_to_save = __KERNEL->current_thread;
        __KERNEL->current_thread = thread;
        __GLOBAL->heap = __KERNEL->current_thread->heap;
        __KERNEL->next_thread = __KERNEL->current_thread;
    }
    //first - look at cache
    int pos = 0;
    if (__KERNEL->thread_list_size)
    {
        int first = 0;
        int last = __KERNEL->thread_list_size - 1;
        int mid;
        while (first < last)
        {
            mid = (first + last) >> 1;
            if (__KERNEL->active_threads[mid]->current_priority < thread_to_save->current_priority)
                first = mid + 1;
            else
                last = mid;
        }
        pos = first;
        if (__KERNEL->active_threads[pos]->current_priority < thread_to_save->current_priority)
            ++pos;
    }

    //we have space in cache?
    if (pos < THREAD_CACHE_SIZE)
    {
        //does we have active thread with same priority?
        if (!(__KERNEL->active_threads[pos] != NULL && __KERNEL->active_threads[pos]->current_priority == thread_to_save->current_priority))
        {
            //last list is going out ouf cache
            push_last_in_list();
            memmove(__KERNEL->active_threads + pos + 1, __KERNEL->active_threads + pos, (__KERNEL->thread_list_size - pos - 1) * sizeof(void*));
            __KERNEL->active_threads[pos] = NULL;
        }
        dlist_add_tail((DLIST**)&__KERNEL->active_threads[pos], (DLIST*)thread_to_save);
    }
    //find and allocate timer on uncached list
    else
    {
        //top
        if (__KERNEL->threads_uncached == NULL || thread_to_save->current_priority < __KERNEL->threads_uncached->current_priority)
            dlist_add_head((DLIST**)&__KERNEL->threads_uncached, (DLIST*)thread_to_save);
        //bottom
        else if (thread_to_save->current_priority >= ((THREAD*)__KERNEL->threads_uncached->list.prev)->current_priority)
            dlist_add_tail((DLIST**)&__KERNEL->threads_uncached, (DLIST*)thread_to_save);
        //in the middle
        else
        {
            DLIST_ENUM de;
            THREAD* cur;
            dlist_enum_start((DLIST**)&__KERNEL->threads_uncached, &de);
            while (dlist_enum(&de, (DLIST**)&cur))
                if (cur->current_priority < thread_to_save->current_priority)
                {
                    dlist_add_before((DLIST**)&__KERNEL->threads_uncached, (DLIST*)cur, (DLIST*)thread_to_save);
                    break;
                }
        }
    }
}

void thread_remove_from_active_list(THREAD* thread)
{
    int pos = 0;
    //freeze active task
    if (thread == __KERNEL->current_thread)
    {
#if (KERNEL_PROFILING)
///        svc_get_uptime(&_active_threads[0]->uptime_start);
///        time_sub(&_current_thread->uptime_start, &_active_threads[0]->uptime_start, &_current_thread->uptime_start);
///        time_add(&_current_thread->uptime_start, &_current_thread->uptime, &_current_thread->uptime);
#endif //KERNEL_PROFILING
        __KERNEL->current_thread = __KERNEL->active_threads[0];
        __GLOBAL->heap = __KERNEL->current_thread->heap;
        __KERNEL->next_thread = __KERNEL->current_thread;
    }
    //try to search in cache
    else
    {
        int first = 0;
        int last = __KERNEL->thread_list_size - 1;
        int mid;
        while (first < last)
        {
            mid = (first + last) >> 1;
            if (__KERNEL->active_threads[mid]->current_priority < thread->current_priority)
                first = mid + 1;
            else
                last = mid;
        }
        pos = first;
        if (__KERNEL->active_threads[pos]->current_priority < thread->current_priority)
            ++pos;
    }

    if (pos < THREAD_CACHE_SIZE)
    {
        dlist_remove_head((DLIST**)&(__KERNEL->active_threads[pos]));

        //removed all at current priority level
        if (__KERNEL->active_threads[pos] == NULL)
        {
            memmove(__KERNEL->active_threads + pos, __KERNEL->active_threads + pos + 1, (__KERNEL->thread_list_size - pos - 1) * sizeof(void*));
            //restore to cache from list
            pop_last_from_list();
        }
    }
    //remove from uncached
    else
        dlist_remove((DLIST**)&__KERNEL->threads_uncached, (DLIST*)thread);
}

static inline void svc_thread_unfreeze(THREAD* thread)
{
    CHECK_MAGIC(thread, MAGIC_THREAD);
    switch (thread->flags & THREAD_MODE_MASK)
    {
    case THREAD_MODE_FROZEN:
        thread_add_to_active_list(thread);
        if (__KERNEL->next_thread)
            pend_switch_context();
        thread->flags &= ~THREAD_MODE_MASK;
        thread->flags |= THREAD_MODE_RUNNING;
        break;
    case THREAD_MODE_WAITING_FROZEN:
        thread->flags &= ~THREAD_MODE_MASK;
        thread->flags |= THREAD_MODE_WAITING;
        break;
    }
}

static inline void svc_thread_freeze(THREAD* thread)
{
    CHECK_MAGIC(thread, MAGIC_THREAD);
    switch (thread->flags & THREAD_MODE_MASK)
    {
    case THREAD_MODE_RUNNING:
        thread_remove_from_active_list(thread);
        if (__KERNEL->next_thread)
            pend_switch_context();
        thread->flags &= ~THREAD_MODE_MASK;
        thread->flags |= THREAD_MODE_FROZEN;
        break;
    case THREAD_MODE_WAITING:
        thread->flags &= ~THREAD_MODE_MASK;
        thread->flags |= THREAD_MODE_WAITING_FROZEN;
        break;
    }
}

void svc_thread_error(THREAD* thread, int error)
{
    CHECK_CONTEXT(SUPERVISOR_CONTEXT | IRQ_CONTEXT);
    thread->heap->error = error;
}

THREAD* svc_thread_get_current()
{
    return __KERNEL->current_thread;
}

void svc_thread_set_current_priority(THREAD* thread, unsigned int priority)
{
    CHECK_CONTEXT(SUPERVISOR_CONTEXT | IRQ_CONTEXT);
    CHECK_MAGIC(thread, MAGIC_THREAD);
    if (thread->current_priority != priority)
    {
        switch (thread->flags & THREAD_MODE_MASK)
        {
        case THREAD_MODE_RUNNING:
            thread_remove_from_active_list(thread);
            thread->current_priority = priority;
            thread_add_to_active_list(thread);
            if (__KERNEL->next_thread)
                pend_switch_context();
            break;
        //if we are waiting for mutex, adjusting priority can affect on mutex owner
        case THREAD_MODE_WAITING:
        case THREAD_MODE_WAITING_FROZEN:
            thread->current_priority = priority;
            if ((thread->flags & THREAD_SYNC_MASK) == THREAD_SYNC_MUTEX)
                svc_thread_set_current_priority(((MUTEX*)thread->sync_object)->owner, svc_mutex_calculate_owner_priority(((MUTEX*)thread->sync_object)->owner));
            break;
        default:
            thread->current_priority = priority;
        }
    }
}

void svc_thread_restore_current_priority(THREAD* thread)
{
    svc_thread_set_current_priority(thread, thread->base_priority);
}

static inline void svc_thread_set_priority(THREAD* thread, unsigned int priority)
{
    thread->base_priority = priority;
    svc_thread_set_current_priority(thread, svc_mutex_calculate_owner_priority(thread));
}

static inline void svc_thread_get_priority(THREAD* thread, unsigned int* priority)
{
    *priority = thread->base_priority;
}

static void svc_thread_destroy(THREAD* thread)
{
    CHECK_MAGIC(thread, MAGIC_THREAD);
    //we cannot destroy IDLE thread
    if (thread == __KERNEL->idle_thread)
    {
#if (KERNEL_DEBUG)
        printf("IDLE thread cannot be destroyed\n\r");
#endif
        panic();
    }
    //if thread is running, freeze it first
    if ((thread->flags & THREAD_MODE_MASK)    == THREAD_MODE_RUNNING)
    {
        thread_remove_from_active_list(thread);
        //we don't need to save context on exit
        if (__KERNEL->active_thread == thread)
            __KERNEL->active_thread = NULL;
        if (__KERNEL->next_thread)
            pend_switch_context();
    }
    //if thread is owned by any sync object, release them
    if  (thread->flags & THREAD_MODE_WAITING_SYNC_OBJECT)
    {
        //if timer is still active, kill him
        if (thread->flags & THREAD_TIMER_ACTIVE)
            svc_sys_timer_destroy(&thread->timer);
        //say sync object to release us
        switch (thread->flags & THREAD_SYNC_MASK)
        {
        case THREAD_SYNC_TIMER_ONLY:
            break;
        case THREAD_SYNC_MUTEX:
            svc_mutex_lock_release((MUTEX*)thread->sync_object, thread);
            break;
        case THREAD_SYNC_EVENT:
            svc_event_lock_release((EVENT*)thread->sync_object, thread);
            break;
        case THREAD_SYNC_SEM:
            svc_sem_lock_release((SEM*)thread->sync_object, thread);
            break;
        default:
            ASSERT(false);
        }
    }
    //release memory, occupied by thread
    stack_free(thread->heap);
    sys_free(thread);
}

void svc_thread_destroy_current()
{
    svc_thread_destroy(__KERNEL->current_thread);
}

void svc_thread_sleep(TIME* time, THREAD_SYNC_TYPE sync_type, void *sync_object)
{
    CHECK_CONTEXT(SUPERVISOR_CONTEXT);
    THREAD* thread = __KERNEL->current_thread;
    CHECK_MAGIC(thread, MAGIC_THREAD);
    //idle thread cannot sleep or be locked by mutex
    if (thread == __KERNEL->idle_thread)
    {
#if (KERNEL_DEBUG)
        printf("IDLE thread cannot sleep\n\r");
#endif
        panic();
    }

    thread_remove_from_active_list(thread);
    pend_switch_context();
    thread->flags &= ~(THREAD_MODE_MASK | THREAD_SYNC_MASK);
    thread->flags |= THREAD_MODE_WAITING | sync_type;
    thread->sync_object = sync_object;

    //adjust owner priority
    if (sync_type == THREAD_SYNC_MUTEX && ((MUTEX*)sync_object)->owner->current_priority > thread->current_priority)
        svc_thread_set_current_priority(((MUTEX*)sync_object)->owner, thread->current_priority);

    //create timer if not infinite
    if (time->sec || time->usec)
    {
        thread->flags |= THREAD_TIMER_ACTIVE;
        thread->timer.time.sec = time->sec;
        thread->timer.time.usec = time->usec;
        svc_sys_timer_create(&thread->timer);
    }
}

void svc_thread_wakeup(THREAD* thread)
{
    CHECK_CONTEXT(SUPERVISOR_CONTEXT | IRQ_CONTEXT);
    CHECK_MAGIC(thread, MAGIC_THREAD);
    if  (thread->flags & THREAD_MODE_WAITING_SYNC_OBJECT)
    {
        //if timer is still active, kill him
        if (thread->flags & THREAD_TIMER_ACTIVE)
            svc_sys_timer_destroy(&thread->timer);
        thread->flags &= ~(THREAD_TIMER_ACTIVE | THREAD_SYNC_MASK);
        thread->sync_object = NULL;

        switch (thread->flags & THREAD_MODE_MASK)
        {
        case THREAD_MODE_WAITING_FROZEN:
            thread->flags &= ~THREAD_MODE_MASK;
            thread->flags |= THREAD_MODE_FROZEN;
            break;
        case THREAD_MODE_WAITING:
            thread_add_to_active_list(thread);
            if (__KERNEL->next_thread)
                pend_switch_context();
            thread->flags &= ~THREAD_MODE_MASK;
            thread->flags |= THREAD_MODE_RUNNING;
            break;
        }
    }
}

#if (KERNEL_PROFILING)
static inline void svc_thread_switch_test()
{
    THREAD* thread = __KERNEL->current_thread;
    thread_remove_from_active_list(thread);
    thread_add_to_active_list(thread);
    pend_switch_context();
    //next thread is now same as active thread, it will simulate context switching
}

static inline unsigned int stack_used_max(unsigned int top, unsigned int cur)
{
    unsigned int i;
    unsigned int last = cur;
    for (i = cur - sizeof(unsigned int); i >= top; i -= 4)
        if (*(unsigned int*)i != MAGIC_UNINITIALIZED)
            last = i;
    return last;
}

void thread_print_stat(THREAD* thread)
{
    char thread_name[THREAD_NAME_PRINT_SIZE + 1];
    TIME thread_uptime;
    //format name
    int i;
    thread_name[THREAD_NAME_PRINT_SIZE] = 0;
    memset(thread_name, ' ', THREAD_NAME_PRINT_SIZE);
    strncpy(thread_name, PROCESS_NAME(thread->heap), THREAD_NAME_PRINT_SIZE);
    for (i = 0; i < THREAD_NAME_PRINT_SIZE; ++i)
        if (thread_name[i] == 0)
            thread_name[i] = ' ';
    printf("%s ", thread_name);

    //format priority
    if (thread->current_priority == IDLE_PRIORITY)
        printf("-idle-   ");
    else
        printf("%2d(%2d)   ", thread->current_priority, thread->base_priority);

    //stack size
    unsigned int current_stack, max_stack;
    current_stack = thread->heap_size - ((unsigned int)thread->sp - (unsigned int)thread->heap) / sizeof(unsigned int);
    max_stack = thread->heap_size - (stack_used_max((unsigned int)thread->heap, (unsigned int)thread->sp) - (unsigned int)thread->heap) / sizeof(unsigned int);
    printf("%3d/%3d/%3d   ", current_stack, max_stack, thread->heap_size);

    //uptime, including time for current thread
    if (thread == __KERNEL->current_thread)
    {
//        get_uptime(&thread_uptime);
        time_sub(&thread->uptime_start, &thread_uptime, &thread_uptime);
        time_add(&thread_uptime, &thread->uptime, &thread_uptime);
    }
    else
    {
        thread_uptime.sec = thread->uptime.sec;
        thread_uptime.usec = thread->uptime.usec;
    }
    printf("%3d:%02d.%03d\n\r", thread_uptime.sec / 60, thread_uptime.sec % 60, thread->uptime.usec / 1000);
}

static inline void svc_thread_stat()
{
    int active_threads_count = 0;
    int i;
    DLIST_ENUM de;
    THREAD* cur;
    printf("    name        priority      stack        uptime\n\r");
    printf("----------------------------------------------------\n\r");
    //current
    thread_print_stat(__KERNEL->current_thread);
    ++active_threads_count;
    //in cache
    for (i = 0; i < __KERNEL->thread_list_size; ++i)
    {
        dlist_enum_start((DLIST**)&__KERNEL->active_threads[i], &de);
        while (dlist_enum(&de, (DLIST**)&cur))
        {
            thread_print_stat(cur);
            ++active_threads_count;
        }
    }
    //out of cache
    dlist_enum_start((DLIST**)&__KERNEL->threads_uncached, &de);
    while (dlist_enum(&de, (DLIST**)&cur))
    {
        thread_print_stat(cur);
        ++active_threads_count;
    }
    printf("total %d threads active\n\r", active_threads_count);
}
#endif //KERNEL_PROFILING

void svc_thread_handler(unsigned int num, unsigned int param1, unsigned int param2)
{
    CHECK_CONTEXT(SUPERVISOR_CONTEXT | IRQ_CONTEXT);
    CRITICAL_ENTER;
    switch (num)
    {
    case SYS_CALL_THREAD_CREATE:
        svc_thread_create((THREAD_CALL*)param1, (THREAD**)param2);
        break;
    case SYS_CALL_THREAD_UNFREEZE:
        svc_thread_unfreeze((THREAD*)param1);
        break;
    case SYS_CALL_THREAD_FREEZE:
        svc_thread_freeze((THREAD*)param1);
        break;
    case SYS_CALL_THREAD_GET_PRIORITY:
        svc_thread_get_priority((THREAD*)param1, (unsigned int*)param2);
        break;
    case SYS_CALL_THREAD_SET_PRIORITY:
        svc_thread_set_priority((THREAD*)param1, (unsigned int)param2);
        break;
    case SYS_CALL_THREAD_DESTROY:
        svc_thread_destroy((THREAD*)param1);
        break;
    case SYS_CALL_THREAD_SLEEP:
        svc_thread_sleep((TIME*)param1, THREAD_SYNC_TIMER_ONLY, NULL);
        break;
#if (KERNEL_PROFILING)
    case SYS_CALL_THREAD_SWITCH_TEST:
        svc_thread_switch_test();
        break;
    case SYS_CALL_THREAD_STAT:
        svc_thread_stat();
        break;
    case SYS_CALL_STACK_STAT:
        svc_stack_stat();
        break;
#endif //KERNEL_PROFILING
    default:
        error(ERROR_INVALID_SYS_CALL);
    }
    CRITICAL_LEAVE;
}

void thread_init()
{
    THREAD_CALL tc;
    tc.name = "IDLE THREAD";
    tc.priority = IDLE_PRIORITY;
    tc.heap_size = THREAD_IDLE_STACK_SIZE;
    tc.fn = (THREAD_FUNCTION)idle_task;
    svc_thread_create(&tc, &__KERNEL->idle_thread);

    //activate idle_thread
    __KERNEL->idle_thread->flags = THREAD_MODE_RUNNING;
    __KERNEL->current_thread = __KERNEL->idle_thread;
    __GLOBAL->heap = __KERNEL->current_thread->heap;
    __KERNEL->next_thread = __KERNEL->current_thread;
    pend_switch_context();
}

void abnormal_exit()
{
    register THREAD* thread;
    thread = (THREAD*)thread_get_current();
#if (KERNEL_DEBUG)
    printf("Warning: abnormal thread termination: %s\n\r", PROCESS_NAME(thread->heap));
#endif
    svc_thread_destroy(thread);
}
