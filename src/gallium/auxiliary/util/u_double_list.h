/**************************************************************************
 * 
 * Copyright 2006 Tungsten Graphics, Inc., Bismarck, ND. USA.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, 
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE 
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 **************************************************************************/

/**
 * \file
 * List macros heavily inspired by the Linux kernel
 * list handling. No list looping yet.
 * 
 * Is not threadsafe, so common operations need to
 * be protected using an external mutex.
 */

#ifndef _U_DOUBLE_LIST_H_
#define _U_DOUBLE_LIST_H_


#include <stddef.h>


struct list_head
{
    struct list_head *prev;
    struct list_head *next;
};


#define LIST_INITHEAD(__item)			\
  do {						\
    (__item)->prev = (__item);			\
    (__item)->next = (__item);			\
  } while (0)

#define LIST_ADD(__item, __list)		\
  do {						\
    (__item)->prev = (__list);			\
    (__item)->next = (__list)->next;		\
    (__list)->next->prev = (__item);		\
    (__list)->next = (__item);			\
  } while (0)

#define LIST_ADDTAIL(__item, __list)		\
  do {						\
    (__item)->next = (__list);			\
    (__item)->prev = (__list)->prev;		\
    (__list)->prev->next = (__item);		\
    (__list)->prev = (__item);			\
  } while(0)

#define LIST_REPLACE(__from, __to)		\
  do {						\
    (__to)->prev = (__from)->prev;		\
    (__to)->next = (__from)->next;		\
    (__from)->next->prev = (__to);		\
    (__from)->prev->next = (__to);		\
  } while (0)

#define LIST_DEL(__item)			\
  do {						\
    (__item)->prev->next = (__item)->next;	\
    (__item)->next->prev = (__item)->prev;	\
  } while(0)

#define LIST_DELINIT(__item)			\
  do {						\
    (__item)->prev->next = (__item)->next;	\
    (__item)->next->prev = (__item)->prev;	\
    (__item)->next = (__item);			\
    (__item)->prev = (__item);			\
  } while(0)

#define LIST_ENTRY(__type, __item, __field)   \
    ((__type *)(((char *)(__item)) - offsetof(__type, __field)))

#define LIST_IS_EMPTY(__list)                   \
    ((__list)->next == (__list))


#endif /*_U_DOUBLE_LIST_H_*/
