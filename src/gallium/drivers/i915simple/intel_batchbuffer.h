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

#ifndef INTEL_BATCH_H
#define INTEL_BATCH_H

#include "intel_winsys.h"

static INLINE boolean
intel_batchbuffer_check(struct intel_batchbuffer *batch,
                        size_t dwords,
                        size_t relocs)
{
   return dwords * 4 <= batch->size - (batch->ptr - batch->map) &&
          relocs <= (batch->max_relocs - batch->relocs);
}

static INLINE size_t
intel_batchbuffer_space(struct intel_batchbuffer *batch)
{
   return batch->size - (batch->ptr - batch->map);
}

static INLINE void
intel_batchbuffer_dword(struct intel_batchbuffer *batch,
                        unsigned dword)
{
   if (intel_batchbuffer_space(batch) < 4)
      return;

   *(unsigned *)batch->ptr = dword;
   batch->ptr += 4;
}

static INLINE void
intel_batchbuffer_write(struct intel_batchbuffer *batch,
                        void *data,
                        size_t size)
{
   if (intel_batchbuffer_space(batch) < size)
      return;

   memcpy(data, batch->ptr, size);
   batch->ptr += size;
}

static INLINE int
intel_batchbuffer_reloc(struct intel_batchbuffer *batch,
                        struct intel_buffer *buffer,
                        enum intel_buffer_usage usage,
                        size_t offset)
{
   return batch->iws->batchbuffer_reloc(batch, buffer, usage, offset);
}

static INLINE void
intel_batchbuffer_flush(struct intel_batchbuffer *batch,
                        struct pipe_fence_handle **fence)
{
   batch->iws->batchbuffer_flush(batch, fence);
}

#endif
