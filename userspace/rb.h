/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
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

#define RB_ROUND(rb, pos)                                (pos >= rb->header.size ? 0 : pos)

typedef struct {
    unsigned int head, tail, size;
}RB_HEADER;

typedef struct {
    RB_HEADER header;
    char data[((unsigned int)-1) >> 1];
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
    rb->header.head = rb->header.tail = 0;
    rb->header.size = size;
}


/**
    \brief check, if ring buffer is empty
    \param rb: pointer to initialized \ref RB structure
    \retval \b true if empty
*/
__STATIC_INLINE bool rb_is_empty(RB* rb)
{
    return rb->header.head == rb->header.tail;
}

/**
    \brief check, if ring buffer is full
    \param rb: pointer to initialized \ref RB structure
    \retval \b true if full
*/
__STATIC_INLINE bool rb_is_full(RB* rb)
{
    return RB_ROUND(rb, rb->header.head + 1) == rb->header.tail;
}

/**
    \brief put item in ring buffer
    \param rb: pointer to initialized \ref RB structure
    \param c: item to put
    \retval none
*/
__STATIC_INLINE void rb_put(RB* rb, char c)
{
    rb->data[rb->header.head] = c;
    rb->header.head = RB_ROUND(rb, rb->header.head + 1);
}

/**
    \brief get item from ring buffer
    \param rb: pointer to initialized \ref RB structure
    \retval received item
*/
__STATIC_INLINE char rb_get(RB* rb)
{
    register char c = rb->data[rb->header.tail];
    rb->header.tail = RB_ROUND(rb, rb->header.tail + 1);
    return c;
}

/**
    \}
 */

#endif // RB_H
