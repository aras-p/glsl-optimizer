/**************************************************************************
 *
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
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
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

/**
 * \file
 * Implementation of fenced buffers.
 * 
 * \author José Fonseca <jrfonseca-at-tungstengraphics-dot-com>
 * \author Thomas Hellström <thomas-at-tungstengraphics-dot-com>
 */


#include "pipe/p_compiler.h"
#include "pipe/p_debug.h"
#include "pipe/p_winsys.h"
#include "pipe/p_thread.h"
#include "pipe/p_util.h"
#include "util/u_double_list.h"

#include "pb_buffer.h"
#include "pb_buffer_fenced.h"

#ifndef WIN32
#include <unistd.h>
#endif


/**
 * Convenience macro (type safe).
 */
#define SUPER(__derived) (&(__derived)->base)


struct fenced_buffer_list
{
   _glthread_Mutex mutex;
   
   struct pipe_winsys *winsys;
   
   size_t numDelayed;
   size_t checkDelayed;
   
   struct list_head delayed;
};


/**
 * Wrapper around a pipe buffer which adds fencing and reference counting.
 */
struct fenced_buffer
{
   struct pb_buffer base;
   
   struct pb_buffer *buffer;

   struct pipe_fence_handle *fence;

   struct list_head head;
   struct fenced_buffer_list *list;
};


static INLINE struct fenced_buffer *
fenced_buffer(struct pb_buffer *buf)
{
   assert(buf);
   assert(buf->vtbl == &fenced_buffer_vtbl);
   return (struct fenced_buffer *)buf;
}


static void
_fenced_buffer_list_check_free(struct fenced_buffer_list *fenced_list, 
                               int wait)
{
   struct pipe_winsys *winsys = fenced_list->winsys;
   struct fenced_buffer *fenced_buf;   
   struct list_head *list, *prev;
   int signaled = -1;

   list = fenced_list->delayed.next;
   prev = list->prev;
   for (; list != &fenced_list->delayed; list = prev, prev = list->prev) {

      fenced_buf = LIST_ENTRY(struct fenced_buffer, list, head);

      if (signaled != 0) {
         if (wait) {
            signaled = winsys->fence_finish(winsys, fenced_buf->fence, 0);
         }
         else {
            signaled = winsys->fence_signalled(winsys, fenced_buf->fence, 0);
         }
      }

      if (signaled != 0) {
#if 0
	 /* XXX: we are assuming that buffers are freed in the same order they 
	  * are fenced which may not always be true... 
	  */
         break;
#else
         signaled = -1;
	 continue;
#endif
      }

      winsys->fence_reference(winsys, &fenced_buf->fence, NULL);
      
      LIST_DEL(list);
      fenced_list->numDelayed--;

      /* Do the delayed destroy:
       */
      pb_reference(&fenced_buf->buffer, NULL);
      FREE(fenced_buf);
   }
}


static void
fenced_buffer_destroy(struct pb_buffer *buf)
{
   struct fenced_buffer *fenced_buf = fenced_buffer(buf);   
   struct fenced_buffer_list *fenced_list = fenced_buf->list;

   if (fenced_buf->fence) {
      struct pipe_winsys *winsys = fenced_list->winsys;
      if(winsys->fence_signalled(winsys, fenced_buf->fence, 0) != 0) { 
	 LIST_ADDTAIL(&fenced_buf->head, &fenced_list->delayed);
	 fenced_list->numDelayed++;
      }
      else {
	 winsys->fence_reference(winsys, &fenced_buf->fence, NULL);
	 pb_reference(&fenced_buf->buffer, NULL);
	 FREE(fenced_buf);
      }
   }
   else {
      pb_reference(&fenced_buf->buffer, NULL);
      FREE(fenced_buf);
   }
   
   if ((fenced_list->numDelayed % fenced_list->checkDelayed) == 0)
      _fenced_buffer_list_check_free(fenced_list, 0);
}


static void *
fenced_buffer_map(struct pb_buffer *buf, 
                  unsigned flags)
{
   struct fenced_buffer *fenced_buf = fenced_buffer(buf);   
   return pb_map(fenced_buf->buffer, flags);
}


static void
fenced_buffer_unmap(struct pb_buffer *buf)
{
   struct fenced_buffer *fenced_buf = fenced_buffer(buf);   
   pb_unmap(fenced_buf->buffer);
}


static void
fenced_buffer_get_base_buffer(struct pb_buffer *buf,
                              struct pb_buffer **base_buf,
                              unsigned *offset)
{
   struct fenced_buffer *fenced_buf = fenced_buffer(buf);
   pb_get_base_buffer(fenced_buf->buffer, base_buf, offset);
}


const struct pb_vtbl 
fenced_buffer_vtbl = {
      fenced_buffer_destroy,
      fenced_buffer_map,
      fenced_buffer_unmap,
      fenced_buffer_get_base_buffer
};


struct pb_buffer *
fenced_buffer_create(struct fenced_buffer_list *fenced_list, 
                     struct pb_buffer *buffer)
{
   struct fenced_buffer *buf;
   
   if(!buffer)
      return NULL;
   
   buf = CALLOC_STRUCT(fenced_buffer);
   if(!buf)
      return NULL;
   
   buf->base.base.refcount = 1;
   buf->base.base.alignment = buffer->base.alignment;
   buf->base.base.usage = buffer->base.usage;
   buf->base.base.size = buffer->base.size;
   
   buf->base.vtbl = &fenced_buffer_vtbl;
   buf->buffer = buffer;
   buf->list = fenced_list;
   
   return &buf->base;
}


void
buffer_fence(struct pb_buffer *buf,
             struct pipe_fence_handle *fence)
{
   struct fenced_buffer *fenced_buf = fenced_buffer(buf);
   struct fenced_buffer_list *fenced_list = fenced_buf->list;
   struct pipe_winsys *winsys = fenced_list->winsys;
   
   _glthread_LOCK_MUTEX(fenced_list->mutex);
   winsys->fence_reference(winsys, &fenced_buf->fence, fence);
   _glthread_UNLOCK_MUTEX(fenced_list->mutex);
}


struct fenced_buffer_list *
fenced_buffer_list_create(struct pipe_winsys *winsys) 
{
   struct fenced_buffer_list *fenced_list;

   fenced_list = (struct fenced_buffer_list *)CALLOC(1, sizeof(*fenced_list));
   if (!fenced_list)
      return NULL;

   fenced_list->winsys = winsys;

   LIST_INITHEAD(&fenced_list->delayed);

   fenced_list->numDelayed = 0;
   
   /* TODO: don't hard code this */ 
   fenced_list->checkDelayed = 5;

   _glthread_INIT_MUTEX(fenced_list->mutex);

   return fenced_list;
}


void
fenced_buffer_list_check_free(struct fenced_buffer_list *fenced_list, 
                              int wait)
{
   _glthread_LOCK_MUTEX(fenced_list->mutex);
   _fenced_buffer_list_check_free(fenced_list, wait);
   _glthread_UNLOCK_MUTEX(fenced_list->mutex);
}


void
fenced_buffer_list_destroy(struct fenced_buffer_list *fenced_list)
{
   _glthread_LOCK_MUTEX(fenced_list->mutex);

   /* Wait on outstanding fences */
   while (fenced_list->numDelayed) {
      _glthread_UNLOCK_MUTEX(fenced_list->mutex);
#ifndef WIN32
      sched_yield();
#endif
      _fenced_buffer_list_check_free(fenced_list, 1);
      _glthread_LOCK_MUTEX(fenced_list->mutex);
   }

   _glthread_UNLOCK_MUTEX(fenced_list->mutex);
   
   FREE(fenced_list);
}


