/**************************************************************************
 *
 * Copyright 2006 Tungsten Graphics, Inc., Cedar Park, Texas.
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
 * Buffer manager using the old texture memory manager.
 * 
 * \author José Fonseca <jrfonseca@tungstengraphics.com>
 */


#include <assert.h>
#include <stdlib.h>
#include <unistd.h>

#include "main/imports.h"
#include "glapi/glthread.h"
#include "main/mm.h"
#include "linked_list.h"

#include "p_defines.h"
#include "pb_buffer.h"
#include "pb_bufmgr.h"


/**
 * Convenience macro (type safe).
 */
#define SUPER(__derived) (&(__derived)->base)


struct mm_buffer_manager
{
   struct buffer_manager base;
   
   _glthread_Mutex mutex;
   
   size_t size;
   struct mem_block *heap;
   
   size_t align2;
   
   struct pipe_buffer *buffer;
   void *map;
};


static inline struct mm_buffer_manager *
mm_buffer_manager(struct buffer_manager *mgr)
{
   assert(mgr);
   return (struct mm_buffer_manager *)mgr;
}


struct mm_buffer
{
   struct pipe_buffer base;
   
   struct mm_buffer_manager *mgr;
   
   struct mem_block *block;
};


static inline struct mm_buffer *
mm_buffer(struct pipe_buffer *buf)
{
   assert(buf);
   return (struct mm_buffer *)buf;
}


static void
mm_buffer_reference(struct pipe_buffer *buf)
{
   /* No-op */
}


static void
mm_buffer_release(struct pipe_buffer *buf)
{
   struct mm_buffer *mm_buf = mm_buffer(buf);
   struct mm_buffer_manager *mm = mm_buf->mgr;
   
   _glthread_LOCK_MUTEX(mm->mutex);
   mmFreeMem(mm_buf->block);
   free(buf);
   _glthread_UNLOCK_MUTEX(mm->mutex);
}


static void *
mm_buffer_map(struct pipe_buffer *buf,
              unsigned flags)
{
   struct mm_buffer *mm_buf = mm_buffer(buf);
   struct mm_buffer_manager *mm = mm_buf->mgr;

   return (unsigned char *) mm->map + mm_buf->block->ofs;
}


static void
mm_buffer_unmap(struct pipe_buffer *buf)
{
   /* No-op */
}


static void
mm_buffer_get_base_buffer(struct pipe_buffer *buf,
                          struct pipe_buffer **base_buf,
                          unsigned *offset)
{
   struct mm_buffer *mm_buf = mm_buffer(buf);
   struct mm_buffer_manager *mm = mm_buf->mgr;
   buffer_get_base_buffer(mm->buffer, base_buf, offset);
   *offset += mm_buf->block->ofs;
}


static const struct pipe_buffer_vtbl 
mm_buffer_vtbl = {
      mm_buffer_reference,
      mm_buffer_release,
      mm_buffer_map,
      mm_buffer_unmap,
      mm_buffer_get_base_buffer
};


static struct pipe_buffer *
mm_bufmgr_create_buffer(struct buffer_manager *mgr, 
                        size_t size)
{
   struct mm_buffer_manager *mm = mm_buffer_manager(mgr);
   struct mm_buffer *mm_buf;

   _glthread_LOCK_MUTEX(mm->mutex);

   mm_buf = (struct mm_buffer *)malloc(sizeof(*mm_buf));
   if (!mm_buf) {
      _glthread_UNLOCK_MUTEX(mm->mutex);
      return NULL;
   }

   mm_buf->base.vtbl = &mm_buffer_vtbl;
   
   mm_buf->mgr = mm;
   
   mm_buf->block = mmAllocMem(mm->heap, size, mm->align2, 0);
   if(!mm_buf->block) {
      fprintf(stderr, "warning: heap full\n");
#if 0
      mmDumpMemInfo(mm->heap);
#endif
      
      mm_buf->block = mmAllocMem(mm->heap, size, mm->align2, 0);
      if(!mm_buf->block) {
	 assert(0);
         free(mm_buf);
         _glthread_UNLOCK_MUTEX(mm->mutex);
         return NULL;
      }
   }
   
   /* Some sanity checks */
   assert(0 <= mm_buf->block->ofs && mm_buf->block->ofs < mm->size);
   assert(size <= mm_buf->block->size && mm_buf->block->ofs + mm_buf->block->size <= mm->size);
   
   _glthread_UNLOCK_MUTEX(mm->mutex);
   return SUPER(mm_buf);
}


static void
mm_bufmgr_destroy(struct buffer_manager *mgr)
{
   struct mm_buffer_manager *mm = mm_buffer_manager(mgr);
   
   _glthread_LOCK_MUTEX(mm->mutex);

   mmDestroy(mm->heap);
   
   buffer_unmap(mm->buffer);
   buffer_release(mm->buffer);
   
   _glthread_UNLOCK_MUTEX(mm->mutex);
   
   free(mgr);
}


struct buffer_manager *
mm_bufmgr_create_from_buffer(struct pipe_buffer *buffer, 
                             size_t size, size_t align2) 
{
   struct mm_buffer_manager *mm;

   if(!buffer)
      return NULL;
   
   mm = (struct mm_buffer_manager *)calloc(1, sizeof(*mm));
   if (!mm)
      return NULL;

   mm->base.create_buffer = mm_bufmgr_create_buffer;
   mm->base.destroy = mm_bufmgr_destroy;

   mm->size = size;
   mm->align2 = align2; /* 64-byte alignment */

   _glthread_INIT_MUTEX(mm->mutex);

   mm->buffer = buffer; 

   mm->map = buffer_map(mm->buffer, 
                        PIPE_BUFFER_USAGE_CPU_READ |
                        PIPE_BUFFER_USAGE_CPU_WRITE);
   if(!mm->map)
      goto failure;

   mm->heap = mmInit(0, size); 
   if (!mm->heap)
      goto failure;

   return SUPER(mm);
   
failure:
if(mm->heap)
   mmDestroy(mm->heap);
   if(mm->map)
      buffer_unmap(mm->buffer);
   if(mm)
      free(mm);
   return NULL;
}


struct buffer_manager *
mm_bufmgr_create(struct buffer_manager *provider, 
                 size_t size, size_t align2) 
{
   struct pipe_buffer *buffer;
   struct buffer_manager *mgr;

   assert(provider);
   assert(provider->create_buffer);
   buffer = provider->create_buffer(provider, size); 
   if (!buffer)
      return NULL;
   
   mgr = mm_bufmgr_create_from_buffer(buffer, size, align2);
   if (!mgr) {
      buffer_release(buffer);
      return NULL;
   }

  return mgr;
}
