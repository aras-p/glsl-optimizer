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

#include "intel_batchbuffer.h" 
#include "intel_regions.h" 

#include "brw_context.h"
#include "brw_defines.h"
#include "brw_state.h"
#include "brw_draw.h"
#include "brw_vs.h"
#include "brw_wm.h"

#include "../glsl/ralloc.h"

static void
dri_bo_release(drm_intel_bo **bo)
{
   drm_intel_bo_unreference(*bo);
   *bo = NULL;
}


/**
 * called from intelDestroyContext()
 */
static void brw_destroy_context( struct intel_context *intel )
{
   struct brw_context *brw = brw_context(&intel->ctx);

   brw_destroy_state(brw);
   brw_draw_destroy( brw );
   brw_clear_validated_bos(brw);
   ralloc_free(brw->wm.compile_data);

   dri_bo_release(&brw->curbe.curbe_bo);
   dri_bo_release(&brw->vs.prog_bo);
   dri_bo_release(&brw->vs.const_bo);
   dri_bo_release(&brw->gs.prog_bo);
   dri_bo_release(&brw->clip.prog_bo);
   dri_bo_release(&brw->sf.prog_bo);
   dri_bo_release(&brw->wm.prog_bo);
   dri_bo_release(&brw->wm.const_bo);
   dri_bo_release(&brw->cc.prog_bo);

   free(brw->curbe.last_buf);
   free(brw->curbe.next_buf);
}


/**
 * called from intelDrawBuffer()
 */
static void brw_set_draw_region( struct intel_context *intel, 
                                 struct intel_region *color_regions[],
                                 struct intel_region *depth_region,
                                 GLuint num_color_regions)
{
}


/**
 * called from intel_batchbuffer_flush and children before sending a
 * batchbuffer off.
 */
static void brw_finish_batch(struct intel_context *intel)
{
   struct brw_context *brw = brw_context(&intel->ctx);
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
static void brw_new_batch( struct intel_context *intel )
{
   struct brw_context *brw = brw_context(&intel->ctx);

   /* Mark all context state as needing to be re-emitted.
    * This is probably not as severe as on 915, since almost all of our state
    * is just in referenced buffers.
    */
   brw->state.dirty.brw |= BRW_NEW_CONTEXT;

   brw->state.dirty.mesa |= ~0;
   brw->state.dirty.brw |= ~0;
   brw->state.dirty.cache |= ~0;

   brw->vb.nr_current_buffers = 0;
}

static void brw_invalidate_state( struct intel_context *intel, GLuint new_state )
{
   /* nothing */
}

/**
 * \see intel_context.vtbl.is_hiz_depth_format
 */
static bool brw_is_hiz_depth_format(struct intel_context *intel,
                                    gl_format format)
{
   /* In the future, this will support Z_FLOAT32. */
   return intel->has_hiz && (format == MESA_FORMAT_X8_Z24);
}


void brwInitVtbl( struct brw_context *brw )
{
   brw->intel.vtbl.check_vertex_size = 0;
   brw->intel.vtbl.emit_state = 0;
   brw->intel.vtbl.reduced_primitive_state = 0;
   brw->intel.vtbl.render_start = 0;
   brw->intel.vtbl.update_texture_state = 0;

   brw->intel.vtbl.invalidate_state = brw_invalidate_state;
   brw->intel.vtbl.new_batch = brw_new_batch;
   brw->intel.vtbl.finish_batch = brw_finish_batch;
   brw->intel.vtbl.destroy = brw_destroy_context;
   brw->intel.vtbl.set_draw_region = brw_set_draw_region;
   brw->intel.vtbl.debug_batch = brw_debug_batch;
   brw->intel.vtbl.render_target_supported = brw_render_target_supported;
   brw->intel.vtbl.is_hiz_depth_format = brw_is_hiz_depth_format;
}
