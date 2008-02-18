/**************************************************************************
 *
 * Copyright 2006 Tungsten Graphics, Inc., Cedar Park, Texas.
 * Copyright 1999 Wittawat Yamwong
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


#include "linked_list.h"

#include "pipe/p_defines.h"
#include "pipe/p_debug.h"
#include "pipe/p_thread.h"
#include "pipe/p_util.h"
#include "pb_buffer.h"
#include "pb_bufmgr.h"


/**
 * Convenience macro (type safe).
 */
#define SUPER(__derived) (&(__derived)->base)


struct mem_block 
{
   struct mem_block *next, *prev;
   struct mem_block *next_free, *prev_free;
   struct mem_block *heap;
   int ofs, size;
   unsigned int free:1;
   unsigned int reserved:1;
};


#ifdef DEBUG
/**
 * For debugging purposes.
 */
static void
mmDumpMemInfo(const struct mem_block *heap)
{
   debug_printf("Memory heap %p:\n", (void *)heap);
   if (heap == 0) {
      debug_printf("  heap == 0\n");
   } else {
      const struct mem_block *p;

      for(p = heap->next; p != heap; p = p->next) {
	 debug_printf("  Offset:%08x, Size:%08x, %c%c\n",p->ofs,p->size,
		 p->free ? 'F':'.',
		 p->reserved ? 'R':'.');
      }

      debug_printf("\nFree list:\n");

      for(p = heap->next_free; p != heap; p = p->next_free) {
	 debug_printf(" FREE Offset:%08x, Size:%08x, %c%c\n",p->ofs,p->size,
		 p->free ? 'F':'.',
		 p->reserved ? 'R':'.');
      }

   }
   debug_printf("End of memory blocks\n");
}
#endif


/** 
 * input: total size in bytes
 * return: a heap pointer if OK, NULL if error
 */
static struct mem_block *
mmInit(int ofs, int size)
{
   struct mem_block *heap, *block;
  
   if (size <= 0) 
      return NULL;

   heap = CALLOC_STRUCT(mem_block);
   if (!heap) 
      return NULL;
   
   block = CALLOC_STRUCT(mem_block);
   if (!block) {
      FREE(heap);
      return NULL;
   }

   heap->next = block;
   heap->prev = block;
   heap->next_free = block;
   heap->prev_free = block;

   block->heap = heap;
   block->next = heap;
   block->prev = heap;
   block->next_free = heap;
   block->prev_free = heap;

   block->ofs = ofs;
   block->size = size;
   block->free = 1;

   return heap;
}


static struct mem_block *
SliceBlock(struct mem_block *p, 
           int startofs, int size, 
           int reserved, int alignment)
{
   struct mem_block *newblock;

   /* break left  [p, newblock, p->next], then p = newblock */
   if (startofs > p->ofs) {
      newblock = CALLOC_STRUCT(mem_block);
      if (!newblock)
	 return NULL;
      newblock->ofs = startofs;
      newblock->size = p->size - (startofs - p->ofs);
      newblock->free = 1;
      newblock->heap = p->heap;

      newblock->next = p->next;
      newblock->prev = p;
      p->next->prev = newblock;
      p->next = newblock;

      newblock->next_free = p->next_free;
      newblock->prev_free = p;
      p->next_free->prev_free = newblock;
      p->next_free = newblock;

      p->size -= newblock->size;
      p = newblock;
   }

   /* break right, also [p, newblock, p->next] */
   if (size < p->size) {
      newblock = CALLOC_STRUCT(mem_block);
      if (!newblock)
	 return NULL;
      newblock->ofs = startofs + size;
      newblock->size = p->size - size;
      newblock->free = 1;
      newblock->heap = p->heap;

      newblock->next = p->next;
      newblock->prev = p;
      p->next->prev = newblock;
      p->next = newblock;

      newblock->next_free = p->next_free;
      newblock->prev_free = p;
      p->next_free->prev_free = newblock;
      p->next_free = newblock;
	 
      p->size = size;
   }

   /* p = middle block */
   p->free = 0;

   /* Remove p from the free list: 
    */
   p->next_free->prev_free = p->prev_free;
   p->prev_free->next_free = p->next_free;

   p->next_free = 0;
   p->prev_free = 0;

   p->reserved = reserved;
   return p;
}


/**
 * Allocate 'size' bytes with 2^align2 bytes alignment,
 * restrict the search to free memory after 'startSearch'
 * depth and back buffers should be in different 4mb banks
 * to get better page hits if possible
 * input:	size = size of block
 *       	align2 = 2^align2 bytes alignment
 *		startSearch = linear offset from start of heap to begin search
 * return: pointer to the allocated block, 0 if error
 */
static struct mem_block *
mmAllocMem(struct mem_block *heap, int size, int align2, int startSearch)
{
   struct mem_block *p;
   const int mask = (1 << align2)-1;
   int startofs = 0;
   int endofs;

   if (!heap || align2 < 0 || size <= 0)
      return NULL;

   for (p = heap->next_free; p != heap; p = p->next_free) {
      assert(p->free);

      startofs = (p->ofs + mask) & ~mask;
      if ( startofs < startSearch ) {
	 startofs = startSearch;
      }
      endofs = startofs+size;
      if (endofs <= (p->ofs+p->size))
	 break;
   }

   if (p == heap) 
      return NULL;

   assert(p->free);
   p = SliceBlock(p,startofs,size,0,mask+1);

   return p;
}


#if 0
/**
 * Free block starts at offset
 * input: pointer to a heap, start offset
 * return: pointer to a block
 */
static struct mem_block *
mmFindBlock(struct mem_block *heap, int start)
{
   struct mem_block *p;

   for (p = heap->next; p != heap; p = p->next) {
      if (p->ofs == start) 
	 return p;
   }

   return NULL;
}
#endif


static INLINE int
Join2Blocks(struct mem_block *p)
{
   /* XXX there should be some assertions here */

   /* NOTE: heap->free == 0 */

   if (p->free && p->next->free) {
      struct mem_block *q = p->next;

      assert(p->ofs + p->size == q->ofs);
      p->size += q->size;

      p->next = q->next;
      q->next->prev = p;

      q->next_free->prev_free = q->prev_free; 
      q->prev_free->next_free = q->next_free;
     
      FREE(q);
      return 1;
   }
   return 0;
}


/**
 * Free block starts at offset
 * input: pointer to a block
 * return: 0 if OK, -1 if error
 */
static int
mmFreeMem(struct mem_block *b)
{
   if (!b)
      return 0;

   if (b->free) {
      debug_printf("block already free\n");
      return -1;
   }
   if (b->reserved) {
      debug_printf("block is reserved\n");
      return -1;
   }

   b->free = 1;
   b->next_free = b->heap->next_free;
   b->prev_free = b->heap;
   b->next_free->prev_free = b;
   b->prev_free->next_free = b;

   Join2Blocks(b);
   if (b->prev != b->heap)
      Join2Blocks(b->prev);

   return 0;
}


/**
 * destroy MM
 */
static void
mmDestroy(struct mem_block *heap)
{
   struct mem_block *p;

   if (!heap)
      return;

   for (p = heap->next; p != heap; ) {
      struct mem_block *next = p->next;
      FREE(p);
      p = next;
   }

   FREE(heap);
}


struct mm_pb_manager
{
   struct pb_manager base;
   
   _glthread_Mutex mutex;
   
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
   
   _glthread_LOCK_MUTEX(mm->mutex);
   mmFreeMem(mm_buf->block);
   FREE(buf);
   _glthread_UNLOCK_MUTEX(mm->mutex);
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
   
   _glthread_LOCK_MUTEX(mm->mutex);

   mm_buf = CALLOC_STRUCT(mm_buffer);
   if (!mm_buf) {
      _glthread_UNLOCK_MUTEX(mm->mutex);
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
	 assert(0);
         FREE(mm_buf);
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
mm_bufmgr_destroy(struct pb_manager *mgr)
{
   struct mm_pb_manager *mm = mm_pb_manager(mgr);
   
   _glthread_LOCK_MUTEX(mm->mutex);

   mmDestroy(mm->heap);
   
   pb_unmap(mm->buffer);
   pb_reference(&mm->buffer, NULL);
   
   _glthread_UNLOCK_MUTEX(mm->mutex);
   
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

   _glthread_INIT_MUTEX(mm->mutex);

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

   assert(provider);
   assert(provider->create_buffer);
   
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
