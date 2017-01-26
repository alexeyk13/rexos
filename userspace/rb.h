/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef RB_H
#define RB_H

/** \addtogroup lib_rb ring buffer
    ring buffer routines
    \{
    \}
 */

#include "types.h"
#include "cc_macro.h"

#define RB_ROUND(rb, pos)                                ((pos) >= ((rb)->size) ? 0 : (pos))
#define RB_ROUND_BACK(rb, pos)                           ((pos) < 0 ? ((rb)->size) - 1 : (pos))

typedef struct {
    unsigned int head, tail, size;
}RB;

/** \addtogroup lib_rb ring buffer
    \{
 */

/**
    \brief initialize ring buffer structure
    \param rb: pointer to allocated \ref RB structure
    \param size: ring buffer size in bytes
    \retval none
*/
__STATIC_INLINE void rb_init(RB* rb, unsigned int size)
{
    rb->head = rb->tail = 0;
    rb->size = size;
}


/**
    \brief clear ring buffer
    \param rb: pointer to allocated \ref RB structure
    \retval none
*/
__STATIC_INLINE void rb_clear(RB* rb)
{
    rb->head = rb->tail;
}

/**
    \brief check, if ring buffer is empty
    \param rb: pointer to initialized \ref RB structure
    \retval \b true if empty
*/
__STATIC_INLINE bool rb_is_empty(RB* rb)
{
    return rb->head == rb->tail;
}

/**
    \brief check, if ring buffer is full
    \param rb: pointer to initialized \ref RB structure
    \retval \b true if full
*/
__STATIC_INLINE bool rb_is_full(RB* rb)
{
    return RB_ROUND(rb, rb->head + 1) == rb->tail;
}

/**
    \brief put item in ring buffer
    \param rb: pointer to initialized \ref RB structure
    \retval index of element from start, where need to put data
*/
__STATIC_INLINE unsigned int rb_put(RB* rb)
{
    register int offset = rb->head;
    rb->head = RB_ROUND(rb, rb->head + 1);
    return offset;
}

/**
    \brief get item from ring buffer
    \param rb: pointer to initialized \ref RB structure
    \retval index of element from where we can get data
*/
__STATIC_INLINE unsigned int rb_get(RB* rb)
{
    register int offset = rb->tail;
    rb->tail = RB_ROUND(rb, rb->tail + 1);
    return offset;
}

/**
    \brief get rb used size
    \param rb: pointer to initialized \ref RB structure
    \retval used items
*/
__STATIC_INLINE unsigned int rb_size(RB* rb)
{
    return rb->tail > rb->head ? rb->size - rb->tail + rb->head : rb->head - rb->tail;
}

/**
    \brief get rb free size
    \param rb: pointer to initialized \ref RB structure
    \retval free items
*/
__STATIC_INLINE unsigned int rb_free(RB* rb)
{
    return rb->tail > rb->head ? rb->tail - rb->head - 1: rb->size - rb->head + rb->tail - 1;
}

/**
    \}
 */

#endif // RB_H
