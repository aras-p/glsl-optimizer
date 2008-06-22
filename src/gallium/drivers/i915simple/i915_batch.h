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

#ifndef I915_BATCH_H
#define I915_BATCH_H

#include "i915_winsys.h"

struct i915_batchbuffer
{
   struct pipe_buffer *buffer;
   struct i915_winsys *winsys;

   unsigned char *map;
   unsigned char *ptr;

   size_t size;
   size_t actual_size;

   size_t relocs;
   size_t max_relocs;
};

static INLINE boolean
i915_batchbuffer_check( struct i915_batchbuffer *batch,
			size_t dwords,
			size_t relocs )
{
   /** TODO JB: Check relocs */
   return dwords * 4 <= batch->size - (batch->ptr - batch->map);
}

static INLINE size_t
i915_batchbuffer_space( struct i915_batchbuffer *batch )
{
   return batch->size - (batch->ptr - batch->map);
}

static INLINE void
i915_batchbuffer_dword( struct i915_batchbuffer *batch,
			unsigned dword )
{
   if (i915_batchbuffer_space(batch) < 4)
      return;

   *(unsigned *)batch->ptr = dword;
   batch->ptr += 4;
}

static INLINE void
i915_batchbuffer_write( struct i915_batchbuffer *batch,
			void *data,
			size_t size )
{
   if (i915_batchbuffer_space(batch) < size)
      return;

   memcpy(data, batch->ptr, size);
   batch->ptr += size;
}

static INLINE void
i915_batchbuffer_reloc( struct i915_batchbuffer *batch,
			struct pipe_buffer *buffer,
			size_t flags,
			size_t offset )
{
   batch->winsys->batch_reloc( batch->winsys, buffer, flags, offset );
}

static INLINE void
i915_batchbuffer_flush( struct i915_batchbuffer *batch,
			struct pipe_fence_handle **fence )
{
   batch->winsys->batch_flush( batch->winsys, fence );
}

#define BEGIN_BATCH( dwords, relocs ) \
   (i915_batchbuffer_check( i915->batch, dwords, relocs ))

#define OUT_BATCH( dword ) \
   i915_batchbuffer_dword( i915->batch, dword )

#define OUT_RELOC( buf, flags, delta ) \
   i915_batchbuffer_reloc( i915->batch, buf, flags, delta )

#define FLUSH_BATCH(fence) do { 			\
   i915->winsys->batch_flush( i915->winsys, fence );	\
   i915->hardware_dirty = ~0;				\
} while (0)

#endif
