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

#include "main/glheader.h"
#include "main/mtypes.h"
#include "main/imports.h"
#include "main/macros.h"
#include "main/colormac.h"
#include "main/renderbuffer.h"
#include "main/framebuffer.h"

#include "intel_batchbuffer.h" 
#include "intel_regions.h" 
#include "intel_fbo.h"

#include "brw_context.h"
#include "brw_program.h"
#include "brw_defines.h"
#include "brw_state.h"
#include "brw_draw.h"
#include "brw_vs.h"
#include "brw_wm.h"

#include "gen6_blorp.h"
#include "gen7_blorp.h"

#include "glsl/ralloc.h"

static void
dri_bo_release(drm_intel_bo **bo)
{
   drm_intel_bo_unreference(*bo);
   *bo = NULL;
}


/**
 * called from intelDestroyContext()
 */
static void
brw_destroy_context(struct brw_context *brw)
{
   if (INTEL_DEBUG & DEBUG_SHADER_TIME) {
      /* Force a report. */
      brw->shader_time.report_time = 0;

      brw_collect_and_report_shader_time(brw);
      brw_destroy_shader_time(brw);
   }

   brw_destroy_state(brw);
   brw_draw_destroy( brw );

   dri_bo_release(&brw->curbe.curbe_bo);
   dri_bo_release(&brw->vs.const_bo);
   dri_bo_release(&brw->wm.const_bo);

   free(brw->curbe.last_buf);
   free(brw->curbe.next_buf);

   drm_intel_gem_context_destroy(brw->hw_ctx);
}

/**
 * called from intel_batchbuffer_flush and children before sending a
 * batchbuffer off.
 *
 * Note that ALL state emitted here must fit in the reserved space
 * at the end of a batchbuffer.  If you add more GPU state, increase
 * the BATCH_RESERVED macro.
 */
static void
brw_finish_batch(struct brw_context *brw)
{
   brw_emit_query_end(brw);

   if (brw->curbe.curbe_bo) {
      drm_intel_gem_bo_unmap_gtt(brw->curbe.curbe_bo);
      drm_intel_bo_unreference(brw->curbe.curbe_bo);
      brw->curbe.curbe_bo = NULL;
   }
}


/**
 * called from intelFlushBatchLocked
 */
static void
brw_new_batch(struct brw_context *brw)
{
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

   /* Mark that the current program cache BO has been used by the GPU.
    * It will be reallocated if we need to put new programs in for the
    * next batch.
    */
   brw->cache.bo_used_by_gpu = true;

   /* We need to periodically reap the shader time results, because rollover
    * happens every few seconds.  We also want to see results every once in a
    * while, because many programs won't cleanly destroy our context, so the
    * end-of-run printout may not happen.
    */
   if (INTEL_DEBUG & DEBUG_SHADER_TIME)
      brw_collect_and_report_shader_time(brw);
}

void brwInitVtbl( struct brw_context *brw )
{
   brw->vtbl.new_batch = brw_new_batch;
   brw->vtbl.finish_batch = brw_finish_batch;
   brw->vtbl.destroy = brw_destroy_context;

   assert(brw->gen >= 4);
   if (brw->gen >= 7) {
      gen7_init_vtable_surface_functions(brw);
      brw->vtbl.emit_depth_stencil_hiz = gen7_emit_depth_stencil_hiz;
   } else if (brw->gen >= 4) {
      gen4_init_vtable_surface_functions(brw);
      brw->vtbl.emit_depth_stencil_hiz = brw_emit_depth_stencil_hiz;
   }
}
