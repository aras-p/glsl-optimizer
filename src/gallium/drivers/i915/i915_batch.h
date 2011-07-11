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

#include "i915_batchbuffer.h"
#include "i915_context.h"


#define BEGIN_BATCH(dwords) \
   (i915_winsys_batchbuffer_check(i915->batch, dwords))

#define OUT_BATCH(dword) \
   i915_winsys_batchbuffer_dword(i915->batch, dword)

#define OUT_BATCH_F(f) \
   i915_winsys_batchbuffer_float(i915->batch, f)

#define OUT_RELOC(buf, usage, offset) \
   i915_winsys_batchbuffer_reloc(i915->batch, buf, usage, offset, false)

#define OUT_RELOC_FENCED(buf, usage, offset) \
   i915_winsys_batchbuffer_reloc(i915->batch, buf, usage, offset, true)

#define FLUSH_BATCH(fence) \
   i915_flush(i915, fence)

/************************************************************************
 * i915_flush.c
 */
void i915_flush(struct i915_context *i915, struct pipe_fence_handle **fence);

/*
 * Flush if the current color buf is idle and we have more than 256 vertices
 * queued, or if the current color buf is busy and we have more than 4096
 * vertices queued.
 */
static INLINE void i915_flush_heuristically(struct i915_context* i915,
                                            int num_vertex)
{
   struct i915_winsys *iws = i915->iws;
   i915->vertices_since_last_flush += num_vertex;
   if ( i915->vertices_since_last_flush > 4096
      || ( i915->vertices_since_last_flush > 256 &&
           !iws->buffer_is_busy(iws, i915->current.cbuf_bo)) )
      FLUSH_BATCH(NULL);
}


#endif
