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
#include "brw_state.h"
#include "brw_vs.h"
#include "brw_wm.h"

static void
dri_bo_release(dri_bo **bo)
{
   dri_bo_unreference(*bo);
   *bo = NULL;
}


/**
 * called from intelDestroyContext()
 */
static void brw_destroy_context( struct intel_context *intel )
{
   struct brw_context *brw = brw_context(&intel->ctx);
   int i;

   brw_destroy_state(brw);
   brw_draw_destroy( brw );
   brw_clear_validated_bos(brw);
   if (brw->wm.compile_data) {
      free(brw->wm.compile_data->instruction);
      free(brw->wm.compile_data->vreg);
      free(brw->wm.compile_data->refs);
      free(brw->wm.compile_data->prog_instructions);
      free(brw->wm.compile_data);
   }

   for (i = 0; i < brw->state.nr_color_regions; i++)
      intel_region_release(&brw->state.color_regions[i]);
   brw->state.nr_color_regions = 0;
   intel_region_release(&brw->state.depth_region);

   dri_bo_release(&brw->curbe.curbe_bo);
   dri_bo_release(&brw->vs.prog_bo);
   dri_bo_release(&brw->vs.state_bo);
   dri_bo_release(&brw->vs.bind_bo);
   dri_bo_release(&brw->gs.prog_bo);
   dri_bo_release(&brw->gs.state_bo);
   dri_bo_release(&brw->clip.prog_bo);
   dri_bo_release(&brw->clip.state_bo);
   dri_bo_release(&brw->clip.vp_bo);
   dri_bo_release(&brw->sf.prog_bo);
   dri_bo_release(&brw->sf.state_bo);
   dri_bo_release(&brw->sf.vp_bo);
   for (i = 0; i < BRW_MAX_TEX_UNIT; i++)
      dri_bo_release(&brw->wm.sdc_bo[i]);
   dri_bo_release(&brw->wm.bind_bo);
   for (i = 0; i < BRW_WM_MAX_SURF; i++)
      dri_bo_release(&brw->wm.surf_bo[i]);
   dri_bo_release(&brw->wm.sampler_bo);
   dri_bo_release(&brw->wm.prog_bo);
   dri_bo_release(&brw->wm.state_bo);
   dri_bo_release(&brw->cc.prog_bo);
   dri_bo_release(&brw->cc.state_bo);
   dri_bo_release(&brw->cc.vp_bo);
   dri_bo_release(&brw->cc.blend_state_bo);
   dri_bo_release(&brw->cc.depth_stencil_state_bo);
   dri_bo_release(&brw->cc.color_calc_state_bo);
}


/**
 * called from intelDrawBuffer()
 */
static void brw_set_draw_region( struct intel_context *intel, 
                                 struct intel_region *color_regions[],
                                 struct intel_region *depth_region,
                                 GLuint num_color_regions)
{
   struct brw_context *brw = brw_context(&intel->ctx);
   GLuint i;

   /* release old color/depth regions */
   if (brw->state.depth_region != depth_region)
      brw->state.dirty.brw |= BRW_NEW_DEPTH_BUFFER;
   for (i = 0; i < brw->state.nr_color_regions; i++)
       intel_region_release(&brw->state.color_regions[i]);
   intel_region_release(&brw->state.depth_region);

   /* reference new color/depth regions */
   for (i = 0; i < num_color_regions; i++)
       intel_region_reference(&brw->state.color_regions[i], color_regions[i]);
   intel_region_reference(&brw->state.depth_region, depth_region);
   brw->state.nr_color_regions = num_color_regions;
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

   /* Move to the end of the current upload buffer so that we'll force choosing
    * a new buffer next time.
    */
   if (brw->vb.upload.bo != NULL) {
      dri_bo_unreference(brw->vb.upload.bo);
      brw->vb.upload.bo = NULL;
      brw->vb.upload.offset = 0;
   }
}

static void brw_invalidate_state( struct intel_context *intel, GLuint new_state )
{
   /* nothing */
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
}
