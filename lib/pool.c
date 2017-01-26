/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "pool.h"
#include "../userspace/error.h"
#include "../userspace/process.h"
#include <string.h>

#if (KERNEL_RANGE_CHECKING)

#define SLOT_HEADER_SIZE                                        (sizeof(void*) + sizeof(unsigned int))
#define SLOT_FOOTER_SIZE                                        (sizeof (unsigned int))

#else

#define SLOT_HEADER_SIZE                                        (sizeof(void*))
#define SLOT_FOOTER_SIZE                                        (0)

#endif //(KERNEL_RANGE_CHECKING)

#define MIN_SLOT_FULL_SIZE                                        (SLOT_HEADER_SIZE + sizeof(int) + SLOT_FOOTER_SIZE)

#define NEXT_SLOT(ptr)                                            (*(void**)((unsigned int)(ptr) - SLOT_HEADER_SIZE))
#define NEXT_FREE(ptr)                                            (*(void**)(ptr))
#define NUM(ptr)                                                    (unsigned int)(ptr)
#define ALIGN_SIZE                                                (sizeof(int))
#define ALIGN(var)                                                (((var) + (ALIGN_SIZE - 1)) & ~(ALIGN_SIZE - 1))

#if (KERNEL_RANGE_CHECKING)

static const unsigned int RANGE_MARK =                            0xcdcdcdcd;
static const unsigned int RANGE_MARK_END =                        0xdcdcdcdc;
static const unsigned int RANGE_MARK_POOL_END =                   0xeeeeeeee;

#define SET_MARK(ptr)                                             *((unsigned int*)(ptr) - 1) = RANGE_MARK; \
                                                                    if (NEXT_SLOT(ptr) != NULL) \
                                                                        *(unsigned int*)((unsigned int)NEXT_SLOT(ptr) - SLOT_HEADER_SIZE - SLOT_FOOTER_SIZE) = RANGE_MARK_END; \
                                                                    else \
                                                                        *(unsigned int*)(ptr) = RANGE_MARK_POOL_END
#define CLEAR_MARK(ptr)                                           *((unsigned int*)(ptr) - 1) = 0; \
                                                                    if (NEXT_SLOT(ptr) != NULL) \
                                                                        *(unsigned int*)((unsigned int)NEXT_SLOT(ptr) - SLOT_HEADER_SIZE - SLOT_FOOTER_SIZE) = 0; \
                                                                    else \
                                                                        *(unsigned int*)(ptr) = 0

#else

#define SET_MARK(ptr)
#define CLEAR_MARK(ptr)

#endif //(KERNEL_RANGE_CHECKING)


/*
        malloc

        data slot:
        SLOT_HEADER        <--- next slot (pointing to data AFTER SLOT_HEADER)
        <data>            <--- returned pointer
        <align to sizeof(int)>

        free slot:
        SLOT_HEADER        <--- next slot (pointing to data AFTER SLOT_HEADER)
        <free bytes>    <--- pointer to next free slot or NULL

        last slot:
        SLOT_HEADER        <--- NULL pointer here
        unused tail

*/

void pool_init(POOL* pool, void* data)
{
    // _sbrk implementation
    pool->first_slot = pool->last_slot = (void*)(ALIGN(NUM(data)) + SLOT_HEADER_SIZE);
    NEXT_SLOT(pool->first_slot) = NULL;
    SET_MARK(pool->first_slot);
    pool->free_slot = NULL;
}

static bool grow(POOL* pool, size_t size, void* sp)
{
    register void *new_last;
    if (NEXT_SLOT(pool->last_slot) != NULL)
    {
        error(ERROR_POOL_CORRUPTED);
        return false;
    }

    new_last = (void*)(NUM(pool->last_slot) + SLOT_HEADER_SIZE + size + SLOT_FOOTER_SIZE);
    //check uint overflow and compare with stack
    if (NUM(new_last) < NUM(pool->last_slot) || NUM(new_last) >= NUM(sp))
    {
        error(ERROR_OUT_OF_MEMORY);
        return false;
    }
    //_brk implementation
    CLEAR_MARK(pool->last_slot);
    NEXT_SLOT(pool->last_slot) = new_last;
    NEXT_SLOT(new_last) = NULL;
    SET_MARK(pool->last_slot);
    SET_MARK(new_last);

    pool_free(pool, pool->last_slot);
    pool->last_slot = new_last;
    return true;
}

void* pool_malloc(POOL* pool, size_t size, void* sp)
{
    size_t len;
    register void *free_before, *next_slot, *new_slot, *cur;
    int i;

    //optimize for ARM 32bit align
    len = ALIGN(size);
    if (size == 0)
        return NULL;
    if (NUM(pool->last_slot) + len < NUM(pool->last_slot))
    {
        error(ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    for (i = 0; i < 2; ++i)
    {
        //forward thru empty slots
        for (free_before = NULL, cur = pool->free_slot; cur != NULL; free_before = cur, cur = NEXT_FREE(cur))
        {
            next_slot = NEXT_SLOT(cur);
            new_slot = (void*)(NUM(cur) + SLOT_HEADER_SIZE + len + SLOT_FOOTER_SIZE);
            //enough space (before next slot)?
            if (NUM(new_slot) <= NUM(next_slot))
            {
                //enough space to split and make free slot after
                if (NUM(new_slot) + MIN_SLOT_FULL_SIZE <= NUM(next_slot))
                {
                    CLEAR_MARK(cur);
                    NEXT_SLOT(new_slot) = next_slot;
                    NEXT_SLOT(cur) = new_slot;

                    NEXT_FREE(new_slot) = NEXT_FREE(cur);
                    NEXT_FREE(cur) = new_slot;
                    SET_MARK(cur);
                    SET_MARK(new_slot);
                }
                //remove current from free slot list
                if (free_before)
                    NEXT_FREE(free_before) = NEXT_FREE(cur);
                else
                    pool->free_slot = NEXT_FREE(cur);
                return cur;
            }
        }
        //try to allocate more space
        if (!grow(pool, len, sp))
            break;
    }
    return NULL;
}

size_t pool_slot_size(POOL* poll, void* ptr)
{
    if (ptr == NULL)
        return 0;
    return NUM(NEXT_SLOT(ptr)) - NUM(ptr) - SLOT_HEADER_SIZE - SLOT_FOOTER_SIZE;
}

void* pool_realloc(POOL* pool, void* ptr, size_t size, void *sp)
{
    register void *next, *p, *n;
    void *res;
    unsigned int cur_size;
    unsigned int len = ALIGN(size);
    int i;

    if (ptr == NULL)
        return pool_malloc(pool, len, sp);
    next = NEXT_SLOT(ptr);
    if (len == 0)
    {
        pool_free(pool, ptr);
        return NULL;
    }
    cur_size = pool_slot_size(pool, ptr);

    for (i = 0; i < 2; ++i)
    {
        //next is free? append!
        for (p = NULL, n = pool->free_slot; n && n <= next; p = n, n = NEXT_FREE(n))
        {
            if (n == next)
            {
                CLEAR_MARK(ptr);
                CLEAR_MARK(next);
                NEXT_SLOT(ptr) = NEXT_SLOT(next);
                if (p)
                    NEXT_FREE(p) = NEXT_FREE(n);
                else
                    pool->free_slot = NEXT_FREE(n);
                next = NEXT_SLOT(ptr);
                SET_MARK(ptr);
                break;
            }
        }

        //at end of pool? grow!
        if (len <= cur_size || next != pool->last_slot)
            break;
        if (!grow(pool, len - cur_size, sp))
            break;
    }

    //slot enough size?
    n = (void*)(NUM(ptr) + SLOT_HEADER_SIZE + SLOT_FOOTER_SIZE + len);
    if (n <= next)
    {
        //free space at end. Split slot
        if (NUM(next) - NUM(n) >= MIN_SLOT_FULL_SIZE)
        {
            CLEAR_MARK(ptr);
            NEXT_SLOT(n) = next;
            NEXT_SLOT(ptr) = n;
            SET_MARK(ptr);
            SET_MARK(n);
            pool_free(pool, n);
        }
        return ptr;
    }

    //can't extend. Allocate in other place and copy.
    res = pool_malloc(pool, size, sp);
    if (res)
    {
        memcpy(res, ptr, cur_size);
        pool_free(pool, ptr);
    }
    return res;
}

void pool_free(POOL* pool, void* ptr)
{
    register void* free_before;
    register void* free_after;

    if (ptr == NULL)
        return;

    //find free slots before and after our ptr
    for (free_before = NULL, free_after = pool->free_slot; free_after != NULL && NUM(ptr) > NUM(free_after); free_before = free_after, free_after = NEXT_FREE(free_after)) {}

    if (
         //pointer in free slots list?
         NUM(free_after) == NUM(ptr)
         //next after current slot is broken?
         || NUM(NEXT_SLOT(ptr)) <= NUM(ptr)
         //next free slot before next slot?
         || (free_after != NULL && NUM(NEXT_SLOT(ptr)) > NUM(free_after))
         //next after free slot before before current?
         || (free_before != NULL && NUM(NEXT_SLOT(free_before)) > NUM(ptr)))
    {
        error(ERROR_POOL_CORRUPTED);
        return;
    }

    NEXT_FREE(ptr) = free_after;
    if (free_before != NULL)
        NEXT_FREE(free_before) = ptr;
    //free first slot
    else
        pool->free_slot = ptr;

    //next is also free?
    if (free_after && NEXT_SLOT(ptr) == free_after)
    {
        CLEAR_MARK(ptr);
        CLEAR_MARK(free_after);
        NEXT_SLOT(ptr) = NEXT_SLOT(free_after);
        NEXT_FREE(ptr) = NEXT_FREE(free_after);
        free_after = NEXT_FREE(free_after);
        SET_MARK(ptr);
    }

    //before is also free?
    if (free_before && NEXT_SLOT(free_before) == ptr)
    {
        CLEAR_MARK(free_before);
        CLEAR_MARK(ptr);
        NEXT_SLOT(free_before) = NEXT_SLOT(ptr);
        NEXT_FREE(free_before) = free_after;
        SET_MARK(free_before);
    }
}

#if (KERNEL_PROFILING)

void* pool_free_ptr(POOL* pool)
{
    return pool->last_slot + SLOT_HEADER_SIZE;
}

bool pool_check(POOL* pool, void* sp)
{
    register void *before, *cur;
    //basic check
    if (pool->first_slot == NULL || pool->last_slot == NULL ||
         NUM(pool->first_slot) > NUM(pool->last_slot) ||
         NUM(pool->free_slot) > NUM(pool->last_slot) || NEXT_SLOT(pool->last_slot) != NULL)
    {
        error(ERROR_POOL_CORRUPTED);
        return false;
    }
    if (NUM(sp) < NUM(pool->last_slot))
    {
        error(ERROR_OUT_OF_MEMORY);
        return false;
    }
    //check all slots first
    for (before = NULL, cur = pool->first_slot; cur != NULL; before = cur, cur = NEXT_SLOT(cur))
    {
        if (NUM(cur) < NUM(before) || NUM(cur) > NUM(pool->last_slot))
        {
            error(ERROR_POOL_CORRUPTED);
            return false;
        }
#if (KERNEL_RANGE_CHECKING)
        //check header
        if (*((unsigned int*)(cur) - 1) != RANGE_MARK)
        {
            error(ERROR_POOL_RANGE_CHECK_FAILED);
            return false;
        }

        if (NEXT_SLOT(cur))
        {
            //check footer
            if (*(unsigned int*)((unsigned int)NEXT_SLOT(cur) - SLOT_HEADER_SIZE - SLOT_FOOTER_SIZE) != RANGE_MARK_END)
            {
                error(ERROR_POOL_RANGE_CHECK_FAILED);
                return false;
            }
        }
        //last slot
        else
        {
            if (*(unsigned int*)(cur) != RANGE_MARK_POOL_END)
            {
                error(ERROR_POOL_RANGE_CHECK_FAILED);
                return false;
            }
        }
#endif //(KERNEL_RANGE_CHECKING)
    }

    //check free slots
    for (before = NULL, cur = pool->free_slot; cur != NULL; before = cur, cur = NEXT_FREE(cur))
    {
        if (NUM(cur) < NUM(before) || NUM(cur) > NUM(pool->last_slot))
        {
            error(ERROR_POOL_CORRUPTED);
            return false;
        }
#if (KERNEL_RANGE_CHECKING)
        //check header
        if (*((unsigned int*)(cur) - 1) != RANGE_MARK)
        {
            error(ERROR_POOL_RANGE_CHECK_FAILED);
            return false;
        }

        if (NEXT_SLOT(cur))
        {
            //check footer
            if (*(unsigned int*)((unsigned int)NEXT_SLOT(cur) - SLOT_HEADER_SIZE - SLOT_FOOTER_SIZE) != RANGE_MARK_END)
            {
                error(ERROR_POOL_RANGE_CHECK_FAILED);
                return false;
            }
        }
        //last slot
        else
        {
            if (*(unsigned int*)(cur) != RANGE_MARK_POOL_END)
            {
                error(ERROR_POOL_RANGE_CHECK_FAILED);
                return false;
            }
        }
#endif //(KERNEL_RANGE_CHECKING)
    }
    return true;
}

void pool_stat(POOL* pool, POOL_STAT* stat, void* sp)
{
    void *cur, *cur_free;
    unsigned int size;
    memset(stat, 0, sizeof(POOL_STAT));
    if (pool_check(pool, sp))
    {
        cur_free = pool->free_slot;
        for (cur = pool->first_slot; cur != pool->last_slot; cur = NEXT_SLOT(cur))
        {
            size = NUM(NEXT_SLOT(cur)) - NUM(cur) - SLOT_HEADER_SIZE - SLOT_FOOTER_SIZE;
            //it's free slot?
            if (cur == cur_free)
            {
                ++stat->free_slots;
                if (size > stat->largest_free)
                    stat->largest_free = size;
                stat->free += size;
                cur_free = NEXT_FREE(cur_free);
            }
            else
            {
                ++stat->used_slots;
                stat->used += size;
            }
        }
    }
    //space between last_slot and sp possibly can grow
    if (NUM(sp) >= NUM(pool->last_slot) + sizeof(unsigned int) + MIN_SLOT_FULL_SIZE)
    {
        size = NUM(sp) - NUM(pool->last_slot) - MIN_SLOT_FULL_SIZE;
        ++stat->free_slots;
        if (size > stat->largest_free)
            stat->largest_free = size;
        stat->free += size;
    }
}

#endif //KERNEL_PROFILING
