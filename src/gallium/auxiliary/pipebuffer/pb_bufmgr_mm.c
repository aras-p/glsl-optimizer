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


#include "pipe/p_defines.h"
#include "pipe/p_debug.h"
#include "pipe/p_thread.h"
#include "util/u_memory.h"
#include "util/u_double_list.h"
#include "util/u_mm.h"
#include "pb_buffer.h"
#include "pb_bufmgr.h"


/**
 * Convenience macro (type safe).
 */
#define SUPER(__derived) (&(__derived)->base)


struct mm_pb_manager
{
   struct pb_manager base;
   
   pipe_mutex mutex;
   
   size_t size;
   struct mem_block *heap;
   
   size_t align2;
   
   struct pb_buffer *buffer;
   void *map;
};


static INLINE struct mm_pb_manager *
mm_pb_manager(struct pb_manager *mgr)
{
   assert(mgr);
   return (struct mm_pb_manager *)mgr;
}


struct mm_buffer
{
   struct pb_buffer base;
   
   struct mm_pb_manager *mgr;
   
   struct mem_block *block;
};


static INLINE struct mm_buffer *
mm_buffer(struct pb_buffer *buf)
{
   assert(buf);
   return (struct mm_buffer *)buf;
}


static void
mm_buffer_destroy(struct pb_buffer *buf)
{
   struct mm_buffer *mm_buf = mm_buffer(buf);
   struct mm_pb_manager *mm = mm_buf->mgr;
   
   assert(buf->base.refcount == 0);
   
   pipe_mutex_lock(mm->mutex);
   mmFreeMem(mm_buf->block);
   FREE(buf);
   pipe_mutex_unlock(mm->mutex);
}


static void *
mm_buffer_map(struct pb_buffer *buf,
              unsigned flags)
{
   struct mm_buffer *mm_buf = mm_buffer(buf);
   struct mm_pb_manager *mm = mm_buf->mgr;

   return (unsigned char *) mm->map + mm_buf->block->ofs;
}


static void
mm_buffer_unmap(struct pb_buffer *buf)
{
   /* No-op */
}


static void
mm_buffer_get_base_buffer(struct pb_buffer *buf,
                          struct pb_buffer **base_buf,
                          unsigned *offset)
{
   struct mm_buffer *mm_buf = mm_buffer(buf);
   struct mm_pb_manager *mm = mm_buf->mgr;
   pb_get_base_buffer(mm->buffer, base_buf, offset);
   *offset += mm_buf->block->ofs;
}


static const struct pb_vtbl 
mm_buffer_vtbl = {
      mm_buffer_destroy,
      mm_buffer_map,
      mm_buffer_unmap,
      mm_buffer_get_base_buffer
};


static struct pb_buffer *
mm_bufmgr_create_buffer(struct pb_manager *mgr, 
                        size_t size,
                        const struct pb_desc *desc)
{
   struct mm_pb_manager *mm = mm_pb_manager(mgr);
   struct mm_buffer *mm_buf;

   /* We don't handle alignments larger then the one initially setup */
   assert(desc->alignment % (1 << mm->align2) == 0);
   if(desc->alignment % (1 << mm->align2))
      return NULL;
   
   pipe_mutex_lock(mm->mutex);

   mm_buf = CALLOC_STRUCT(mm_buffer);
   if (!mm_buf) {
      pipe_mutex_unlock(mm->mutex);
      return NULL;
   }

   mm_buf->base.base.refcount = 1;
   mm_buf->base.base.alignment = desc->alignment;
   mm_buf->base.base.usage = desc->usage;
   mm_buf->base.base.size = size;
   
   mm_buf->base.vtbl = &mm_buffer_vtbl;
   
   mm_buf->mgr = mm;
   
   mm_buf->block = mmAllocMem(mm->heap, size, mm->align2, 0);
   if(!mm_buf->block) {
      debug_printf("warning: heap full\n");
#if 0
      mmDumpMemInfo(mm->heap);
#endif
      
      mm_buf->block = mmAllocMem(mm->heap, size, mm->align2, 0);
      if(!mm_buf->block) {
         FREE(mm_buf);
         pipe_mutex_unlock(mm->mutex);
         return NULL;
      }
   }
   
   /* Some sanity checks */
   assert(0 <= (unsigned)mm_buf->block->ofs && (unsigned)mm_buf->block->ofs < mm->size);
   assert(size <= (unsigned)mm_buf->block->size && (unsigned)mm_buf->block->ofs + (unsigned)mm_buf->block->size <= mm->size);
   
   pipe_mutex_unlock(mm->mutex);
   return SUPER(mm_buf);
}


static void
mm_bufmgr_destroy(struct pb_manager *mgr)
{
   struct mm_pb_manager *mm = mm_pb_manager(mgr);
   
   pipe_mutex_lock(mm->mutex);

   mmDestroy(mm->heap);
   
   pb_unmap(mm->buffer);
   pb_reference(&mm->buffer, NULL);
   
   pipe_mutex_unlock(mm->mutex);
   
   FREE(mgr);
}


struct pb_manager *
mm_bufmgr_create_from_buffer(struct pb_buffer *buffer, 
                             size_t size, size_t align2) 
{
   struct mm_pb_manager *mm;

   if(!buffer)
      return NULL;
   
   mm = CALLOC_STRUCT(mm_pb_manager);
   if (!mm)
      return NULL;

   mm->base.create_buffer = mm_bufmgr_create_buffer;
   mm->base.destroy = mm_bufmgr_destroy;

   mm->size = size;
   mm->align2 = align2; /* 64-byte alignment */

   pipe_mutex_init(mm->mutex);

   mm->buffer = buffer; 

   mm->map = pb_map(mm->buffer, 
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
      pb_unmap(mm->buffer);
   if(mm)
      FREE(mm);
   return NULL;
}


struct pb_manager *
mm_bufmgr_create(struct pb_manager *provider, 
                 size_t size, size_t align2) 
{
   struct pb_buffer *buffer;
   struct pb_manager *mgr;
   struct pb_desc desc;

   if(!provider)
      return NULL;
   
   memset(&desc, 0, sizeof(desc));
   desc.alignment = 1 << align2;
   
   buffer = provider->create_buffer(provider, size, &desc); 
   if (!buffer)
      return NULL;
   
   mgr = mm_bufmgr_create_from_buffer(buffer, size, align2);
   if (!mgr) {
      pb_reference(&buffer, NULL);
      return NULL;
   }

  return mgr;
}
