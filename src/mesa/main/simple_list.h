/* $Id: simple_list.h,v 1.2 2001/03/12 00:48:38 gareth Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.5
 *
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*  Simple macros for typesafe, intrusive lists.
 *  (C) 1997, Keith Whitwell
 *
 *  Intended to work with a list sentinal which is created as an empty
 *  list.  Insert & delete are O(1).
 */


#ifndef _SIMPLE_LIST_H
#define _SIMPLE_LIST_H

#define remove_from_list(elem)			\
do {						\
   (elem)->next->prev = (elem)->prev;		\
   (elem)->prev->next = (elem)->next;		\
} while (0)

#define insert_at_head(list, elem)		\
do {						\
   (elem)->prev = list;				\
   (elem)->next = (list)->next;			\
   (list)->next->prev = elem;			\
   (list)->next = elem;				\
} while(0)

#define insert_at_tail(list, elem)		\
do {						\
   (elem)->next = list;				\
   (elem)->prev = (list)->prev;			\
   (list)->prev->next = elem;			\
   (list)->prev = elem;				\
} while(0)

#define move_to_head(list, elem)		\
do {						\
   remove_from_list(elem);			\
   insert_at_head(list, elem);			\
} while (0)

#define move_to_tail(list, elem)		\
do {						\
   remove_from_list(elem);			\
   insert_at_tail(list, elem);			\
} while (0)


#define make_empty_list(sentinal)		\
do {						\
   (sentinal)->next = sentinal;			\
   (sentinal)->prev = sentinal;			\
} while (0)


#define first_elem(list)       ((list)->next)
#define last_elem(list)        ((list)->prev)
#define next_elem(elem)        ((elem)->next)
#define prev_elem(elem)        ((elem)->prev)
#define at_end(list, elem)     ((elem) == (list))
#define is_empty_list(list)    ((list)->next == (list))

#define foreach(ptr, list)     \
        for( ptr=(list)->next ;  ptr!=list ;  ptr=(ptr)->next )

/* Kludgey - Lets you unlink the current value during a list
 *           traversal.  Useful for free()-ing a list, element
 *           by element.
 */
#define foreach_s(ptr, t, list)   \
        for(ptr=(list)->next,t=(ptr)->next; list != ptr; ptr=t, t=(t)->next)


#endif
