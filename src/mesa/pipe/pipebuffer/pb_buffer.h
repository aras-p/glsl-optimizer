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
 * Generic code for buffers.
 * 
 * Behind a pipe buffle handle there can be DMA buffers, client (or user) 
 * buffers, regular malloced buffers, etc. This file provides an abstract base 
 * buffer handle that allows the driver to cope with all those kinds of buffers 
 * in a more flexible way.
 * 
 * There is no obligation of a winsys driver to use this library. And a pipe
 * driver should be completly agnostic about it.
 * 
 * \author José Fonseca <jrfonseca@tungstengraphics.com>
 */

#ifndef PB_BUFFER_H_
#define PB_BUFFER_H_


#include <assert.h>
#include <stdlib.h>


struct pipe_buffer_vtbl;


/**
 * Base class for all pipe buffers.
 */
struct pipe_buffer 
{
   /**
    * Pointer to the virtual function table.
    *
    * Avoid accessing this table directly. Use the inline functions below 
    * instead to avoid mistakes. 
    */
   const struct pipe_buffer_vtbl *vtbl;
};


/**
 * Virtual function table for the buffer storage operations.
 * 
 * Note that creation is not done through this table.
 */
struct pipe_buffer_vtbl
{
   /**
    * Add a reference to the buffer.
    *
    * This method can be a no-op for buffers that don't need reference
    * counting.
    */
   void (*reference)( struct pipe_buffer *buf );

   /**
    * Release a reference to this buffer and destroy it.
    */
   void (*release)( struct pipe_buffer *buf );

   /** 
    * Map the entire data store of a buffer object into the client's address.
    * flags is bitmask of PIPE_BUFFER_FLAG_READ/WRITE. 
    */
   void *(*map)( struct pipe_buffer *buf, 
                 unsigned flags );
   
   void (*unmap)( struct pipe_buffer *buf );

   /**
    * Get the base buffer and the offset.
    * 
    * A buffer can be subdivided in smaller buffers. This method should return
    * the underlaying buffer, and the relative offset.
    * 
    * Buffers without an underlaying base buffer should return themselves, with 
    * a zero offset.
    * 
    * Note that this will increase the reference count of the base buffer.
    */
   void (*get_base_buffer)( struct pipe_buffer *buf,
                            struct pipe_buffer **base_buf,
                            unsigned *offset );
};


/** *dst = src with reference counting */
void
buffer_reference(struct pipe_buffer **dst,
                 struct pipe_buffer *src);


static inline void
buffer_release(struct pipe_buffer *buf)
{
   assert(buf);
   buf->vtbl->release(buf);
}


static inline void *
buffer_map(struct pipe_buffer *buf, 
           unsigned flags)
{
   assert(buf);
   return buf->vtbl->map(buf, flags);
}


static inline void 
buffer_unmap(struct pipe_buffer *buf)
{
   assert(buf);
   buf->vtbl->unmap(buf);
}


static inline void
buffer_get_base_buffer( struct pipe_buffer *buf,
                        struct pipe_buffer **base_buf,
                        unsigned *offset )
{
   buf->vtbl->get_base_buffer(buf, base_buf, offset);
}


/** Placeholder for empty buffers. */
extern struct pipe_buffer null_buffer;


/**
 * Client buffers (also designated as user buffers) are just for convenience 
 * of the state tracker, so that it can masquerade its own data as a buffer.
 */
struct pipe_buffer *
client_buffer_create(void *data);


/**
 * Malloc-based buffer to store data that can't be used by the graphics 
 * hardware.
 */
struct pipe_buffer *
malloc_buffer_create(unsigned size);


#endif /*PB_BUFFER_H_*/
