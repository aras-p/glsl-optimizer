/**************************************************************************
 *
 * Copyright 2007-2009 VMware, Inc.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

/**
 * \file
 * Implementation of fenced buffers.
 * 
 * \author Jose Fonseca <jrfonseca-at-tungstengraphics-dot-com>
 * \author Thomas Hellström <thomas-at-tungstengraphics-dot-com>
 */


#include "pipe/p_config.h"

#if defined(PIPE_OS_LINUX) || defined(PIPE_OS_BSD) || defined(PIPE_OS_SOLARIS)
#include <unistd.h>
#include <sched.h>
#endif

#include "pipe/p_compiler.h"
#include "pipe/p_defines.h"
#include "util/u_debug.h"
#include "pipe/p_thread.h"
#include "util/u_memory.h"
#include "util/u_double_list.h"

#include "pb_buffer.h"
#include "pb_buffer_fenced.h"



/**
 * Convenience macro (type safe).
 */
#define SUPER(__derived) (&(__derived)->base)


struct fenced_buffer_list
{
   pipe_mutex mutex;
   
   struct pb_fence_ops *ops;
   
   pb_size numDelayed;
   struct list_head delayed;
   
#ifdef DEBUG
   pb_size numUnfenced;
   struct list_head unfenced;
#endif
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
   struct pb_validate *vl;
   unsigned validation_flags;
   struct pipe_fence_handle *fence;

   struct list_head head;
   struct fenced_buffer_list *list;
};


static INLINE struct fenced_buffer *
fenced_buffer(struct pb_buffer *buf)
{
   assert(buf);
   return (struct fenced_buffer *)buf;
}


static INLINE void
_fenced_buffer_add(struct fenced_buffer *fenced_buf)
{
   struct fenced_buffer_list *fenced_list = fenced_buf->list;

   assert(pipe_is_referenced(&fenced_buf->base.base.reference));
   assert(fenced_buf->flags & PIPE_BUFFER_USAGE_GPU_READ_WRITE);
   assert(fenced_buf->fence);

#ifdef DEBUG
   LIST_DEL(&fenced_buf->head);
   assert(fenced_list->numUnfenced);
   --fenced_list->numUnfenced;
#endif
   LIST_ADDTAIL(&fenced_buf->head, &fenced_list->delayed);
   ++fenced_list->numDelayed;
}


/**
 * Actually destroy the buffer.
 */
static INLINE void
_fenced_buffer_destroy(struct fenced_buffer *fenced_buf)
{
   struct fenced_buffer_list *fenced_list = fenced_buf->list;
   
   assert(!pipe_is_referenced(&fenced_buf->base.base.reference));
   assert(!fenced_buf->fence);
#ifdef DEBUG
   assert(fenced_buf->head.prev);
   assert(fenced_buf->head.next);
   LIST_DEL(&fenced_buf->head);
   assert(fenced_list->numUnfenced);
   --fenced_list->numUnfenced;
#else
   (void)fenced_list;
#endif
   pb_reference(&fenced_buf->buffer, NULL);
   FREE(fenced_buf);
}


static INLINE void
_fenced_buffer_remove(struct fenced_buffer_list *fenced_list,
                      struct fenced_buffer *fenced_buf)
{
   struct pb_fence_ops *ops = fenced_list->ops;

   assert(fenced_buf->fence);
   assert(fenced_buf->list == fenced_list);
   
   ops->fence_reference(ops, &fenced_buf->fence, NULL);
   fenced_buf->flags &= ~PIPE_BUFFER_USAGE_GPU_READ_WRITE;
   
   assert(fenced_buf->head.prev);
   assert(fenced_buf->head.next);
   
   LIST_DEL(&fenced_buf->head);
   assert(fenced_list->numDelayed);
   --fenced_list->numDelayed;
   
#ifdef DEBUG
   LIST_ADDTAIL(&fenced_buf->head, &fenced_list->unfenced);
   ++fenced_list->numUnfenced;
#endif
   
   /**
    * FIXME!!!
    */

   if(!pipe_is_referenced(&fenced_buf->base.base.reference))
      _fenced_buffer_destroy(fenced_buf);
}


static INLINE enum pipe_error
_fenced_buffer_finish(struct fenced_buffer *fenced_buf)
{
   struct fenced_buffer_list *fenced_list = fenced_buf->list;
   struct pb_fence_ops *ops = fenced_list->ops;

#if 0
   debug_warning("waiting for GPU");
#endif

   assert(fenced_buf->fence);
   if(fenced_buf->fence) {
      if(ops->fence_finish(ops, fenced_buf->fence, 0) != 0) {
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
   struct pb_fence_ops *ops = fenced_list->ops;
   struct list_head *curr, *next;
   struct fenced_buffer *fenced_buf;
   struct pb_buffer *pb_buf;
   struct pipe_fence_handle *prev_fence = NULL;

   curr = fenced_list->delayed.next;
   next = curr->next;
   while(curr != &fenced_list->delayed) {
      fenced_buf = LIST_ENTRY(struct fenced_buffer, curr, head);

      if(fenced_buf->fence != prev_fence) {
	 int signaled;
	 if (wait)
	    signaled = ops->fence_finish(ops, fenced_buf->fence, 0);
	 else
	    signaled = ops->fence_signalled(ops, fenced_buf->fence, 0);
	 if (signaled != 0)
	    break;
	 prev_fence = fenced_buf->fence;
      }
      else {
	 assert(ops->fence_signalled(ops, fenced_buf->fence, 0) == 0);
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
   assert(!pipe_is_referenced(&fenced_buf->base.base.reference));
   if (fenced_buf->fence) {
      struct pb_fence_ops *ops = fenced_list->ops;
      if(ops->fence_signalled(ops, fenced_buf->fence, 0) == 0) {
	 struct list_head *curr, *prev;
	 curr = &fenced_buf->head;
	 prev = curr->prev;
	 do {
	    fenced_buf = LIST_ENTRY(struct fenced_buffer, curr, head);
	    assert(ops->fence_signalled(ops, fenced_buf->fence, 0) == 0);
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
   struct fenced_buffer_list *fenced_list = fenced_buf->list;
   struct pb_fence_ops *ops = fenced_list->ops;
   void *map;

   assert(!(flags & PIPE_BUFFER_USAGE_GPU_READ_WRITE));
   
   /* Serialize writes */
   if((fenced_buf->flags & PIPE_BUFFER_USAGE_GPU_WRITE) ||
      ((fenced_buf->flags & PIPE_BUFFER_USAGE_GPU_READ) && (flags & PIPE_BUFFER_USAGE_CPU_WRITE))) {
      if(flags & PIPE_BUFFER_USAGE_DONTBLOCK) {
         /* Don't wait for the GPU to finish writing */
         if(ops->fence_signalled(ops, fenced_buf->fence, 0) == 0)
            _fenced_buffer_remove(fenced_list, fenced_buf);
         else
            return NULL;
      }
      else {
         /* Wait for the GPU to finish writing */
         _fenced_buffer_finish(fenced_buf);
      }
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
      fenced_buf->flags |= flags & PIPE_BUFFER_USAGE_CPU_READ_WRITE;
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


static enum pipe_error
fenced_buffer_validate(struct pb_buffer *buf,
                       struct pb_validate *vl,
                       unsigned flags)
{
   struct fenced_buffer *fenced_buf = fenced_buffer(buf);
   enum pipe_error ret;
   
   if(!vl) {
      /* invalidate */
      fenced_buf->vl = NULL;
      fenced_buf->validation_flags = 0;
      return PIPE_OK;
   }
   
   assert(flags & PIPE_BUFFER_USAGE_GPU_READ_WRITE);
   assert(!(flags & ~PIPE_BUFFER_USAGE_GPU_READ_WRITE));
   flags &= PIPE_BUFFER_USAGE_GPU_READ_WRITE;

   /* Buffer cannot be validated in two different lists */ 
   if(fenced_buf->vl && fenced_buf->vl != vl)
      return PIPE_ERROR_RETRY;
   
#if 0
   /* Do not validate if buffer is still mapped */
   if(fenced_buf->flags & PIPE_BUFFER_USAGE_CPU_READ_WRITE) {
      /* TODO: wait for the thread that mapped the buffer to unmap it */
      return PIPE_ERROR_RETRY;
   }
   /* Final sanity checking */
   assert(!(fenced_buf->flags & PIPE_BUFFER_USAGE_CPU_READ_WRITE));
   assert(!fenced_buf->mapcount);
#endif

   if(fenced_buf->vl == vl &&
      (fenced_buf->validation_flags & flags) == flags) {
      /* Nothing to do -- buffer already validated */
      return PIPE_OK;
   }
   
   ret = pb_validate(fenced_buf->buffer, vl, flags);
   if (ret != PIPE_OK)
      return ret;
   
   fenced_buf->vl = vl;
   fenced_buf->validation_flags |= flags;
   
   return PIPE_OK;
}


static void
fenced_buffer_fence(struct pb_buffer *buf,
                    struct pipe_fence_handle *fence)
{
   struct fenced_buffer *fenced_buf;
   struct fenced_buffer_list *fenced_list;
   struct pb_fence_ops *ops;

   fenced_buf = fenced_buffer(buf);
   fenced_list = fenced_buf->list;
   ops = fenced_list->ops;
   
   if(fence == fenced_buf->fence) {
      /* Nothing to do */
      return;
   }

   assert(fenced_buf->vl);
   assert(fenced_buf->validation_flags);
   
   pipe_mutex_lock(fenced_list->mutex);
   if (fenced_buf->fence)
      _fenced_buffer_remove(fenced_list, fenced_buf);
   if (fence) {
      ops->fence_reference(ops, &fenced_buf->fence, fence);
      fenced_buf->flags |= fenced_buf->validation_flags;
      _fenced_buffer_add(fenced_buf);
   }
   pipe_mutex_unlock(fenced_list->mutex);
   
   pb_fence(fenced_buf->buffer, fence);

   fenced_buf->vl = NULL;
   fenced_buf->validation_flags = 0;
}


static void
fenced_buffer_get_base_buffer(struct pb_buffer *buf,
                              struct pb_buffer **base_buf,
                              pb_size *offset)
{
   struct fenced_buffer *fenced_buf = fenced_buffer(buf);
   pb_get_base_buffer(fenced_buf->buffer, base_buf, offset);
}


static const struct pb_vtbl 
fenced_buffer_vtbl = {
      fenced_buffer_destroy,
      fenced_buffer_map,
      fenced_buffer_unmap,
      fenced_buffer_validate,
      fenced_buffer_fence,
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
   
   pipe_reference_init(&buf->base.base.reference, 1);
   buf->base.base.alignment = buffer->base.alignment;
   buf->base.base.usage = buffer->base.usage;
   buf->base.base.size = buffer->base.size;
   
   buf->base.vtbl = &fenced_buffer_vtbl;
   buf->buffer = buffer;
   buf->list = fenced_list;
   
#ifdef DEBUG
   pipe_mutex_lock(fenced_list->mutex);
   LIST_ADDTAIL(&buf->head, &fenced_list->unfenced);
   ++fenced_list->numUnfenced;
   pipe_mutex_unlock(fenced_list->mutex);
#endif

   return &buf->base;
}


struct fenced_buffer_list *
fenced_buffer_list_create(struct pb_fence_ops *ops) 
{
   struct fenced_buffer_list *fenced_list;

   fenced_list = CALLOC_STRUCT(fenced_buffer_list);
   if (!fenced_list)
      return NULL;

   fenced_list->ops = ops;

   LIST_INITHEAD(&fenced_list->delayed);
   fenced_list->numDelayed = 0;
   
#ifdef DEBUG
   LIST_INITHEAD(&fenced_list->unfenced);
   fenced_list->numUnfenced = 0;
#endif

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


#ifdef DEBUG
void
fenced_buffer_list_dump(struct fenced_buffer_list *fenced_list)
{
   struct pb_fence_ops *ops = fenced_list->ops;
   struct list_head *curr, *next;
   struct fenced_buffer *fenced_buf;

   pipe_mutex_lock(fenced_list->mutex);

   debug_printf("%10s %7s %7s %10s %s\n",
                "buffer", "size", "refcount", "fence", "signalled");
   
   curr = fenced_list->unfenced.next;
   next = curr->next;
   while(curr != &fenced_list->unfenced) {
      fenced_buf = LIST_ENTRY(struct fenced_buffer, curr, head);
      assert(!fenced_buf->fence);
      debug_printf("%10p %7u %7u\n",
                   (void *) fenced_buf,
                   fenced_buf->base.base.size,
                   p_atomic_read(&fenced_buf->base.base.reference.count));
      curr = next; 
      next = curr->next;
   }
   
   curr = fenced_list->delayed.next;
   next = curr->next;
   while(curr != &fenced_list->delayed) {
      int signaled;
      fenced_buf = LIST_ENTRY(struct fenced_buffer, curr, head);
      signaled = ops->fence_signalled(ops, fenced_buf->fence, 0);
      debug_printf("%10p %7u %7u %10p %s\n",
                   (void *) fenced_buf,
                   fenced_buf->base.base.size,
                   p_atomic_read(&fenced_buf->base.base.reference.count),
                   (void *) fenced_buf->fence,
                   signaled == 0 ? "y" : "n");
      curr = next; 
      next = curr->next;
   }
   
   pipe_mutex_unlock(fenced_list->mutex);
}
#endif


void
fenced_buffer_list_destroy(struct fenced_buffer_list *fenced_list)
{
   pipe_mutex_lock(fenced_list->mutex);

   /* Wait on outstanding fences */
   while (fenced_list->numDelayed) {
      pipe_mutex_unlock(fenced_list->mutex);
#if defined(PIPE_OS_LINUX) || defined(PIPE_OS_BSD) || defined(PIPE_OS_SOLARIS)
      sched_yield();
#endif
      _fenced_buffer_list_check_free(fenced_list, 1);
      pipe_mutex_lock(fenced_list->mutex);
   }

#ifdef DEBUG
   /*assert(!fenced_list->numUnfenced);*/
#endif
      
   pipe_mutex_unlock(fenced_list->mutex);
   
   fenced_list->ops->destroy(fenced_list->ops);
   
   FREE(fenced_list);
}


