/**************************************************************************
 * 
 * Copyright 2009 VMware, Inc.
 * Copyright 2012 Marek Olšák <maraeo@gmail.com>
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
 * IN NO EVENT SHALL THE AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

/* A simple allocator that suballocates memory from a large buffer. */

#include "pipe/p_defines.h"
#include "util/u_inlines.h"
#include "pipe/p_context.h"
#include "util/u_memory.h"
#include "util/u_math.h"

#include "u_suballoc.h"


struct u_suballocator {
   struct pipe_context *pipe;

   unsigned size;          /* Size of the whole buffer, in bytes. */
   unsigned alignment;     /* Alignment of each sub-allocation. */
   unsigned bind;          /* Bitmask of PIPE_BIND_* flags. */
   unsigned usage;         /* One of PIPE_USAGE_* flags. */
   boolean zero_buffer_memory; /* If the buffer contents should be zeroed. */

   struct pipe_resource *buffer;   /* The buffer we suballocate from. */
   unsigned offset; /* Aligned offset pointing at the first unused byte. */
};


/**
 * Create a suballocator.
 *
 * \p zero_buffer_memory determines whether the buffer contents should be
 * cleared to 0 after the allocation.
 */
struct u_suballocator *
u_suballocator_create(struct pipe_context *pipe, unsigned size,
                      unsigned alignment, unsigned bind, unsigned usage,
		      boolean zero_buffer_memory)
{
   struct u_suballocator *allocator = CALLOC_STRUCT(u_suballocator);
   if (!allocator)
      return NULL;

   allocator->pipe = pipe;
   allocator->size = align(size, alignment);
   allocator->alignment = alignment;
   allocator->bind = bind;
   allocator->usage = usage;
   allocator->zero_buffer_memory = zero_buffer_memory;
   return allocator;
}

void
u_suballocator_destroy(struct u_suballocator *allocator)
{
   pipe_resource_reference(&allocator->buffer, NULL);
   FREE(allocator);
}

void
u_suballocator_alloc(struct u_suballocator *allocator, unsigned size,
                     unsigned *out_offset, struct pipe_resource **outbuf)
{
   unsigned alloc_size = align(size, allocator->alignment);

   /* Don't allow allocations larger than the buffer size. */
   if (alloc_size > allocator->size)
      goto fail;

   /* Make sure we have enough space in the buffer. */
   if (!allocator->buffer ||
       allocator->offset + alloc_size > allocator->size) {
      /* Allocate a new buffer. */
      pipe_resource_reference(&allocator->buffer, NULL);
      allocator->offset = 0;
      allocator->buffer =
         pipe_buffer_create(allocator->pipe->screen, allocator->bind,
                            allocator->usage, allocator->size);
      if (!allocator->buffer)
         goto fail;

      /* Clear the memory if needed. */
      if (allocator->zero_buffer_memory) {
         struct pipe_transfer *transfer = NULL;
         void *ptr;

         ptr = pipe_buffer_map(allocator->pipe, allocator->buffer,
			       PIPE_TRANSFER_WRITE, &transfer);
         memset(ptr, 0, allocator->size);
         pipe_buffer_unmap(allocator->pipe, transfer);
      }
   }

   assert(allocator->offset % allocator->alignment == 0);
   assert(allocator->offset < allocator->buffer->width0);
   assert(allocator->offset + alloc_size <= allocator->buffer->width0);

   /* Return the buffer. */
   *out_offset = allocator->offset;
   pipe_resource_reference(outbuf, allocator->buffer);

   allocator->offset += alloc_size;
   return;

fail:
   pipe_resource_reference(outbuf, NULL);
}
