/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics (http://www.tungstengraphics.com) to
 develop this 3D driver.
 
 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:
 
 The above copyright notice and this permission notice (including the
 next paragraph) shall be included in all copies or substantial
 portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 
 **********************************************************************/
 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  */
     
#include "brw_state.h"
#include "intel_batchbuffer.h"
#include "main/imports.h"

/**
 * Allocates a block of space in the batchbuffer for indirect state.
 *
 * We don't want to allocate separate BOs for every bit of indirect
 * state in the driver.  It means overallocating by a significant
 * margin (4096 bytes, even if the object is just a 20-byte surface
 * state), and more buffers to walk and count for aperture size checking.
 *
 * However, due to the restrictions inposed by the aperture size
 * checking performance hacks, we can't have the batch point at a
 * separate indirect state buffer, because once the batch points at
 * it, no more relocations can be added to it.  So, we sneak these
 * buffers in at the top of the batchbuffer.
 */
void *
brw_state_batch(struct brw_context *brw,
		int size,
		int alignment,
		uint32_t *out_offset)
{
   struct intel_batchbuffer *batch = &brw->intel.batch;
   uint32_t offset;

   assert(size < batch->bo->size);
   offset = ROUND_DOWN_TO(batch->state_batch_offset - size, alignment);

   /* If allocating from the top would wrap below the batchbuffer, or
    * if the batch's used space (plus the reserved pad) collides with our
    * space, then flush and try again.
    */
   if (batch->state_batch_offset < size ||
       offset < 4*batch->used + batch->reserved_space) {
      intel_batchbuffer_flush(&brw->intel);
      offset = ROUND_DOWN_TO(batch->state_batch_offset - size, alignment);
   }

   batch->state_batch_offset = offset;

   *out_offset = offset;
   return batch->map + (offset>>2);
}
