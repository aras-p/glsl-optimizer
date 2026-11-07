/*
 * Copyright © 2008, 2010 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/**
 * \file list.h
 * \brief Doubly-linked list abstract container type.
 *
 * Each doubly-linked list has a sentinel head and tail node.  These nodes
 * contain no data.  The head sentinel can be identified by its \c prev
 * pointer being \c NULL.  The tail sentinel can be identified by its
 * \c next pointer being \c NULL.
 *
 * A list is empty if either the head sentinel's \c next pointer points to the
 * tail sentinel or the tail sentinel's \c prev poiner points to the head
 * sentinel. The head sentinel and tail sentinel nodes are allocated within the
 * list structure.
 *
 * Do note that this means that the list nodes will contain pointers into the
 * list structure itself and as a result you may not \c realloc() an  \c
 * exec_list or any structure in which an \c exec_list is embedded.
 */

#ifndef LIST_CONTAINER_H
#define LIST_CONTAINER_H

#include <assert.h>

#include "util/ralloc.h"

struct exec_node {
   struct exec_node *next;
   struct exec_node *prev;
};

static inline void
exec_node_init(struct exec_node *n)
{
   n->next = NULL;
   n->prev = NULL;
}

static inline const struct exec_node *
exec_node_get_next_const(const struct exec_node *n)
{
   return n->next;
}

static inline struct exec_node *
exec_node_get_next(struct exec_node *n)
{
   return n->next;
}

static inline const struct exec_node *
exec_node_get_prev_const(const struct exec_node *n)
{
   return n->prev;
}

static inline struct exec_node *
exec_node_get_prev(struct exec_node *n)
{
   return n->prev;
}

static inline void
exec_node_remove(struct exec_node *n)
{
   n->next->prev = n->prev;
   n->prev->next = n->next;
   n->next = NULL;
   n->prev = NULL;
}

static inline void
exec_node_self_link(struct exec_node *n)
{
   n->next = n;
   n->prev = n;
}

static inline void
exec_node_insert_after(struct exec_node *n, struct exec_node *after)
{
   after->next = n->next;
   after->prev = n;

   n->next->prev = after;
   n->next = after;
}

static inline void
exec_node_insert_node_before(struct exec_node *n, struct exec_node *before)
{
   before->next = n;
   before->prev = n->prev;

   n->prev->next = before;
   n->prev = before;
}

static inline bool
exec_node_is_tail_sentinel(const struct exec_node *n)
{
   return n->next == NULL;
}

static inline bool
exec_node_is_head_sentinel(const struct exec_node *n)
{
   return n->prev == NULL;
}

#ifdef __cplusplus
/* This macro will not work correctly if `t' uses virtual inheritance. */
#define exec_list_offsetof(t, f, p) \
   (((char *) &((t *) p)->f) - ((char *) p))
#else
#define exec_list_offsetof(t, f, p) offsetof(t, f)
#endif

/**
 * Get a pointer to the structure containing an exec_node
 *
 * Given a pointer to an \c exec_node embedded in a structure, get a pointer to
 * the containing structure.
 *
 * \param type  Base type of the structure containing the node
 * \param node  Pointer to the \c exec_node
 * \param field Name of the field in \c type that is the embedded \c exec_node
 */
#define exec_node_data(type, node, field) \
   ((type *) (((uintptr_t) node) - exec_list_offsetof(type, field, node)))

struct exec_list {
   struct exec_node head_sentinel;
   struct exec_node tail_sentinel;
};

static inline void
exec_list_make_empty(struct exec_list *list)
{
   list->head_sentinel.next = &list->tail_sentinel;
   list->head_sentinel.prev = NULL;
   list->tail_sentinel.next = NULL;
   list->tail_sentinel.prev = &list->head_sentinel;
}

static inline bool
exec_list_is_empty(const struct exec_list *list)
{
   /* There are three ways to test whether a list is empty or not.
    *
    * - Check to see if the head sentinel's \c next is the tail sentinel.
    * - Check to see if the tail sentinel's \c prev is the head sentinel.
    * - Check to see if the head is the sentinel node by test whether its
    *   \c next pointer is \c NULL.
    *
    * The first two methods tend to generate better code on modern systems
    * because they save a pointer dereference.
    */
   return list->head_sentinel.next == &list->tail_sentinel;
}

static inline bool
exec_list_is_singular(const struct exec_list *list)
{
   return !exec_list_is_empty(list) &&
          list->head_sentinel.next->next == &list->tail_sentinel;
}

static inline const struct exec_node *
exec_list_get_head_const(const struct exec_list *list)
{
   return !exec_list_is_empty(list) ? list->head_sentinel.next : NULL;
}

static inline struct exec_node *
exec_list_get_head(struct exec_list *list)
{
   return !exec_list_is_empty(list) ? list->head_sentinel.next : NULL;
}

static inline struct exec_node *
exec_list_get_head_raw(struct exec_list *list)
{
   return list->head_sentinel.next;
}

static inline struct exec_node *
exec_list_get_tail(struct exec_list *list)
{
   return !exec_list_is_empty(list) ? list->tail_sentinel.prev : NULL;
}

static inline unsigned
exec_list_length(const struct exec_list *list)
{
   unsigned size = 0;
   struct exec_node *node;

   for (node = list->head_sentinel.next; node->next != NULL; node = node->next) {
      size++;
   }

   return size;
}

static inline void
exec_list_push_head(struct exec_list *list, struct exec_node *n)
{
   n->next = list->head_sentinel.next;
   n->prev = &list->head_sentinel;

   n->next->prev = n;
   list->head_sentinel.next = n;
}

static inline void
exec_list_push_tail(struct exec_list *list, struct exec_node *n)
{
   n->next = &list->tail_sentinel;
   n->prev = list->tail_sentinel.prev;

   n->prev->next = n;
   list->tail_sentinel.prev = n;
}

static inline struct exec_node *
exec_list_pop_head(struct exec_list *list)
{
   struct exec_node *const n = exec_list_get_head(list);
   if (n != NULL)
      exec_node_remove(n);

   return n;
}

static inline void
exec_list_move_nodes_to(struct exec_list *list, struct exec_list *target)
{
   if (exec_list_is_empty(list)) {
      exec_list_make_empty(target);
   } else {
      target->head_sentinel.next = list->head_sentinel.next;
      target->head_sentinel.prev = NULL;
      target->tail_sentinel.next = NULL;
      target->tail_sentinel.prev = list->tail_sentinel.prev;

      target->head_sentinel.next->prev = &target->head_sentinel;
      target->tail_sentinel.prev->next = &target->tail_sentinel;

      exec_list_make_empty(list);
   }
}

static inline void
exec_list_append(struct exec_list *list, struct exec_list *source)
{
   if (exec_list_is_empty(source))
      return;

   /* Link the first node of the source with the last node of the target list.
    */
   list->tail_sentinel.prev->next = source->head_sentinel.next;
   source->head_sentinel.next->prev = list->tail_sentinel.prev;

   /* Make the tail of the source list be the tail of the target list.
    */
   list->tail_sentinel.prev = source->tail_sentinel.prev;
   list->tail_sentinel.prev->next = &list->tail_sentinel;

   /* Make the source list empty for good measure.
    */
   exec_list_make_empty(source);
}

static inline void
exec_node_insert_list_after(struct exec_node *n, struct exec_list *after)
{
   if (exec_list_is_empty(after))
      return;

   after->tail_sentinel.prev->next = n->next;
   after->head_sentinel.next->prev = n;

   n->next->prev = after->tail_sentinel.prev;
   n->next = after->head_sentinel.next;

   exec_list_make_empty(after);
}

static inline void
exec_list_validate(const struct exec_list *list)
{
   const struct exec_node *node;

   assert(list->head_sentinel.next->prev == &list->head_sentinel);
   assert(list->head_sentinel.prev == NULL);
   assert(list->tail_sentinel.next == NULL);
   assert(list->tail_sentinel.prev->next == &list->tail_sentinel);

   /* We could try to use one of the interators below for this but they all
    * either require C++ or assume the exec_node is embedded in a structure
    * which is not the case for this function.
    */
   for (node = list->head_sentinel.next; node->next != NULL; node = node->next) {
      assert(node->next->prev == node);
      assert(node->prev->next == node);
   }
}

/**
 * Iterate through two lists at once.  Stops at the end of the shorter list.
 *
 * This is safe against either current node being removed or replaced.
 */
#define foreach_two_lists(__node1, __list1, __node2, __list2) \
   for (struct exec_node * __node1 = (__list1)->head_sentinel.next, \
                         * __node2 = (__list2)->head_sentinel.next, \
                         * __next1 = __node1->next,           \
                         * __next2 = __node2->next            \
	; __next1 != NULL && __next2 != NULL                  \
	; __node1 = __next1,                                  \
          __node2 = __next2,                                  \
          __next1 = __next1->next,                            \
          __next2 = __next2->next)

#define exec_node_data_forward(type, node, field) \
   (!exec_node_is_tail_sentinel(node) ? exec_node_data(type, node, field) : NULL)

#define exec_node_data_backward(type, node, field) \
   (!exec_node_is_head_sentinel(node) ? exec_node_data(type, node, field) : NULL)

#define exec_node_data_next(type, node, field) \
   exec_node_data_forward(type, (node)->field.next, field)

#define exec_node_data_prev(type, node, field) \
   exec_node_data_backward(type, (node)->field.prev, field)

#define exec_node_data_head(type, list, field) \
   exec_node_data_forward(type, (list)->head_sentinel.next, field)

#define exec_node_data_tail(type, list, field) \
   exec_node_data_backward(type, (list)->tail_sentinel.prev, field)

/**
 * Iterate over the list from head to tail. Removal is safe for all nodes except the current
 * iteration's.
 */
#define foreach_list_typed(type, node, field, list)           \
   for (type * node = exec_node_data_head(type, list, field); \
   node != NULL;                                              \
   node = exec_node_data_next(type, node, field))

#define foreach_list_typed_from(type, node, field, list, __start)     \
   for (type * node = exec_node_data_forward(type, (__start), field); \
   node != NULL;                                                      \
   node = exec_node_data_next(type, node, field))

/**
 * Iterate over the list from tail to head. Removal is safe for all nodes except the current
 * iteration's.
 */
#define foreach_list_typed_reverse(type, node, field, list)   \
   for (type * node = exec_node_data_tail(type, list, field); \
        node != NULL;                                         \
        node = exec_node_data_prev(type, node, field))

/**
 * Iterate over the list from head to tail. Removal is safe for all nodes except the next
 * iteration's. If the next iteration's node is removed and not inserted again, this loop exits.
 */
#define foreach_list_typed_safe(type, node, field, list)         \
   for (type * node = exec_node_data_head(type, list, field),    \
             * __next = node ?                                   \
           exec_node_data_next(type, node, field) : NULL;        \
        node != NULL;                                            \
        node = __next, __next = (__next && __next->field.next) ? \
           exec_node_data_next(type, __next, field) : NULL)

/**
 * Iterate over the list from tail to head. Removal is safe for all nodes except the next
 * iteration's. If the next iteration's node is removed and not inserted again, this loop exits.
 */
#define foreach_list_typed_reverse_safe(type, node, field, list) \
   for (type * node = exec_node_data_tail(type, list, field),    \
             * __prev = node ?                                   \
           exec_node_data_prev(type, node, field) : NULL;        \
        node != NULL;                                            \
        node = __prev, __prev = (__prev && __prev->field.prev) ? \
           exec_node_data_prev(type, __prev, field) : NULL)

#endif /* LIST_CONTAINER_H */
