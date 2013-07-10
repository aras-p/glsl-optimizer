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

#include "intel_batchbuffer.h"
#include "intel_buffer_objects.h"
#include "intel_reg.h"
#include "intel_bufmgr.h"
#include "intel_buffers.h"
#include "brw_context.h"

static void
intel_batchbuffer_reset(struct brw_context *brw);

struct cached_batch_item {
   struct cached_batch_item *next;
   uint16_t header;
   uint16_t size;
};

static void
clear_cache(struct brw_context *brw)
{
   struct cached_batch_item *item = brw->batch.cached_items;

   while (item) {
      struct cached_batch_item *next = item->next;
      free(item);
      item = next;
   }

   brw->batch.cached_items = NULL;
}

void
intel_batchbuffer_init(struct brw_context *brw)
{
   intel_batchbuffer_reset(brw);

   if (brw->gen >= 6) {
      /* We can't just use brw_state_batch to get a chunk of space for
       * the gen6 workaround because it involves actually writing to
       * the buffer, and the kernel doesn't let us write to the batch.
       */
      brw->batch.workaround_bo = drm_intel_bo_alloc(brw->bufmgr,
						      "pipe_control workaround",
						      4096, 4096);
   }

   if (!brw->has_llc) {
      brw->batch.cpu_map = malloc(BATCH_SZ);
      brw->batch.map = brw->batch.cpu_map;
   }
}

static void
intel_batchbuffer_reset(struct brw_context *brw)
{
   if (brw->batch.last_bo != NULL) {
      drm_intel_bo_unreference(brw->batch.last_bo);
      brw->batch.last_bo = NULL;
   }
   brw->batch.last_bo = brw->batch.bo;

   clear_cache(brw);

   brw->batch.bo = drm_intel_bo_alloc(brw->bufmgr, "batchbuffer",
					BATCH_SZ, 4096);
   if (brw->has_llc) {
      drm_intel_bo_map(brw->batch.bo, true);
      brw->batch.map = brw->batch.bo->virtual;
   }

   brw->batch.reserved_space = BATCH_RESERVED;
   brw->batch.state_batch_offset = brw->batch.bo->size;
   brw->batch.used = 0;
   brw->batch.needs_sol_reset = false;
}

void
intel_batchbuffer_save_state(struct brw_context *brw)
{
   brw->batch.saved.used = brw->batch.used;
   brw->batch.saved.reloc_count =
      drm_intel_gem_bo_get_reloc_count(brw->batch.bo);
}

void
intel_batchbuffer_reset_to_saved(struct brw_context *brw)
{
   drm_intel_gem_bo_clear_relocs(brw->batch.bo, brw->batch.saved.reloc_count);

   brw->batch.used = brw->batch.saved.used;

   /* Cached batch state is dead, since we just cleared some unknown part of the
    * batchbuffer.  Assume that the caller resets any other state necessary.
    */
   clear_cache(brw);
}

void
intel_batchbuffer_free(struct brw_context *brw)
{
   free(brw->batch.cpu_map);
   drm_intel_bo_unreference(brw->batch.last_bo);
   drm_intel_bo_unreference(brw->batch.bo);
   drm_intel_bo_unreference(brw->batch.workaround_bo);
   clear_cache(brw);
}

static void
do_batch_dump(struct brw_context *brw)
{
   struct drm_intel_decode *decode;
   struct intel_batchbuffer *batch = &brw->batch;
   int ret;

   decode = drm_intel_decode_context_alloc(brw->intelScreen->deviceID);
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

      brw_debug_batch(brw);
   }
}

/* TODO: Push this whole function into bufmgr.
 */
static int
do_flush_locked(struct brw_context *brw)
{
   struct intel_batchbuffer *batch = &brw->batch;
   int ret = 0;

   if (brw->has_llc) {
      drm_intel_bo_unmap(batch->bo);
   } else {
      ret = drm_intel_bo_subdata(batch->bo, 0, 4*batch->used, batch->map);
      if (ret == 0 && batch->state_batch_offset != batch->bo->size) {
	 ret = drm_intel_bo_subdata(batch->bo,
				    batch->state_batch_offset,
				    batch->bo->size - batch->state_batch_offset,
				    (char *)batch->map + batch->state_batch_offset);
      }
   }

   if (!brw->intelScreen->no_hw) {
      int flags;

      if (brw->gen < 6 || !batch->is_blit) {
	 flags = I915_EXEC_RENDER;
      } else {
	 flags = I915_EXEC_BLT;
      }

      if (batch->needs_sol_reset)
	 flags |= I915_EXEC_GEN7_SOL_RESET;

      if (ret == 0) {
         if (unlikely(INTEL_DEBUG & DEBUG_AUB))
            brw_annotate_aub(brw);
	 if (brw->hw_ctx == NULL || batch->is_blit) {
	    ret = drm_intel_bo_mrb_exec(batch->bo, 4 * batch->used, NULL, 0, 0,
					flags);
	 } else {
	    ret = drm_intel_gem_bo_context_exec(batch->bo, brw->hw_ctx,
						4 * batch->used, flags);
	 }
      }
   }

   if (unlikely(INTEL_DEBUG & DEBUG_BATCH))
      do_batch_dump(brw);

   if (ret != 0) {
      fprintf(stderr, "intel_do_flush_locked failed: %s\n", strerror(-ret));
      exit(1);
   }
   brw->vtbl.new_batch(brw);

   return ret;
}

int
_intel_batchbuffer_flush(struct brw_context *brw,
			 const char *file, int line)
{
   int ret;

   if (brw->batch.used == 0)
      return 0;

   if (brw->first_post_swapbuffers_batch == NULL) {
      brw->first_post_swapbuffers_batch = brw->batch.bo;
      drm_intel_bo_reference(brw->first_post_swapbuffers_batch);
   }

   if (unlikely(INTEL_DEBUG & DEBUG_BATCH))
      fprintf(stderr, "%s:%d: Batchbuffer flush with %db used\n", file, line,
	      4*brw->batch.used);

   brw->batch.reserved_space = 0;

   if (brw->vtbl.finish_batch)
      brw->vtbl.finish_batch(brw);

   /* Mark the end of the buffer. */
   intel_batchbuffer_emit_dword(brw, MI_BATCH_BUFFER_END);
   if (brw->batch.used & 1) {
      /* Round batchbuffer usage to 2 DWORDs. */
      intel_batchbuffer_emit_dword(brw, MI_NOOP);
   }

   intel_upload_finish(brw);

   /* Check that we didn't just wrap our batchbuffer at a bad time. */
   assert(!brw->no_batch_wrap);

   ret = do_flush_locked(brw);

   if (unlikely(INTEL_DEBUG & DEBUG_SYNC)) {
      fprintf(stderr, "waiting for idle\n");
      drm_intel_bo_wait_rendering(brw->batch.bo);
   }

   /* Reset the buffer:
    */
   intel_batchbuffer_reset(brw);

   return ret;
}


/*  This is the only way buffers get added to the validate list.
 */
bool
intel_batchbuffer_emit_reloc(struct brw_context *brw,
                             drm_intel_bo *buffer,
                             uint32_t read_domains, uint32_t write_domain,
			     uint32_t delta)
{
   int ret;

   ret = drm_intel_bo_emit_reloc(brw->batch.bo, 4*brw->batch.used,
				 buffer, delta,
				 read_domains, write_domain);
   assert(ret == 0);
   (void)ret;

   /*
    * Using the old buffer offset, write in what the right data would be, in case
    * the buffer doesn't move and we can short-circuit the relocation processing
    * in the kernel
    */
   intel_batchbuffer_emit_dword(brw, buffer->offset + delta);

   return true;
}

bool
intel_batchbuffer_emit_reloc_fenced(struct brw_context *brw,
				    drm_intel_bo *buffer,
				    uint32_t read_domains,
				    uint32_t write_domain,
				    uint32_t delta)
{
   int ret;

   ret = drm_intel_bo_emit_reloc_fence(brw->batch.bo, 4*brw->batch.used,
				       buffer, delta,
				       read_domains, write_domain);
   assert(ret == 0);
   (void)ret;

   /*
    * Using the old buffer offset, write in what the right data would
    * be, in case the buffer doesn't move and we can short-circuit the
    * relocation processing in the kernel
    */
   intel_batchbuffer_emit_dword(brw, buffer->offset + delta);

   return true;
}

void
intel_batchbuffer_data(struct brw_context *brw,
                       const void *data, GLuint bytes, bool is_blit)
{
   assert((bytes & 3) == 0);
   intel_batchbuffer_require_space(brw, bytes, is_blit);
   __memcpy(brw->batch.map + brw->batch.used, data, bytes);
   brw->batch.used += bytes >> 2;
}

void
intel_batchbuffer_cached_advance(struct brw_context *brw)
{
   struct cached_batch_item **prev = &brw->batch.cached_items, *item;
   uint32_t sz = (brw->batch.used - brw->batch.emit) * sizeof(uint32_t);
   uint32_t *start = brw->batch.map + brw->batch.emit;
   uint16_t op = *start >> 16;

   while (*prev) {
      uint32_t *old;

      item = *prev;
      old = brw->batch.map + item->header;
      if (op == *old >> 16) {
	 if (item->size == sz && memcmp(old, start, sz) == 0) {
	    if (prev != &brw->batch.cached_items) {
	       *prev = item->next;
	       item->next = brw->batch.cached_items;
	       brw->batch.cached_items = item;
	    }
	    brw->batch.used = brw->batch.emit;
	    return;
	 }

	 goto emit;
      }
      prev = &item->next;
   }

   item = malloc(sizeof(struct cached_batch_item));
   if (item == NULL)
      return;

   item->next = brw->batch.cached_items;
   brw->batch.cached_items = item;

emit:
   item->size = sz;
   item->header = brw->batch.emit;
}

/**
 * Restriction [DevSNB, DevIVB]:
 *
 * Prior to changing Depth/Stencil Buffer state (i.e. any combination of
 * 3DSTATE_DEPTH_BUFFER, 3DSTATE_CLEAR_PARAMS, 3DSTATE_STENCIL_BUFFER,
 * 3DSTATE_HIER_DEPTH_BUFFER) SW must first issue a pipelined depth stall
 * (PIPE_CONTROL with Depth Stall bit set), followed by a pipelined depth
 * cache flush (PIPE_CONTROL with Depth Flush Bit set), followed by
 * another pipelined depth stall (PIPE_CONTROL with Depth Stall bit set),
 * unless SW can otherwise guarantee that the pipeline from WM onwards is
 * already flushed (e.g., via a preceding MI_FLUSH).
 */
void
intel_emit_depth_stall_flushes(struct brw_context *brw)
{
   assert(brw->gen >= 6 && brw->gen <= 7);

   BEGIN_BATCH(4);
   OUT_BATCH(_3DSTATE_PIPE_CONTROL | (4 - 2));
   OUT_BATCH(PIPE_CONTROL_DEPTH_STALL);
   OUT_BATCH(0); /* address */
   OUT_BATCH(0); /* write data */
   ADVANCE_BATCH()

   BEGIN_BATCH(4);
   OUT_BATCH(_3DSTATE_PIPE_CONTROL | (4 - 2));
   OUT_BATCH(PIPE_CONTROL_DEPTH_CACHE_FLUSH);
   OUT_BATCH(0); /* address */
   OUT_BATCH(0); /* write data */
   ADVANCE_BATCH();

   BEGIN_BATCH(4);
   OUT_BATCH(_3DSTATE_PIPE_CONTROL | (4 - 2));
   OUT_BATCH(PIPE_CONTROL_DEPTH_STALL);
   OUT_BATCH(0); /* address */
   OUT_BATCH(0); /* write data */
   ADVANCE_BATCH();
}

/**
 * From the Ivybridge PRM, Volume 2 Part 1, Section 3.2 (VS Stage Input):
 * "A PIPE_CONTROL with Post-Sync Operation set to 1h and a depth
 *  stall needs to be sent just prior to any 3DSTATE_VS, 3DSTATE_URB_VS,
 *  3DSTATE_CONSTANT_VS, 3DSTATE_BINDING_TABLE_POINTER_VS,
 *  3DSTATE_SAMPLER_STATE_POINTER_VS command.  Only one PIPE_CONTROL needs
 *  to be sent before any combination of VS associated 3DSTATE."
 */
void
gen7_emit_vs_workaround_flush(struct brw_context *brw)
{
   assert(brw->gen == 7);

   BEGIN_BATCH(4);
   OUT_BATCH(_3DSTATE_PIPE_CONTROL | (4 - 2));
   OUT_BATCH(PIPE_CONTROL_DEPTH_STALL | PIPE_CONTROL_WRITE_IMMEDIATE);
   OUT_RELOC(brw->batch.workaround_bo,
	     I915_GEM_DOMAIN_INSTRUCTION, I915_GEM_DOMAIN_INSTRUCTION, 0);
   OUT_BATCH(0); /* write data */
   ADVANCE_BATCH();
}

/**
 * Emits a PIPE_CONTROL with a non-zero post-sync operation, for
 * implementing two workarounds on gen6.  From section 1.4.7.1
 * "PIPE_CONTROL" of the Sandy Bridge PRM volume 2 part 1:
 *
 * [DevSNB-C+{W/A}] Before any depth stall flush (including those
 * produced by non-pipelined state commands), software needs to first
 * send a PIPE_CONTROL with no bits set except Post-Sync Operation !=
 * 0.
 *
 * [Dev-SNB{W/A}]: Before a PIPE_CONTROL with Write Cache Flush Enable
 * =1, a PIPE_CONTROL with any non-zero post-sync-op is required.
 *
 * And the workaround for these two requires this workaround first:
 *
 * [Dev-SNB{W/A}]: Pipe-control with CS-stall bit set must be sent
 * BEFORE the pipe-control with a post-sync op and no write-cache
 * flushes.
 *
 * And this last workaround is tricky because of the requirements on
 * that bit.  From section 1.4.7.2.3 "Stall" of the Sandy Bridge PRM
 * volume 2 part 1:
 *
 *     "1 of the following must also be set:
 *      - Render Target Cache Flush Enable ([12] of DW1)
 *      - Depth Cache Flush Enable ([0] of DW1)
 *      - Stall at Pixel Scoreboard ([1] of DW1)
 *      - Depth Stall ([13] of DW1)
 *      - Post-Sync Operation ([13] of DW1)
 *      - Notify Enable ([8] of DW1)"
 *
 * The cache flushes require the workaround flush that triggered this
 * one, so we can't use it.  Depth stall would trigger the same.
 * Post-sync nonzero is what triggered this second workaround, so we
 * can't use that one either.  Notify enable is IRQs, which aren't
 * really our business.  That leaves only stall at scoreboard.
 */
void
intel_emit_post_sync_nonzero_flush(struct brw_context *brw)
{
   if (!brw->batch.need_workaround_flush)
      return;

   BEGIN_BATCH(4);
   OUT_BATCH(_3DSTATE_PIPE_CONTROL | (4 - 2));
   OUT_BATCH(PIPE_CONTROL_CS_STALL |
	     PIPE_CONTROL_STALL_AT_SCOREBOARD);
   OUT_BATCH(0); /* address */
   OUT_BATCH(0); /* write data */
   ADVANCE_BATCH();

   BEGIN_BATCH(4);
   OUT_BATCH(_3DSTATE_PIPE_CONTROL | (4 - 2));
   OUT_BATCH(PIPE_CONTROL_WRITE_IMMEDIATE);
   OUT_RELOC(brw->batch.workaround_bo,
	     I915_GEM_DOMAIN_INSTRUCTION, I915_GEM_DOMAIN_INSTRUCTION, 0);
   OUT_BATCH(0); /* write data */
   ADVANCE_BATCH();

   brw->batch.need_workaround_flush = false;
}

/* Emit a pipelined flush to either flush render and texture cache for
 * reading from a FBO-drawn texture, or flush so that frontbuffer
 * render appears on the screen in DRI1.
 *
 * This is also used for the always_flush_cache driconf debug option.
 */
void
intel_batchbuffer_emit_mi_flush(struct brw_context *brw)
{
   if (brw->gen >= 6) {
      if (brw->batch.is_blit) {
	 BEGIN_BATCH_BLT(4);
	 OUT_BATCH(MI_FLUSH_DW);
	 OUT_BATCH(0);
	 OUT_BATCH(0);
	 OUT_BATCH(0);
	 ADVANCE_BATCH();
      } else {
	 if (brw->gen == 6) {
	    /* Hardware workaround: SNB B-Spec says:
	     *
	     * [Dev-SNB{W/A}]: Before a PIPE_CONTROL with Write Cache
	     * Flush Enable =1, a PIPE_CONTROL with any non-zero
	     * post-sync-op is required.
	     */
	    intel_emit_post_sync_nonzero_flush(brw);
	 }

	 BEGIN_BATCH(4);
	 OUT_BATCH(_3DSTATE_PIPE_CONTROL | (4 - 2));
	 OUT_BATCH(PIPE_CONTROL_INSTRUCTION_FLUSH |
		   PIPE_CONTROL_WRITE_FLUSH |
		   PIPE_CONTROL_DEPTH_CACHE_FLUSH |
                   PIPE_CONTROL_VF_CACHE_INVALIDATE |
		   PIPE_CONTROL_TC_FLUSH |
		   PIPE_CONTROL_NO_WRITE |
                   PIPE_CONTROL_CS_STALL);
	 OUT_BATCH(0); /* write address */
	 OUT_BATCH(0); /* write data */
	 ADVANCE_BATCH();
      }
   } else {
      BEGIN_BATCH(4);
      OUT_BATCH(_3DSTATE_PIPE_CONTROL | (4 - 2) |
		PIPE_CONTROL_WRITE_FLUSH |
		PIPE_CONTROL_NO_WRITE);
      OUT_BATCH(0); /* write address */
      OUT_BATCH(0); /* write data */
      OUT_BATCH(0); /* write data */
      ADVANCE_BATCH();
   }
}
