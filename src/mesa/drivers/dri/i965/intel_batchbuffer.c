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

#include "intel_batchbuffer.h"
#include "intel_buffer_objects.h"
#include "intel_reg.h"
#include "intel_bufmgr.h"
#include "intel_buffers.h"
#include "intel_fbo.h"
#include "brw_context.h"

static void
intel_batchbuffer_reset(struct brw_context *brw);

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

   brw->batch.need_workaround_flush = true;

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

   brw_render_cache_set_clear(brw);

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

   /* We don't know what ring the new batch will be sent to until we see the
    * first BEGIN_BATCH or BEGIN_BATCH_BLT.  Mark it as unknown.
    */
   brw->batch.ring = UNKNOWN_RING;
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
   if (brw->batch.used == 0)
      brw->batch.ring = UNKNOWN_RING;
}

void
intel_batchbuffer_free(struct brw_context *brw)
{
   free(brw->batch.cpu_map);
   drm_intel_bo_unreference(brw->batch.last_bo);
   drm_intel_bo_unreference(brw->batch.bo);
   drm_intel_bo_unreference(brw->batch.workaround_bo);
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
					 batch->bo->offset64,
					 batch->used);
   } else {
      fprintf(stderr,
	      "WARNING: failed to map batchbuffer (%s), "
	      "dumping uploaded data instead.\n", strerror(ret));

      drm_intel_decode_set_batch_pointer(decode,
					 batch->map,
					 batch->bo->offset64,
					 batch->used);
   }

   drm_intel_decode_set_output_file(decode, stderr);
   drm_intel_decode(decode);

   drm_intel_decode_context_free(decode);

   if (ret == 0) {
      drm_intel_bo_unmap(batch->bo);

      brw_debug_batch(brw);
   }
}

void
intel_batchbuffer_emit_render_ring_prelude(struct brw_context *brw)
{
   /* We may need to enable and snapshot OA counters. */
   brw_perf_monitor_new_batch(brw);
}

/**
 * Called when starting a new batch buffer.
 */
static void
brw_new_batch(struct brw_context *brw)
{
   /* Create a new batchbuffer and reset the associated state: */
   intel_batchbuffer_reset(brw);

   /* If the kernel supports hardware contexts, then most hardware state is
    * preserved between batches; we only need to re-emit state that is required
    * to be in every batch.  Otherwise we need to re-emit all the state that
    * would otherwise be stored in the context (which for all intents and
    * purposes means everything).
    */
   if (brw->hw_ctx == NULL)
      brw->state.dirty.brw |= BRW_NEW_CONTEXT;

   brw->state.dirty.brw |= BRW_NEW_BATCH;

   /* Assume that the last command before the start of our batch was a
    * primitive, for safety.
    */
   brw->batch.need_workaround_flush = true;

   brw->state_batch_count = 0;

   brw->ib.type = -1;

   /* We need to periodically reap the shader time results, because rollover
    * happens every few seconds.  We also want to see results every once in a
    * while, because many programs won't cleanly destroy our context, so the
    * end-of-run printout may not happen.
    */
   if (INTEL_DEBUG & DEBUG_SHADER_TIME)
      brw_collect_and_report_shader_time(brw);

   if (INTEL_DEBUG & DEBUG_PERFMON)
      brw_dump_perf_monitors(brw);
}

/**
 * Called from intel_batchbuffer_flush before emitting MI_BATCHBUFFER_END and
 * sending it off.
 *
 * This function can emit state (say, to preserve registers that aren't saved
 * between batches).  All of this state MUST fit in the reserved space at the
 * end of the batchbuffer.  If you add more GPU state, increase the reserved
 * space by updating the BATCH_RESERVED macro.
 */
static void
brw_finish_batch(struct brw_context *brw)
{
   /* Capture the closing pipeline statistics register values necessary to
    * support query objects (in the non-hardware context world).
    */
   brw_emit_query_end(brw);

   /* We may also need to snapshot and disable OA counters. */
   if (brw->batch.ring == RENDER_RING)
      brw_perf_monitor_finish_batch(brw);

   if (brw->curbe.curbe_bo) {
      drm_intel_gem_bo_unmap_gtt(brw->curbe.curbe_bo);
      drm_intel_bo_unreference(brw->curbe.curbe_bo);
      brw->curbe.curbe_bo = NULL;
   }

   /* Mark that the current program cache BO has been used by the GPU.
    * It will be reallocated if we need to put new programs in for the
    * next batch.
    */
   brw->cache.bo_used_by_gpu = true;
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

      if (brw->gen >= 6 && batch->ring == BLT_RING) {
         flags = I915_EXEC_BLT;
      } else {
         flags = I915_EXEC_RENDER;
      }
      if (batch->needs_sol_reset)
	 flags |= I915_EXEC_GEN7_SOL_RESET;

      if (ret == 0) {
         if (unlikely(INTEL_DEBUG & DEBUG_AUB))
            brw_annotate_aub(brw);
	 if (brw->hw_ctx == NULL || batch->ring != RENDER_RING) {
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

   if (unlikely(INTEL_DEBUG & DEBUG_BATCH)) {
      int bytes_for_commands = 4 * brw->batch.used;
      int bytes_for_state = brw->batch.bo->size - brw->batch.state_batch_offset;
      int total_bytes = bytes_for_commands + bytes_for_state;
      fprintf(stderr, "%s:%d: Batchbuffer flush with %4db (pkt) + "
              "%4db (state) = %4db (%0.1f%%)\n", file, line,
              bytes_for_commands, bytes_for_state,
              total_bytes,
              100.0f * total_bytes / BATCH_SZ);
   }

   brw->batch.reserved_space = 0;

   brw_finish_batch(brw);

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

   /* Start a new batch buffer. */
   brw_new_batch(brw);

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
   intel_batchbuffer_emit_dword(brw, buffer->offset64 + delta);

   return true;
}

bool
intel_batchbuffer_emit_reloc64(struct brw_context *brw,
                               drm_intel_bo *buffer,
                               uint32_t read_domains, uint32_t write_domain,
			       uint32_t delta)
{
   int ret = drm_intel_bo_emit_reloc(brw->batch.bo, 4*brw->batch.used,
                                     buffer, delta,
                                     read_domains, write_domain);
   assert(ret == 0);
   (void) ret;

   /* Using the old buffer offset, write in what the right data would be, in
    * case the buffer doesn't move and we can short-circuit the relocation
    * processing in the kernel
    */
   uint64_t offset = buffer->offset64 + delta;
   intel_batchbuffer_emit_dword(brw, offset);
   intel_batchbuffer_emit_dword(brw, offset >> 32);

   return true;
}


void
intel_batchbuffer_data(struct brw_context *brw,
                       const void *data, GLuint bytes, enum brw_gpu_ring ring)
{
   assert((bytes & 3) == 0);
   intel_batchbuffer_require_space(brw, bytes, ring);
   __memcpy(brw->batch.map + brw->batch.used, data, bytes);
   brw->batch.used += bytes >> 2;
}

/**
 * According to the latest documentation, any PIPE_CONTROL with the
 * "Command Streamer Stall" bit set must also have another bit set,
 * with five different options:
 *
 *  - Render Target Cache Flush
 *  - Depth Cache Flush
 *  - Stall at Pixel Scoreboard
 *  - Post-Sync Operation
 *  - Depth Stall
 *
 * I chose "Stall at Pixel Scoreboard" since we've used it effectively
 * in the past, but the choice is fairly arbitrary.
 */
static void
gen8_add_cs_stall_workaround_bits(uint32_t *flags)
{
   uint32_t wa_bits = PIPE_CONTROL_WRITE_FLUSH |
                      PIPE_CONTROL_DEPTH_CACHE_FLUSH |
                      PIPE_CONTROL_WRITE_IMMEDIATE |
                      PIPE_CONTROL_WRITE_DEPTH_COUNT |
                      PIPE_CONTROL_WRITE_TIMESTAMP |
                      PIPE_CONTROL_STALL_AT_SCOREBOARD |
                      PIPE_CONTROL_DEPTH_STALL;

   /* If we're doing a CS stall, and don't already have one of the
    * workaround bits set, add "Stall at Pixel Scoreboard."
    */
   if ((*flags & PIPE_CONTROL_CS_STALL) != 0 && (*flags & wa_bits) == 0)
      *flags |= PIPE_CONTROL_STALL_AT_SCOREBOARD;
}

/**
 * Emit a PIPE_CONTROL with various flushing flags.
 *
 * The caller is responsible for deciding what flags are appropriate for the
 * given generation.
 */
void
brw_emit_pipe_control_flush(struct brw_context *brw, uint32_t flags)
{
   if (brw->gen >= 8) {
      gen8_add_cs_stall_workaround_bits(&flags);

      BEGIN_BATCH(6);
      OUT_BATCH(_3DSTATE_PIPE_CONTROL | (6 - 2));
      OUT_BATCH(flags);
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);
      ADVANCE_BATCH();
   } else if (brw->gen >= 6) {
      BEGIN_BATCH(5);
      OUT_BATCH(_3DSTATE_PIPE_CONTROL | (5 - 2));
      OUT_BATCH(flags);
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);
      ADVANCE_BATCH();
   } else {
      BEGIN_BATCH(4);
      OUT_BATCH(_3DSTATE_PIPE_CONTROL | flags | (4 - 2));
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);
      ADVANCE_BATCH();
   }
}

/**
 * Emit a PIPE_CONTROL that writes to a buffer object.
 *
 * \p flags should contain one of the following items:
 *  - PIPE_CONTROL_WRITE_IMMEDIATE
 *  - PIPE_CONTROL_WRITE_TIMESTAMP
 *  - PIPE_CONTROL_WRITE_DEPTH_COUNT
 */
void
brw_emit_pipe_control_write(struct brw_context *brw, uint32_t flags,
                            drm_intel_bo *bo, uint32_t offset,
                            uint32_t imm_lower, uint32_t imm_upper)
{
   if (brw->gen >= 8) {
      gen8_add_cs_stall_workaround_bits(&flags);

      BEGIN_BATCH(6);
      OUT_BATCH(_3DSTATE_PIPE_CONTROL | (6 - 2));
      OUT_BATCH(flags);
      OUT_RELOC64(bo, I915_GEM_DOMAIN_INSTRUCTION, I915_GEM_DOMAIN_INSTRUCTION,
                  offset);
      OUT_BATCH(imm_lower);
      OUT_BATCH(imm_upper);
      ADVANCE_BATCH();
   } else if (brw->gen >= 6) {
      /* PPGTT/GGTT is selected by DW2 bit 2 on Sandybridge, but DW1 bit 24
       * on later platforms.  We always use PPGTT on Gen7+.
       */
      unsigned gen6_gtt = brw->gen == 6 ? PIPE_CONTROL_GLOBAL_GTT_WRITE : 0;

      BEGIN_BATCH(5);
      OUT_BATCH(_3DSTATE_PIPE_CONTROL | (5 - 2));
      OUT_BATCH(flags);
      OUT_RELOC(bo, I915_GEM_DOMAIN_INSTRUCTION, I915_GEM_DOMAIN_INSTRUCTION,
                gen6_gtt | offset);
      OUT_BATCH(imm_lower);
      OUT_BATCH(imm_upper);
      ADVANCE_BATCH();
   } else {
      BEGIN_BATCH(4);
      OUT_BATCH(_3DSTATE_PIPE_CONTROL | flags | (4 - 2));
      OUT_RELOC(bo, I915_GEM_DOMAIN_INSTRUCTION, I915_GEM_DOMAIN_INSTRUCTION,
                PIPE_CONTROL_GLOBAL_GTT_WRITE | offset);
      OUT_BATCH(imm_lower);
      OUT_BATCH(imm_upper);
      ADVANCE_BATCH();
   }
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
   assert(brw->gen >= 6 && brw->gen <= 8);

   brw_emit_pipe_control_flush(brw, PIPE_CONTROL_DEPTH_STALL);
   brw_emit_pipe_control_flush(brw, PIPE_CONTROL_DEPTH_CACHE_FLUSH);
   brw_emit_pipe_control_flush(brw, PIPE_CONTROL_DEPTH_STALL);
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
   brw_emit_pipe_control_write(brw,
                               PIPE_CONTROL_WRITE_IMMEDIATE
                               | PIPE_CONTROL_DEPTH_STALL,
                               brw->batch.workaround_bo, 0,
                               0, 0);
}


/**
 * Emit a PIPE_CONTROL command for gen7 with the CS Stall bit set.
 */
void
gen7_emit_cs_stall_flush(struct brw_context *brw)
{
   brw_emit_pipe_control_write(brw,
                               PIPE_CONTROL_CS_STALL
                               | PIPE_CONTROL_WRITE_IMMEDIATE,
                               brw->batch.workaround_bo, 0,
                               0, 0);
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

   brw_emit_pipe_control_flush(brw,
                               PIPE_CONTROL_CS_STALL |
                               PIPE_CONTROL_STALL_AT_SCOREBOARD);

   brw_emit_pipe_control_write(brw, PIPE_CONTROL_WRITE_IMMEDIATE,
                               brw->batch.workaround_bo, 0, 0, 0);

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
   if (brw->batch.ring == BLT_RING && brw->gen >= 6) {
      BEGIN_BATCH_BLT(4);
      OUT_BATCH(MI_FLUSH_DW);
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);
      ADVANCE_BATCH();
   } else {
      int flags = PIPE_CONTROL_NO_WRITE | PIPE_CONTROL_WRITE_FLUSH;
      if (brw->gen >= 6) {
         flags |= PIPE_CONTROL_INSTRUCTION_FLUSH |
                  PIPE_CONTROL_DEPTH_CACHE_FLUSH |
                  PIPE_CONTROL_VF_CACHE_INVALIDATE |
                  PIPE_CONTROL_TC_FLUSH |
                  PIPE_CONTROL_CS_STALL;

         if (brw->gen == 6) {
            /* Hardware workaround: SNB B-Spec says:
             *
             * [Dev-SNB{W/A}]: Before a PIPE_CONTROL with Write Cache
             * Flush Enable =1, a PIPE_CONTROL with any non-zero
             * post-sync-op is required.
             */
            intel_emit_post_sync_nonzero_flush(brw);
         }
      }
      brw_emit_pipe_control_flush(brw, flags);
   }

   brw_render_cache_set_clear(brw);
}

void
brw_load_register_mem(struct brw_context *brw,
                      uint32_t reg,
                      drm_intel_bo *bo,
                      uint32_t read_domains, uint32_t write_domain,
                      uint32_t offset)
{
   /* MI_LOAD_REGISTER_MEM only exists on Gen7+. */
   assert(brw->gen >= 7);

   if (brw->gen >= 8) {
      BEGIN_BATCH(4);
      OUT_BATCH(GEN7_MI_LOAD_REGISTER_MEM | (4 - 2));
      OUT_BATCH(reg);
      OUT_RELOC64(bo, read_domains, write_domain, offset);
      ADVANCE_BATCH();
   } else {
      BEGIN_BATCH(3);
      OUT_BATCH(GEN7_MI_LOAD_REGISTER_MEM | (3 - 2));
      OUT_BATCH(reg);
      OUT_RELOC(bo, read_domains, write_domain, offset);
      ADVANCE_BATCH();
   }
}
