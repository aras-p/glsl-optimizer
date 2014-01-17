/**************************************************************************
 * 
 * Copyright 2006 VMware, Inc.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#include "intel_context.h"
#include "intel_batchbuffer.h"
#include "intel_buffer_objects.h"
#include "intel_reg.h"
#include "intel_bufmgr.h"
#include "intel_buffers.h"

static void
intel_batchbuffer_reset(struct intel_context *intel);

void
intel_batchbuffer_init(struct intel_context *intel)
{
   intel_batchbuffer_reset(intel);

   intel->batch.cpu_map = malloc(intel->maxBatchSize);
   intel->batch.map = intel->batch.cpu_map;
}

static void
intel_batchbuffer_reset(struct intel_context *intel)
{
   if (intel->batch.last_bo != NULL) {
      drm_intel_bo_unreference(intel->batch.last_bo);
      intel->batch.last_bo = NULL;
   }
   intel->batch.last_bo = intel->batch.bo;

   intel->batch.bo = drm_intel_bo_alloc(intel->bufmgr, "batchbuffer",
					intel->maxBatchSize, 4096);

   intel->batch.reserved_space = BATCH_RESERVED;
   intel->batch.used = 0;
}

void
intel_batchbuffer_free(struct intel_context *intel)
{
   free(intel->batch.cpu_map);
   drm_intel_bo_unreference(intel->batch.last_bo);
   drm_intel_bo_unreference(intel->batch.bo);
}

static void
do_batch_dump(struct intel_context *intel)
{
   struct drm_intel_decode *decode;
   struct intel_batchbuffer *batch = &intel->batch;
   int ret;

   decode = drm_intel_decode_context_alloc(intel->intelScreen->deviceID);
   if (!decode)
      return;

   ret = drm_intel_bo_map(batch->bo, false);
   if (ret == 0) {
      drm_intel_decode_set_batch_pointer(decode,
					 batch->bo->virtual,
					 batch->bo->offset,
					 batch->used);
   } else {
      fprintf(stderr,
	      "WARNING: failed to map batchbuffer (%s), "
	      "dumping uploaded data instead.\n", strerror(ret));

      drm_intel_decode_set_batch_pointer(decode,
					 batch->map,
					 batch->bo->offset,
					 batch->used);
   }

   drm_intel_decode(decode);

   drm_intel_decode_context_free(decode);

   if (ret == 0) {
      drm_intel_bo_unmap(batch->bo);

      if (intel->vtbl.debug_batch != NULL)
	 intel->vtbl.debug_batch(intel);
   }
}

/* TODO: Push this whole function into bufmgr.
 */
static int
do_flush_locked(struct intel_context *intel)
{
   struct intel_batchbuffer *batch = &intel->batch;
   int ret = 0;

   ret = drm_intel_bo_subdata(batch->bo, 0, 4*batch->used, batch->map);

   if (!intel->intelScreen->no_hw) {
      if (ret == 0) {
         if (unlikely(INTEL_DEBUG & DEBUG_AUB) && intel->vtbl.annotate_aub)
            intel->vtbl.annotate_aub(intel);
         ret = drm_intel_bo_mrb_exec(batch->bo, 4 * batch->used, NULL, 0, 0,
                                     I915_EXEC_RENDER);
      }
   }

   if (unlikely(INTEL_DEBUG & DEBUG_BATCH))
      do_batch_dump(intel);

   if (ret != 0) {
      fprintf(stderr, "intel_do_flush_locked failed: %s\n", strerror(-ret));
      exit(1);
   }
   intel->vtbl.new_batch(intel);

   return ret;
}

int
_intel_batchbuffer_flush(struct intel_context *intel,
			 const char *file, int line)
{
   int ret;

   if (intel->batch.used == 0)
      return 0;

   if (intel->first_post_swapbuffers_batch == NULL) {
      intel->first_post_swapbuffers_batch = intel->batch.bo;
      drm_intel_bo_reference(intel->first_post_swapbuffers_batch);
   }

   if (unlikely(INTEL_DEBUG & DEBUG_BATCH))
      fprintf(stderr, "%s:%d: Batchbuffer flush with %db used\n", file, line,
	      4*intel->batch.used);

   intel->batch.reserved_space = 0;

   if (intel->vtbl.finish_batch)
      intel->vtbl.finish_batch(intel);

   /* Mark the end of the buffer. */
   intel_batchbuffer_emit_dword(intel, MI_BATCH_BUFFER_END);
   if (intel->batch.used & 1) {
      /* Round batchbuffer usage to 2 DWORDs. */
      intel_batchbuffer_emit_dword(intel, MI_NOOP);
   }

   intel_upload_finish(intel);

   /* Check that we didn't just wrap our batchbuffer at a bad time. */
   assert(!intel->no_batch_wrap);

   ret = do_flush_locked(intel);

   if (unlikely(INTEL_DEBUG & DEBUG_SYNC)) {
      fprintf(stderr, "waiting for idle\n");
      drm_intel_bo_wait_rendering(intel->batch.bo);
   }

   /* Reset the buffer:
    */
   intel_batchbuffer_reset(intel);

   return ret;
}


/*  This is the only way buffers get added to the validate list.
 */
bool
intel_batchbuffer_emit_reloc(struct intel_context *intel,
                             drm_intel_bo *buffer,
                             uint32_t read_domains, uint32_t write_domain,
			     uint32_t delta)
{
   int ret;

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

   return true;
}

bool
intel_batchbuffer_emit_reloc_fenced(struct intel_context *intel,
				    drm_intel_bo *buffer,
				    uint32_t read_domains,
				    uint32_t write_domain,
				    uint32_t delta)
{
   int ret;

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

   return true;
}

void
intel_batchbuffer_data(struct intel_context *intel,
                       const void *data, GLuint bytes)
{
   assert((bytes & 3) == 0);
   intel_batchbuffer_require_space(intel, bytes);
   __memcpy(intel->batch.map + intel->batch.used, data, bytes);
   intel->batch.used += bytes >> 2;
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
   BEGIN_BATCH(1);
   OUT_BATCH(MI_FLUSH);
   ADVANCE_BATCH();
}
