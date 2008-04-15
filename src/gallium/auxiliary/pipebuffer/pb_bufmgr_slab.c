/**************************************************************************
 *
 * Copyright 2006-2008 Tungsten Graphics, Inc., Cedar Park, TX., USA
 * All Rights Reserved.
 *
 * Permission is hereby granted, FREE of charge, to any person obtaining a
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
 *
 **************************************************************************/

/**
 * @file
 * S-lab pool implementation.
 * 
 * @author Thomas Hellstrom <thomas-at-tungstengraphics-dot-com>
 * @author Jose Fonseca <jrfonseca@tungstengraphics.com>
 */

#include "pipe/p_compiler.h"
#include "pipe/p_error.h"
#include "pipe/p_debug.h"
#include "pipe/p_thread.h"
#include "pipe/p_defines.h"
#include "pipe/p_util.h"
#include "util/u_double_list.h"
#include "util/u_time.h"

#include "pb_buffer.h"
#include "pb_bufmgr.h"


#define DRI_SLABPOOL_ALLOC_RETRIES 100


struct pb_slab;

struct pb_slab_buffer
{
   struct pb_buffer base;
   
   struct pb_slab *slab;
   struct list_head head;
   unsigned mapCount;
   size_t start;
   _glthread_Cond event;
};

struct pb_slab
{
   struct list_head head;
   struct list_head freeBuffers;
   size_t numBuffers;
   size_t numFree;
   struct pb_slab_buffer *buffers;
   struct pb_slab_size_header *header;
   
   struct pb_buffer *bo;
   size_t pageAlignment;
   void *virtual;   
};

struct pb_slab_size_header 
{
   struct list_head slabs;
   struct list_head freeSlabs;
   struct pb_slab_manager *pool;
   size_t bufSize;
   _glthread_Mutex mutex;
};

/**
 * The data of this structure remains constant after
 * initialization and thus needs no mutex protection.
 */
struct pb_slab_manager 
{
   struct pb_manager base;

   struct pb_desc desc;
   size_t *bucketSizes;
   size_t numBuckets;
   size_t pageSize;
   struct pb_manager *provider;
   unsigned pageAlignment;
   unsigned maxSlabSize;
   unsigned desiredNumBuffers;
   struct pb_slab_size_header *headers;
};


static INLINE struct pb_slab_buffer *
pb_slab_buffer(struct pb_buffer *buf)
{
   assert(buf);
   return (struct pb_slab_buffer *)buf;
}


static INLINE struct pb_slab_manager *
pb_slab_manager(struct pb_manager *mgr)
{
   assert(mgr);
   return (struct pb_slab_manager *)mgr;
}


/**
 * Delete a buffer from the slab header delayed list and put
 * it on the slab FREE list.
 */
static void
pb_slab_buffer_destroy(struct pb_buffer *_buf)
{
   struct pb_slab_buffer *buf = pb_slab_buffer(_buf);
   struct pb_slab *slab = buf->slab;
   struct pb_slab_size_header *header = slab->header;
   struct list_head *list = &buf->head;

   _glthread_LOCK_MUTEX(header->mutex);
   
   assert(buf->base.base.refcount == 0);
   
   buf->mapCount = 0;

   LIST_DEL(list);
   LIST_ADDTAIL(list, &slab->freeBuffers);
   slab->numFree++;

   if (slab->head.next == &slab->head)
      LIST_ADDTAIL(&slab->head, &header->slabs);

   if (slab->numFree == slab->numBuffers) {
      list = &slab->head;
      LIST_DEL(list);
      LIST_ADDTAIL(list, &header->freeSlabs);
   }

   if (header->slabs.next == &header->slabs || slab->numFree
	 != slab->numBuffers) {

      struct list_head *next;

      for (list = header->freeSlabs.next, next = list->next; list
	    != &header->freeSlabs; list = next, next = list->next) {

	 slab = LIST_ENTRY(struct pb_slab, list, head);

	 LIST_DELINIT(list);
	 pb_reference(&slab->bo, NULL);
	 FREE(slab->buffers);
	 FREE(slab);
      }
   }
   
   _glthread_UNLOCK_MUTEX(header->mutex);
}


static void *
pb_slab_buffer_map(struct pb_buffer *_buf, 
                   unsigned flags)
{
   struct pb_slab_buffer *buf = pb_slab_buffer(_buf);

   ++buf->mapCount;
   return (void *) ((uint8_t *) buf->slab->virtual + buf->start);
}


static void
pb_slab_buffer_unmap(struct pb_buffer *_buf)
{
   struct pb_slab_buffer *buf = pb_slab_buffer(_buf);

   --buf->mapCount;
   if (buf->mapCount == 0) 
       _glthread_COND_BROADCAST(buf->event);
}


static void
pb_slab_buffer_get_base_buffer(struct pb_buffer *_buf,
                               struct pb_buffer **base_buf,
                               unsigned *offset)
{
   struct pb_slab_buffer *buf = pb_slab_buffer(_buf);
   pb_get_base_buffer(buf->slab->bo, base_buf, offset);
   *offset += buf->start;
}


static const struct pb_vtbl 
pb_slab_buffer_vtbl = {
      pb_slab_buffer_destroy,
      pb_slab_buffer_map,
      pb_slab_buffer_unmap,
      pb_slab_buffer_get_base_buffer
};


static enum pipe_error
pb_slab_create(struct pb_slab_size_header *header)
{
   struct pb_slab_manager *pool = header->pool;
   size_t size = header->bufSize * pool->desiredNumBuffers;
   struct pb_slab *slab;
   struct pb_slab_buffer *buf;
   size_t numBuffers;
   int ret;
   unsigned i;

   slab = CALLOC_STRUCT(pb_slab);
   if (!slab)
      return PIPE_ERROR_OUT_OF_MEMORY;

   /*
    * FIXME: We should perhaps allow some variation in slabsize in order
    * to efficiently reuse slabs.
    */

   size = (size <= pool->maxSlabSize) ? size : pool->maxSlabSize;
   size = (size + pool->pageSize - 1) & ~(pool->pageSize - 1);

   slab->bo = pool->provider->create_buffer(pool->provider, size, &pool->desc);
   if(!slab->bo)
      goto out_err0;

   slab->virtual = pb_map(slab->bo, 
			 PIPE_BUFFER_USAGE_CPU_READ |
			 PIPE_BUFFER_USAGE_CPU_WRITE);
   if(!slab->virtual)
      goto out_err1;

   pb_unmap(slab->bo);

   numBuffers = slab->bo->base.size / header->bufSize;

   slab->buffers = CALLOC(numBuffers, sizeof(*slab->buffers));
   if (!slab->buffers) {
      ret = PIPE_ERROR_OUT_OF_MEMORY;
      goto out_err1;
   }

   LIST_INITHEAD(&slab->head);
   LIST_INITHEAD(&slab->freeBuffers);
   slab->numBuffers = numBuffers;
   slab->numFree = 0;
   slab->header = header;

   buf = slab->buffers;
   for (i=0; i < numBuffers; ++i) {
      buf->base.base.refcount = 0;
      buf->base.base.size = header->bufSize;
      buf->base.base.alignment = 0;
      buf->base.base.usage = 0;
      buf->base.vtbl = &pb_slab_buffer_vtbl;
      buf->slab = slab;
      buf->start = i* header->bufSize;
      buf->mapCount = 0;
      _glthread_INIT_COND(buf->event);
      LIST_ADDTAIL(&buf->head, &slab->freeBuffers);
      slab->numFree++;
      buf++;
   }

   LIST_ADDTAIL(&slab->head, &header->slabs);

   return PIPE_OK;

out_err1: 
   pb_reference(&slab->bo, NULL);
out_err0: 
   FREE(slab);
   return ret;
}


static struct pb_buffer *
pb_slab_manager_create_buffer(struct pb_manager *_pool,
                              size_t size,
                              const struct pb_desc *desc)
{
   struct pb_slab_manager *pool = pb_slab_manager(_pool);
   struct pb_slab_size_header *header;
   unsigned i;
   static struct pb_slab_buffer *buf;
   struct pb_slab *slab;
   struct list_head *list;
   int count = DRI_SLABPOOL_ALLOC_RETRIES;

   /*
    * FIXME: Check for compatibility.
    */

   header = pool->headers;
   for (i=0; i<pool->numBuckets; ++i) {
      if (header->bufSize >= size)
	 break;
      header++;
   }

   if (i >= pool->numBuckets)
      /* Fall back to allocate a buffer object directly from the provider. */
      return pool->provider->create_buffer(pool->provider, size, desc);


   _glthread_LOCK_MUTEX(header->mutex);
   while (header->slabs.next == &header->slabs && count > 0) {
      if (header->slabs.next != &header->slabs)
	 break;

      _glthread_UNLOCK_MUTEX(header->mutex);
      if (count != DRI_SLABPOOL_ALLOC_RETRIES)
	 util_time_sleep(1);
      _glthread_LOCK_MUTEX(header->mutex);
      (void) pb_slab_create(header);
      count--;
   }

   list = header->slabs.next;
   if (list == &header->slabs) {
      _glthread_UNLOCK_MUTEX(header->mutex);
      return NULL;
   }
   slab = LIST_ENTRY(struct pb_slab, list, head);
   if (--slab->numFree == 0)
      LIST_DELINIT(list);

   list = slab->freeBuffers.next;
   LIST_DELINIT(list);

   _glthread_UNLOCK_MUTEX(header->mutex);
   buf = LIST_ENTRY(struct pb_slab_buffer, list, head);
   ++buf->base.base.refcount;
   return &buf->base;
}


static void
pb_slab_manager_destroy(struct pb_manager *_pool)
{
   struct pb_slab_manager *pool = pb_slab_manager(_pool);

   FREE(pool->headers);
   FREE(pool->bucketSizes);
   FREE(pool);
}


struct pb_manager *
pb_slab_manager_create(struct pb_manager *provider, 
                       const struct pb_desc *desc,
                       size_t smallestSize,
                       size_t numSizes,
                       size_t desiredNumBuffers,
                       size_t maxSlabSize,
                       size_t pageAlignment)
{
   struct pb_slab_manager *pool;
   size_t i;

   pool = CALLOC_STRUCT(pb_slab_manager);
   if (!pool)
      goto out_err0;

   pool->bucketSizes = CALLOC(numSizes, sizeof(*pool->bucketSizes));
   if (!pool->bucketSizes)
      goto out_err1;

   pool->headers = CALLOC(numSizes, sizeof(*pool->headers));
   if (!pool->headers)
      goto out_err2;

   pool->desc = *desc;
   pool->numBuckets = numSizes;
#ifdef WIN32
   pool->pageSize = 4096;
#else
   pool->pageSize = getpagesize();
#endif
   pool->provider = provider;
   pool->pageAlignment = pageAlignment;
   pool->maxSlabSize = maxSlabSize;
   pool->desiredNumBuffers = desiredNumBuffers;

   for (i=0; i<pool->numBuckets; ++i) {
      struct pb_slab_size_header *header = &pool->headers[i];
      
      pool->bucketSizes[i] = (smallestSize << i);
      
      _glthread_INIT_MUTEX(header->mutex);

      LIST_INITHEAD(&header->slabs);
      LIST_INITHEAD(&header->freeSlabs);

      header->pool = pool;
      header->bufSize = (smallestSize << i);
   }

   pool->base.destroy = pb_slab_manager_destroy;
   pool->base.create_buffer = pb_slab_manager_create_buffer;

   return &pool->base;

out_err2: 
   FREE(pool->bucketSizes);
out_err1: 
   FREE(pool);
out_err0:
   return NULL;
}
