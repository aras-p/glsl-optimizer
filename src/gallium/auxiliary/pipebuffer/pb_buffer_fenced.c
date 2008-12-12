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


#include "pipe/p_config.h"

#if defined(PIPE_OS_LINUX)
#include <unistd.h>
#include <sched.h>
#endif

#include "pipe/p_compiler.h"
#include "pipe/p_error.h"
#include "pipe/p_debug.h"
#include "pipe/p_winsys.h"
#include "pipe/p_thread.h"
#include "util/u_memory.h"
#include "util/u_double_list.h"

#include "pb_buffer.h"
#include "pb_buffer_fenced.h"



/**
 * Convenience macro (type safe).
 */
#define SUPER(__derived) (&(__derived)->base)

#define PIPE_BUFFER_USAGE_CPU_READ_WRITE \
   ( PIPE_BUFFER_USAGE_CPU_READ | PIPE_BUFFER_USAGE_CPU_WRITE )
#define PIPE_BUFFER_USAGE_GPU_READ_WRITE \
   ( PIPE_BUFFER_USAGE_GPU_READ | PIPE_BUFFER_USAGE_GPU_WRITE )
#define PIPE_BUFFER_USAGE_WRITE \
   ( PIPE_BUFFER_USAGE_CPU_WRITE | PIPE_BUFFER_USAGE_GPU_WRITE )


struct fenced_buffer_list
{
   pipe_mutex mutex;
   
   struct pipe_winsys *winsys;
   
   size_t numDelayed;
   
   struct list_head delayed;
};


/**
 * Wrapper around a pipe buffer which adds fencing and reference counting.
 */
struct fenced_buffer
{
   struct pb_buffer base;
   
   struct pb_buffer *buffer;

   /* FIXME: protect access with mutex */

   /**
    * A bitmask of PIPE_BUFFER_USAGE_CPU/GPU_READ/WRITE describing the current
    * buffer usage.
    */
   unsigned flags;

   unsigned mapcount;
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


static INLINE void
_fenced_buffer_add(struct fenced_buffer *fenced_buf)
{
   struct fenced_buffer_list *fenced_list = fenced_buf->list;

   assert(fenced_buf->base.base.refcount);
   assert(fenced_buf->flags & PIPE_BUFFER_USAGE_GPU_READ_WRITE);
   assert(fenced_buf->fence);

   assert(!fenced_buf->head.prev);
   assert(!fenced_buf->head.next);
   LIST_ADDTAIL(&fenced_buf->head, &fenced_list->delayed);
   ++fenced_list->numDelayed;
}


/**
 * Actually destroy the buffer.
 */
static INLINE void
_fenced_buffer_destroy(struct fenced_buffer *fenced_buf)
{
   assert(!fenced_buf->base.base.refcount);
   assert(!fenced_buf->fence);
   pb_reference(&fenced_buf->buffer, NULL);
   FREE(fenced_buf);
}


static INLINE void
_fenced_buffer_remove(struct fenced_buffer_list *fenced_list,
                      struct fenced_buffer *fenced_buf)
{
   struct pipe_winsys *winsys = fenced_list->winsys;

   assert(fenced_buf->fence);
   assert(fenced_buf->list == fenced_list);
   
   winsys->fence_reference(winsys, &fenced_buf->fence, NULL);
   fenced_buf->flags &= ~PIPE_BUFFER_USAGE_GPU_READ_WRITE;
   
   assert(fenced_buf->head.prev);
   assert(fenced_buf->head.next);
   LIST_DEL(&fenced_buf->head);
#ifdef DEBUG
   fenced_buf->head.prev = NULL;
   fenced_buf->head.next = NULL;
#endif
   
   assert(fenced_list->numDelayed);
   --fenced_list->numDelayed;
   
   if(!fenced_buf->base.base.refcount)
      _fenced_buffer_destroy(fenced_buf);
}


static INLINE enum pipe_error
_fenced_buffer_finish(struct fenced_buffer *fenced_buf)
{
   struct fenced_buffer_list *fenced_list = fenced_buf->list;
   struct pipe_winsys *winsys = fenced_list->winsys;

#if 0
   debug_warning("waiting for GPU");
#endif

   assert(fenced_buf->fence);
   if(fenced_buf->fence) {
      if(winsys->fence_finish(winsys, fenced_buf->fence, 0) != 0) {
	 return PIPE_ERROR;
      }
      /* Remove from the fenced list */
      /* TODO: remove consequents */
      _fenced_buffer_remove(fenced_list, fenced_buf);
   }

   fenced_buf->flags &= ~PIPE_BUFFER_USAGE_GPU_READ_WRITE;
   return PIPE_OK;
}


/**
 * Free as many fenced buffers from the list head as possible. 
 */
static void
_fenced_buffer_list_check_free(struct fenced_buffer_list *fenced_list, 
                               int wait)
{
   struct pipe_winsys *winsys = fenced_list->winsys;
   struct list_head *curr, *next;
   struct fenced_buffer *fenced_buf;
   struct pipe_fence_handle *prev_fence = NULL;

   curr = fenced_list->delayed.next;
   next = curr->next;
   while(curr != &fenced_list->delayed) {
      fenced_buf = LIST_ENTRY(struct fenced_buffer, curr, head);

      if(fenced_buf->fence != prev_fence) {
	 int signaled;
	 if (wait)
	    signaled = winsys->fence_finish(winsys, fenced_buf->fence, 0);
	 else
	    signaled = winsys->fence_signalled(winsys, fenced_buf->fence, 0);
	 if (signaled != 0)
	    break;
	 prev_fence = fenced_buf->fence;
      }
      else {
	 assert(winsys->fence_signalled(winsys, fenced_buf->fence, 0) == 0);
      }

      _fenced_buffer_remove(fenced_list, fenced_buf);

      curr = next; 
      next = curr->next;
   }
}


static void
fenced_buffer_destroy(struct pb_buffer *buf)
{
   struct fenced_buffer *fenced_buf = fenced_buffer(buf);   
   struct fenced_buffer_list *fenced_list = fenced_buf->list;

   pipe_mutex_lock(fenced_list->mutex);
   assert(fenced_buf->base.base.refcount == 0);
   if (fenced_buf->fence) {
      struct pipe_winsys *winsys = fenced_list->winsys;
      if(winsys->fence_signalled(winsys, fenced_buf->fence, 0) == 0) {
	 struct list_head *curr, *prev;
	 curr = &fenced_buf->head;
	 prev = curr->prev;
	 do {
	    fenced_buf = LIST_ENTRY(struct fenced_buffer, curr, head);
	    assert(winsys->fence_signalled(winsys, fenced_buf->fence, 0) == 0);
	    _fenced_buffer_remove(fenced_list, fenced_buf);
	    curr = prev;
	    prev = curr->prev;
	 } while (curr != &fenced_list->delayed);
      }	  
      else {
	 /* delay destruction */
      }
   }
   else {
      _fenced_buffer_destroy(fenced_buf);
   }
   pipe_mutex_unlock(fenced_list->mutex);
}


static void *
fenced_buffer_map(struct pb_buffer *buf, 
                  unsigned flags)
{
   struct fenced_buffer *fenced_buf = fenced_buffer(buf);
   void *map;

   assert(!(flags & ~PIPE_BUFFER_USAGE_CPU_READ_WRITE));
   flags &= PIPE_BUFFER_USAGE_CPU_READ_WRITE;
   
   /* Check for GPU read/write access */
   if(fenced_buf->flags & PIPE_BUFFER_USAGE_GPU_WRITE) {
      /* Wait for the GPU to finish writing */
      _fenced_buffer_finish(fenced_buf);
   }

#if 0
   /* Check for CPU write access (read is OK) */
   if(fenced_buf->flags & PIPE_BUFFER_USAGE_CPU_READ_WRITE) {
      /* this is legal -- just for debugging */
      debug_warning("concurrent CPU writes");
   }
#endif
   
   map = pb_map(fenced_buf->buffer, flags);
   if(map) {
      ++fenced_buf->mapcount;
      fenced_buf->flags |= flags;
   }

   return map;
}


static void
fenced_buffer_unmap(struct pb_buffer *buf)
{
   struct fenced_buffer *fenced_buf = fenced_buffer(buf);
   assert(fenced_buf->mapcount);
   if(fenced_buf->mapcount) {
      pb_unmap(fenced_buf->buffer);
      --fenced_buf->mapcount;
      if(!fenced_buf->mapcount)
	 fenced_buf->flags &= ~PIPE_BUFFER_USAGE_CPU_READ_WRITE;
   }
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
   if(!buf) {
      pb_reference(&buffer, NULL);
      return NULL;
   }
   
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
   struct fenced_buffer *fenced_buf;
   struct fenced_buffer_list *fenced_list;
   struct pipe_winsys *winsys;
   /* FIXME: receive this as a parameter */
   unsigned flags = fence ? PIPE_BUFFER_USAGE_GPU_READ_WRITE : 0;

   /* This is a public function, so be extra cautious with the buffer passed, 
    * as happens frequently to receive null buffers, or pointer to buffers 
    * other than fenced buffers. */
   assert(buf);
   if(!buf)
      return;
   assert(buf->vtbl == &fenced_buffer_vtbl);
   if(buf->vtbl != &fenced_buffer_vtbl)
      return;
   
   fenced_buf = fenced_buffer(buf);
   fenced_list = fenced_buf->list;
   winsys = fenced_list->winsys;
   
   if(!fence || fence == fenced_buf->fence) {
      /* Handle the same fence case specially, not only because it is a fast 
       * path, but mostly to avoid serializing two writes with the same fence, 
       * as that would bring the hardware down to synchronous operation without
       * any benefit.
       */
      fenced_buf->flags |= flags & PIPE_BUFFER_USAGE_GPU_READ_WRITE;
      return;
   }
   
   pipe_mutex_lock(fenced_list->mutex);
   if (fenced_buf->fence)
      _fenced_buffer_remove(fenced_list, fenced_buf);
   if (fence) {
      winsys->fence_reference(winsys, &fenced_buf->fence, fence);
      fenced_buf->flags |= flags & PIPE_BUFFER_USAGE_GPU_READ_WRITE;
      _fenced_buffer_add(fenced_buf);
   }
   pipe_mutex_unlock(fenced_list->mutex);
}


struct fenced_buffer_list *
fenced_buffer_list_create(struct pipe_winsys *winsys) 
{
   struct fenced_buffer_list *fenced_list;

   fenced_list = CALLOC_STRUCT(fenced_buffer_list);
   if (!fenced_list)
      return NULL;

   fenced_list->winsys = winsys;

   LIST_INITHEAD(&fenced_list->delayed);

   fenced_list->numDelayed = 0;
   
   pipe_mutex_init(fenced_list->mutex);

   return fenced_list;
}


void
fenced_buffer_list_check_free(struct fenced_buffer_list *fenced_list, 
                              int wait)
{
   pipe_mutex_lock(fenced_list->mutex);
   _fenced_buffer_list_check_free(fenced_list, wait);
   pipe_mutex_unlock(fenced_list->mutex);
}


void
fenced_buffer_list_destroy(struct fenced_buffer_list *fenced_list)
{
   pipe_mutex_lock(fenced_list->mutex);

   /* Wait on outstanding fences */
   while (fenced_list->numDelayed) {
      pipe_mutex_unlock(fenced_list->mutex);
#if defined(PIPE_OS_LINUX)
      sched_yield();
#endif
      _fenced_buffer_list_check_free(fenced_list, 1);
      pipe_mutex_lock(fenced_list->mutex);
   }

   pipe_mutex_unlock(fenced_list->mutex);
   
   FREE(fenced_list);
}


