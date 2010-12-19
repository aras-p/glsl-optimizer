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



/* A facility similar to the data caching code above, which aims to
 * prevent identical commands being issued repeatedly.
 */
GLboolean brw_cached_batch_struct( struct brw_context *brw,
				   const void *data,
				   GLuint sz )
{
   struct brw_cached_batch_item *item = brw->cached_batch_items;
   struct header *newheader = (struct header *)data;

   if (brw->emit_state_always) {
      intel_batchbuffer_data(brw->intel.batch, data, sz, false);
      return GL_TRUE;
   }

   while (item) {
      if (item->header->opcode == newheader->opcode) {
	 if (item->sz == sz && memcmp(item->header, newheader, sz) == 0)
	    return GL_FALSE;
	 if (item->sz != sz) {
	    free(item->header);
	    item->header = malloc(sz);
	    item->sz = sz;
	 }
	 goto emit;
      }
      item = item->next;
   }

   assert(!item);
   item = CALLOC_STRUCT(brw_cached_batch_item);
   item->header = malloc(sz);
   item->sz = sz;
   item->next = brw->cached_batch_items;
   brw->cached_batch_items = item;

 emit:
   memcpy(item->header, newheader, sz);
   intel_batchbuffer_data(brw->intel.batch, data, sz, false);
   return GL_TRUE;
}

void brw_clear_batch_cache( struct brw_context *brw )
{
   struct brw_cached_batch_item *item = brw->cached_batch_items;

   while (item) {
      struct brw_cached_batch_item *next = item->next;
      free((void *)item->header);
      free(item);
      item = next;
   }

   brw->cached_batch_items = NULL;
}

void brw_destroy_batch_cache( struct brw_context *brw )
{
   brw_clear_batch_cache(brw);
}

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
		drm_intel_bo **out_bo,
		uint32_t *out_offset)
{
   struct intel_batchbuffer *batch = brw->intel.batch;
   uint32_t offset;

   assert(size < batch->buf->size);
   offset = ROUND_DOWN_TO(batch->state_batch_offset - size, alignment);

   /* If allocating from the top would wrap below the batchbuffer, or
    * if the batch's used space (plus the reserved pad) collides with our
    * space, then flush and try again.
    */
   if (batch->state_batch_offset < size ||
       offset < batch->ptr - batch->map + batch->reserved_space) {
      intel_batchbuffer_flush(batch);
      offset = ROUND_DOWN_TO(batch->state_batch_offset - size, alignment);
   }

   batch->state_batch_offset = offset;

   if (*out_bo != batch->buf) {
      drm_intel_bo_unreference(*out_bo);
      drm_intel_bo_reference(batch->buf);
      *out_bo = batch->buf;
   }

   *out_offset = offset;
   return batch->map + offset;
}
