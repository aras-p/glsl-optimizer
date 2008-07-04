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
 * Buffer management.
 * 
 * A buffer manager does only one basic thing: it creates buffers. Actually,
 * "buffer factory" would probably a more accurate description.
 * 
 * You can chain buffer managers so that you can have a finer grained memory
 * management and pooling.
 * 
 * For example, for a simple batch buffer manager you would chain:
 * - the native buffer manager, which provides DMA memory from the graphics
 * memory space;
 * - the pool buffer manager, which keep around a pool of equally sized buffers
 * to avoid latency associated with the native buffer manager; 
 * - the fenced buffer manager, which will delay buffer destruction until the 
 * the moment the card finishing processing it. 
 * 
 * \author José Fonseca <jrfonseca@tungstengraphics.com>
 */

#ifndef PB_BUFMGR_H_
#define PB_BUFMGR_H_


#include "pipe/p_compiler.h"
#include "pipe/p_error.h"


#ifdef __cplusplus
extern "C" {
#endif


struct pb_desc;
struct pipe_buffer;
struct pipe_winsys;


/** 
 * Abstract base class for all buffer managers.
 */
struct pb_manager
{
   struct pb_buffer *
   (*create_buffer)( struct pb_manager *mgr, 
	             size_t size,
	             const struct pb_desc *desc);

   void
   (*destroy)( struct pb_manager *mgr );
};


/**
 * Malloc buffer provider.
 * 
 * Simple wrapper around pb_malloc_buffer_create for convenience.
 */
struct pb_manager *
pb_malloc_bufmgr_create(void);


/** 
 * Static buffer pool sub-allocator.
 * 
 * Manages the allocation of equally sized buffers. It does so by allocating
 * a single big buffer and divide it equally sized buffers. 
 * 
 * It is meant to manage the allocation of batch buffer pools.
 */
struct pb_manager *
pool_bufmgr_create(struct pb_manager *provider, 
                   size_t n, size_t size,
                   const struct pb_desc *desc);


/** 
 * Static sub-allocator based the old memory manager.
 * 
 * It managers buffers of different sizes. It does so by allocating a buffer
 * with the size of the heap, and then using the old mm memory manager to manage
 * that heap. 
 */
struct pb_manager *
mm_bufmgr_create(struct pb_manager *provider, 
                 size_t size, size_t align2);

/**
 * Same as mm_bufmgr_create.
 * 
 * Buffer will be release when the manager is destroyed.
 */
struct pb_manager *
mm_bufmgr_create_from_buffer(struct pb_buffer *buffer, 
                             size_t size, size_t align2);


/**
 * Slab sub-allocator.
 */
struct pb_manager *
pb_slab_manager_create(struct pb_manager *provider,
                       size_t bufSize,
                       size_t slabSize,
                       const struct pb_desc *desc);

/**
 * Allow a range of buffer size, by aggregating multiple slabs sub-allocators 
 * with different bucket sizes.
 */
struct pb_manager *
pb_slab_range_manager_create(struct pb_manager *provider,
                             size_t minBufSize,
                             size_t maxBufSize,
                             size_t slabSize,
                             const struct pb_desc *desc);


/** 
 * Time-based buffer cache.
 *
 * This manager keeps a cache of destroyed buffers during a time interval. 
 */
struct pb_manager *
pb_cache_manager_create(struct pb_manager *provider, 
                     	unsigned usecs); 

void
pb_cache_flush(struct pb_manager *mgr);


/** 
 * Fenced buffer manager.
 *
 * This manager is just meant for convenience. It wraps the buffers returned
 * by another manager in fenced buffers, so that  
 * 
 * NOTE: the buffer manager that provides the buffers will be destroyed
 * at the same time.
 */
struct pb_manager *
fenced_bufmgr_create(struct pb_manager *provider,
                     struct pipe_winsys *winsys);


struct pb_manager *
pb_alt_manager_create(struct pb_manager *provider1, 
                      struct pb_manager *provider2);


/** 
 * Debug buffer manager to detect buffer under- and overflows.
 *
 * Band size should be a multiple of the largest alignment
 */
struct pb_manager *
pb_debug_manager_create(struct pb_manager *provider, size_t band_size); 


#ifdef __cplusplus
}
#endif

#endif /*PB_BUFMGR_H_*/
