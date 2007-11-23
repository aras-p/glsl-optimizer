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


#include <stddef.h>


struct pipe_buffer;
struct pipe_winsys;


/** 
 * Abstract base class for all buffer managers.
 */
struct buffer_manager
{
   /* XXX: we will likely need more allocation flags */
   struct pipe_buffer *
   (*create_buffer)( struct buffer_manager *mgr, 
	             size_t size);

   void
   (*destroy)( struct buffer_manager *mgr );
};


/** 
 * Static buffer pool manager.
 * 
 * Manages the allocation of equally sized buffers. It does so by allocating
 * a single big buffer and divide it equally sized buffers. 
 * 
 * It is meant to manage the allocation of batch buffer pools.
 */
struct buffer_manager *
pool_bufmgr_create(struct buffer_manager *provider, 
                   size_t n, size_t size);


/** 
 * Wraper around the old memory manager.
 * 
 * It managers buffers of different sizes. It does so by allocating a buffer
 * with the size of the heap, and then using the old mm memory manager to manage
 * that heap. 
 */
struct buffer_manager *
mm_bufmgr_create(struct buffer_manager *provider, 
                 size_t size, size_t align2);


/** 
 * Fenced buffer manager.
 *
 * This manager is just meant for convenience. It wraps the buffers returned
 * by another manager in fenced buffers, so that  
 * 
 * NOTE: the buffer manager that provides the buffers will be destroyed
 * at the same time.
 */
struct buffer_manager *
fenced_bufmgr_create(struct buffer_manager *provider,
                     struct pipe_winsys *winsys);


#endif /*PB_BUFMGR_H_*/
