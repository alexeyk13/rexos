/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef DLIST_H
#define DLIST_H

/** \addtogroup lib_dlist dual-linked list
    dual-linked list routines

    dual-linked list is used mainly for system objects,
    however, can be used for user lists.

    \ref DLIST is just a header, added to custom structure
    as a first element. Using cast it's possible to use
    any structure as dual-linked lists of custom objects.

    For memory-critical apllications it's possible to use
    dual-linked lists also as a containers for up to 2 bit flags
    (for 32 bit system)
    \{
 */

/*
    circular dual-linked list
 */

#include "types.h"
#include "cc_macro.h"

/*
    dual-linked list type
    aligned function suffix requires dlist items align to sizeof(int)
    aligned dlist usage is a trick to add some flags to list
*/

/** \} */ // end of lib_dlist group

typedef struct _DLIST
{
    struct _DLIST * prev;
    struct _DLIST * next;
}DLIST;

typedef struct
{
    DLIST* start;
    DLIST* cur;
    bool has_more;
} DLIST_ENUM;

/** \addtogroup lib_dlist dual-linked list
    \{
 */

#include "dlist.h"

/**
    \brief clear list
    \param dlist: address of list
    \retval: none
  */
__STATIC_INLINE void dlist_clear(DLIST** dlist)
{
    *dlist = NULL;
}

/**
    \brief check, if \b DLIST is empty
    \param dlist: address of list
    \retval: \b true if empty
  */
__STATIC_INLINE bool is_dlist_empty(DLIST** dlist)
{
    return *dlist == NULL;
}

/**
    \brief add item to \b DLIST head
    \param dlist: address of list
    \param entry: pointer to object to add
    \retval: none
  */
__STATIC_INLINE void dlist_add_head(DLIST** dlist, DLIST* entry)
{
    if (*dlist == NULL)
        entry->next = entry->prev = entry;
    else
    {
        entry->next = *dlist;
        entry->prev = (*dlist)->prev;
        (*dlist)->prev->next = entry;
        (*dlist)->prev = entry;
    }
    *dlist = entry;
}

/**
    \brief add item to \b DLIST tail
    \param dlist: address of list
    \param entry: pointer to object to add
    \retval: none
  */
__STATIC_INLINE void dlist_add_tail(DLIST** dlist, DLIST* entry)
{
    if (*dlist == NULL)
    {
        *dlist = entry;
        entry->next = entry->prev = entry;
    }
    else
    {
        entry->next = *dlist;
        entry->prev = (*dlist)->prev;
        (*dlist)->prev->next = entry;
        (*dlist)->prev = entry;
    }
}

/**
    \brief add item \b DLIST before existing item
    \param dlist: address of list
    \param before: pointer to existing object
    \param entry: pointer to object to add
    \retval: none
  */
__STATIC_INLINE void dlist_add_before(DLIST** dlist, DLIST* before, DLIST* entry)
{
    if (before == *dlist)
        dlist_add_head(dlist, entry);
    else
    {
        entry->next = before;
        entry->prev = before->prev;
        before->prev->next = entry;
        before->prev = entry;
    }
}

/**
    \brief add item \b DLIST after existing item
    \param dlist: address of list
    \param after: pointer to existing object
    \param entry: pointer to object to add
    \retval: none
  */
__STATIC_INLINE void dlist_add_after(DLIST** dlist, DLIST* after, DLIST* entry)
{
    entry->next = after->next;
    entry->prev = after;
    after->next->prev = entry;
    after->next = entry;
}

/**
    \brief remove head item from \b DLIST
    \param dlist: address of list
    \retval: none
  */
__STATIC_INLINE void dlist_remove_head(DLIST** dlist)
{
    if((*dlist)->next == *dlist)
        *dlist = NULL;
    else
    {
        (*dlist)->next->prev = (*dlist)->prev;
        (*dlist)->prev->next = (*dlist)->next;
        *dlist = (*dlist)->next;
    }
}

/**
    \brief remove tail item from \b DLIST
    \param dlist: address of list
    \retval: none
  */
__STATIC_INLINE void dlist_remove_tail(DLIST** dlist)
{
    if((*dlist)->next == *dlist)
        *dlist = NULL;
    else
    {
        (*dlist)->prev->prev->next = (*dlist);
        (*dlist)->prev = (*dlist)->prev->prev;
    }
}

/**
    \brief remove item from \b DLIST
    \details \b DLIST must contain item
    \param dlist: address of list
    \param entry: pointer to object to add
    \retval: none
  */
__STATIC_INLINE void dlist_remove(DLIST** dlist, DLIST* entry)
{
    if ((*dlist) == entry)
        dlist_remove_head(dlist);
    else
    {
        DLIST* head;
        head = *dlist;
        (*dlist) = entry;
        dlist_remove_head(dlist);
        (*dlist) = head;
    }
}

/**
    \brief go next
    \param dlist: address of list
    \retval: none
  */
__STATIC_INLINE void dlist_next(DLIST** dlist)
{
    if (!(*dlist == NULL))
        (*dlist) = (*dlist)->next;
}

/**
    \brief go previous
    \param dlist: address of list
    \retval: none
  */
__STATIC_INLINE void dlist_prev(DLIST** dlist)
{
    if (!(*dlist == NULL))
        (*dlist) = (*dlist)->prev;
}

/**
    \brief start enumeration
    \param dlist: address of list
    \param de: pointer to existing \ref DLIST_ENUM structure
    \retval: none
  */
__STATIC_INLINE void dlist_enum_start(DLIST** dlist, DLIST_ENUM* de)
{
    de->start = *dlist;
    de->cur = *dlist;
    de->has_more = !(*dlist == NULL);
}

/**
    \brief get next item
    \details enumeration must be prepared by \ref dlist_enum_start
    \param de: pointer to initialized \ref DLIST_ENUM structure
    \param cur: address of pointer to current item holder
    \retval: \b true if success, \b false after last item was fetched
  */
__STATIC_INLINE bool dlist_enum(DLIST_ENUM* de, DLIST** cur)
{
    if (!(de->has_more))
        return false;
    (*cur) = de->cur;
    de->cur = de->cur->next;
    de->has_more = (de->cur != de->start);
    return true;
}

/**
    \brief remove item, while enumeration is in progress
    \param dlist: address of list
    \param de: pointer to initialized \ref DLIST_ENUM structure
    \param cur: pointer to item to remove
    \retval: none
  */
__STATIC_INLINE void dlist_remove_current_inside_enum(DLIST** dlist, DLIST_ENUM* de, DLIST* cur)
{
    if (cur == de->start)
        de->start = de->start->next;
    dlist_remove(dlist, cur);
    if (*dlist == NULL)
        de->has_more = false;
}

/**
    \brief check, if \b DLIST is contains entry
    \param dlist: address of list
    \param entry: pointer to object to check
    \retval: \b true if contains
  */
__STATIC_INLINE bool is_dlist_contains(DLIST** dlist, DLIST* entry)
{
    DLIST_ENUM de;
    DLIST* cur;
    dlist_enum_start(dlist, &de);
    while (dlist_enum(&de, &cur))
    {
        if (cur == entry)
            return true;
    }
    return false;
}

/** \} */ // end of lib_dlist group

#endif // DLIST_H
