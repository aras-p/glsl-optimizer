/**************************************************************************
 * 
 * Copyright 2006 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#include "intel_context.h"
#include "intel_batchbuffer.h"
#include "intel_buffer_objects.h"
#include "intel_decode.h"
#include "intel_reg.h"
#include "intel_bufmgr.h"
#include "intel_buffers.h"

struct cached_batch_item {
   struct cached_batch_item *next;
   uint16_t header;
   uint16_t size;
};

static void clear_cache( struct intel_context *intel )
{
   struct cached_batch_item *item = intel->batch.cached_items;

   while (item) {
      struct cached_batch_item *next = item->next;
      free(item);
      item = next;
   }

   intel->batch.cached_items = NULL;
}

void
intel_batchbuffer_reset(struct intel_context *intel)
{
   if (intel->batch.bo != NULL) {
      drm_intel_bo_unreference(intel->batch.bo);
      intel->batch.bo = NULL;
   }
   clear_cache(intel);

   intel->batch.bo = drm_intel_bo_alloc(intel->bufmgr, "batchbuffer",
					intel->maxBatchSize, 4096);

   intel->batch.reserved_space = BATCH_RESERVED;
   intel->batch.state_batch_offset = intel->batch.bo->size;
   intel->batch.used = 0;
}

void
intel_batchbuffer_free(struct intel_context *intel)
{
   drm_intel_bo_unreference(intel->batch.bo);
   clear_cache(intel);
}


/* TODO: Push this whole function into bufmgr.
 */
static void
do_flush_locked(struct intel_context *intel)
{
   struct intel_batchbuffer *batch = &intel->batch;
   int ret = 0;

   if (!intel->intelScreen->no_hw) {
      int ring;

      if (intel->gen < 6 || !batch->is_blit) {
	 ring = I915_EXEC_RENDER;
      } else {
	 ring = I915_EXEC_BLT;
      }

      ret = drm_intel_bo_subdata(batch->bo, 0, 4*batch->used, batch->map);
      if (ret == 0 && batch->state_batch_offset != batch->bo->size) {
	 ret = drm_intel_bo_subdata(batch->bo,
				    batch->state_batch_offset,
				    batch->bo->size - batch->state_batch_offset,
				    (char *)batch->map + batch->state_batch_offset);
      }

      if (ret == 0)
	 ret = drm_intel_bo_mrb_exec(batch->bo, 4*batch->used, NULL, 0, 0, ring);
   }

   if (unlikely(INTEL_DEBUG & DEBUG_BATCH)) {
      intel_decode(batch->map, batch->used,
		   batch->bo->offset,
		   intel->intelScreen->deviceID, GL_TRUE);

      if (intel->vtbl.debug_batch != NULL)
	 intel->vtbl.debug_batch(intel);
   }

   if (ret != 0) {
      exit(1);
   }
   intel->vtbl.new_batch(intel);
}

void
_intel_batchbuffer_flush(struct intel_context *intel,
			 const char *file, int line)
{
   if (intel->batch.used == 0)
      return;

   if (unlikely(INTEL_DEBUG & DEBUG_BATCH))
      fprintf(stderr, "%s:%d: Batchbuffer flush with %db used\n", file, line,
	      4*intel->batch.used);

   intel->batch.reserved_space = 0;

   if (intel->always_flush_cache) {
      intel_batchbuffer_emit_mi_flush(intel);
   }

   /* Mark the end of the buffer. */
   intel_batchbuffer_emit_dword(intel, MI_BATCH_BUFFER_END);
   if (intel->batch.used & 1) {
      /* Round batchbuffer usage to 2 DWORDs. */
      intel_batchbuffer_emit_dword(intel, MI_NOOP);
   }

   if (intel->vtbl.finish_batch)
      intel->vtbl.finish_batch(intel);

   intel_upload_finish(intel);

   /* Check that we didn't just wrap our batchbuffer at a bad time. */
   assert(!intel->no_batch_wrap);

   do_flush_locked(intel);

   if (unlikely(INTEL_DEBUG & DEBUG_SYNC)) {
      fprintf(stderr, "waiting for idle\n");
      drm_intel_bo_wait_rendering(intel->batch.bo);
   }

   /* Reset the buffer:
    */
   intel_batchbuffer_reset(intel);
}


/*  This is the only way buffers get added to the validate list.
 */
GLboolean
intel_batchbuffer_emit_reloc(struct intel_context *intel,
                             drm_intel_bo *buffer,
                             uint32_t read_domains, uint32_t write_domain,
			     uint32_t delta)
{
   int ret;

   assert(delta < buffer->size);

   ret = drm_intel_bo_emit_reloc(intel->batch.bo, 4*intel->batch.used,
				 buffer, delta,
				 read_domains, write_domain);
   assert(ret == 0);
   (void)ret;

   /*
    * Using the old buffer offset, write in what the right data would be, in case
    * the buffer doesn't move and we can short-circuit the relocation processing
    * in the kernel
    */
   intel_batchbuffer_emit_dword(intel, buffer->offset + delta);

   return GL_TRUE;
}

GLboolean
intel_batchbuffer_emit_reloc_fenced(struct intel_context *intel,
				    drm_intel_bo *buffer,
				    uint32_t read_domains,
				    uint32_t write_domain,
				    uint32_t delta)
{
   int ret;

   assert(delta < buffer->size);

   ret = drm_intel_bo_emit_reloc_fence(intel->batch.bo, 4*intel->batch.used,
				       buffer, delta,
				       read_domains, write_domain);
   assert(ret == 0);
   (void)ret;

   /*
    * Using the old buffer offset, write in what the right data would
    * be, in case the buffer doesn't move and we can short-circuit the
    * relocation processing in the kernel
    */
   intel_batchbuffer_emit_dword(intel, buffer->offset + delta);

   return GL_TRUE;
}

void
intel_batchbuffer_data(struct intel_context *intel,
                       const void *data, GLuint bytes, bool is_blit)
{
   assert((bytes & 3) == 0);
   intel_batchbuffer_require_space(intel, bytes, is_blit);
   __memcpy(intel->batch.map + intel->batch.used, data, bytes);
   intel->batch.used += bytes >> 2;
}

void
intel_batchbuffer_cached_advance(struct intel_context *intel)
{
   struct cached_batch_item **prev = &intel->batch.cached_items, *item;
   uint32_t sz = (intel->batch.used - intel->batch.emit) * sizeof(uint32_t);
   uint32_t *start = intel->batch.map + intel->batch.emit;
   uint16_t op = *start >> 16;

   while (*prev) {
      uint32_t *old;

      item = *prev;
      old = intel->batch.map + item->header;
      if (op == *old >> 16) {
	 if (item->size == sz && memcmp(old, start, sz) == 0) {
	    if (prev != &intel->batch.cached_items) {
	       *prev = item->next;
	       item->next = intel->batch.cached_items;
	       intel->batch.cached_items = item;
	    }
	    intel->batch.used = intel->batch.emit;
	    return;
	 }

	 goto emit;
      }
      prev = &item->next;
   }

   item = malloc(sizeof(struct cached_batch_item));
   if (item == NULL)
      return;

   item->next = intel->batch.cached_items;
   intel->batch.cached_items = item;

emit:
   item->size = sz;
   item->header = intel->batch.emit;
}

/* Emit a pipelined flush to either flush render and texture cache for
 * reading from a FBO-drawn texture, or flush so that frontbuffer
 * render appears on the screen in DRI1.
 *
 * This is also used for the always_flush_cache driconf debug option.
 */
void
intel_batchbuffer_emit_mi_flush(struct intel_context *intel)
{
   if (intel->gen >= 6) {
      if (intel->batch.is_blit) {
	 BEGIN_BATCH_BLT(4);
	 OUT_BATCH(MI_FLUSH_DW);
	 OUT_BATCH(0);
	 OUT_BATCH(0);
	 OUT_BATCH(0);
	 ADVANCE_BATCH();
      } else {
	 BEGIN_BATCH(8);
	 /* XXX workaround: issue any post sync != 0 before write
	  * cache flush = 1
	  */
	 OUT_BATCH(_3DSTATE_PIPE_CONTROL);
	 OUT_BATCH(PIPE_CONTROL_WRITE_IMMEDIATE);
	 OUT_BATCH(0); /* write address */
	 OUT_BATCH(0); /* write data */

	 OUT_BATCH(_3DSTATE_PIPE_CONTROL);
	 OUT_BATCH(PIPE_CONTROL_INSTRUCTION_FLUSH |
		   PIPE_CONTROL_WRITE_FLUSH |
		   PIPE_CONTROL_DEPTH_CACHE_FLUSH |
		   PIPE_CONTROL_NO_WRITE);
	 OUT_BATCH(0); /* write address */
	 OUT_BATCH(0); /* write data */
	 ADVANCE_BATCH();
      }
   } else if (intel->gen >= 4) {
      BEGIN_BATCH(4);
      OUT_BATCH(_3DSTATE_PIPE_CONTROL |
		PIPE_CONTROL_WRITE_FLUSH |
		PIPE_CONTROL_NO_WRITE);
      OUT_BATCH(0); /* write address */
      OUT_BATCH(0); /* write data */
      OUT_BATCH(0); /* write data */
      ADVANCE_BATCH();
   } else {
      BEGIN_BATCH(1);
      OUT_BATCH(MI_FLUSH);
      ADVANCE_BATCH();
   }
}
