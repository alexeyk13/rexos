/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#include "web_node.h"
#include "web_parse.h"
#include "../../userspace/web.h"
#include "../../userspace/error.h"
#include <string.h>

void web_node_create(WEB_NODE* web_node)
{
    so_create(&web_node->items, sizeof(WEB_NODE_ITEM), 1);
    web_node->root = INVALID_HANDLE;
}

void web_node_destroy(WEB_NODE* web_node)
{
    web_node_free(web_node, web_node->root);
    so_destroy(&web_node->items);
}

static WEB_NODE_ITEM* web_node_find_child(WEB_NODE* web_node, WEB_NODE_ITEM* parent, char* name, unsigned int len)
{
    WEB_NODE_ITEM* cur;
    for(cur = so_get(&web_node->items, parent->child); cur != NULL; cur = so_get(&web_node->items, cur->next))
    {
        if (web_stricmp(WEB_OBJ_WILDCARD, 1, cur->name))
            return cur;

        if (web_stricmp(name, len, cur->name))
            return cur;
    }
    return NULL;
}

HANDLE web_node_allocate(WEB_NODE* web_node, HANDLE parent_handle, char* name, unsigned int flags)
{
    WEB_NODE_ITEM* parent;
    WEB_NODE_ITEM* cur;
    WEB_NODE_ITEM* child;
    HANDLE cur_handle;
    unsigned int len;

    len = strlen(name);
    if (parent_handle == WEB_ROOT_NODE)
    {
        if (web_node->root != INVALID_HANDLE)
        {
            error(ERROR_ALREADY_CONFIGURED);
            return INVALID_HANDLE;
        }
    }
    else
    {
        parent = so_get(&web_node->items, parent_handle);
        if (parent == NULL)
            return INVALID_HANDLE;
        if (web_node_find_child(web_node, parent, name, len) != NULL)
        {
            error(ERROR_ALREADY_CONFIGURED);
            return INVALID_HANDLE;
        }
    }

    if ((cur_handle = so_allocate(&web_node->items)) == INVALID_HANDLE)
        return INVALID_HANDLE;
    cur = so_get(&web_node->items, cur_handle);

    if ((cur->name = malloc(len + 1)) == NULL)
    {
        so_free(&web_node->items, cur_handle);
        return INVALID_HANDLE;
    }
    strcpy(cur->name, name);
    cur->self = cur_handle;
    cur->next = cur->child = INVALID_HANDLE;
    cur->flags = flags;

    if (parent_handle == WEB_ROOT_NODE)
        web_node->root = cur_handle;
    else
    {
        //re-fetch after allocate
        parent = so_get(&web_node->items, parent_handle);
        //first child
        if (parent->child == INVALID_HANDLE)
            parent->child = cur_handle;
        //add sibling
        else
        {
            for(child = so_get(&web_node->items, parent->child); child->next != INVALID_HANDLE;
                child = so_get(&web_node->items, child->next)) {}
            child->next = cur_handle;
        }
    }

    return cur_handle;
}

static void web_node_free_internal(WEB_NODE* web_node, WEB_NODE_ITEM* cur)
{
    free(cur->name);
    so_free(&web_node->items, cur->self);
}

static void web_node_free_siblings(WEB_NODE* web_node, WEB_NODE_ITEM* cur)
{
    WEB_NODE_ITEM* sibling;
    WEB_NODE_ITEM* next;

    for (sibling = cur; sibling != NULL; sibling = next)
    {
        next = so_get(&web_node->items, sibling->next);
        if (sibling->child != INVALID_HANDLE)
            web_node_free_siblings(web_node, so_get(&web_node->items, sibling->child));
        web_node_free_internal(web_node, sibling);
    }
}

void web_node_free(WEB_NODE* web_node, HANDLE handle)
{
    WEB_NODE_ITEM* cur;
    WEB_NODE_ITEM* parent;
    HANDLE h;
    bool found;
    cur = so_get(&web_node->items, handle);
    if (cur == NULL)
        return;

    //remove all childs
    if (cur->child != INVALID_HANDLE)
        web_node_free_siblings(web_node, so_get(&web_node->items, cur->child));

    //remove from parent/older brother
    if (handle != web_node->root)
    {
        found = false;
        for (h = so_first(&web_node->items); h != INVALID_HANDLE; h = so_next(&web_node->items, h))
        {
            parent = so_get(&web_node->items, h);
            if (parent->child == handle)
            {
                parent->child = cur->next;
                found = true;
                break;
            }
            if (parent->next == handle)
            {
                parent->next = cur->next;
                found = true;
                break;
            }
        }
        if (!found)
        {
            error(ERROR_NOT_FOUND);
            return;
        }

    }

    //destroy node itself
    web_node_free_internal(web_node, cur);
    //remove root item
    if (handle == web_node->root)
        web_node->root = INVALID_HANDLE;
}

HANDLE web_node_find_path(WEB_NODE* web_node, char* url, unsigned int url_size)
{
    WEB_NODE_ITEM* cur;
    unsigned int len;
    if (web_node->root == INVALID_HANDLE)
        return INVALID_HANDLE;
    if (!url_size || url[0] != '/')
        return INVALID_HANDLE;
    cur = so_get(&web_node->items, web_node->root);
    //skip "/"
    --url_size;
    ++url;
    if (url_size == 0)
        return web_node->root;
    for (;;)
    {
        len = web_get_word(url, url_size, '/');
        cur = web_node_find_child(web_node, cur, url, len);
        if (cur == NULL)
            return INVALID_HANDLE;
        if (len == url_size)
            return cur->self;
        //also skip slash
        url += len + 1;
        url_size -= len + 1;
    }
}

bool web_node_check_flag(WEB_NODE* web_node, HANDLE handle, unsigned int flag)
{
    WEB_NODE_ITEM* cur;
    cur = so_get(&web_node->items, handle);
    if (cur == NULL)
        return false;
    return cur->flags & flag ? true : false;
}
