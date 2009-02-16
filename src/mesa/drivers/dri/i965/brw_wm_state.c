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
                   


#include "brw_context.h"
#include "brw_state.h"
#include "brw_defines.h"
#include "brw_wm.h"

/***********************************************************************
 * WM unit - fragment programs and rasterization
 */

struct brw_wm_unit_key {
   unsigned int total_grf, total_scratch;
   unsigned int urb_entry_read_length;
   unsigned int curb_entry_read_length;
   unsigned int dispatch_grf_start_reg;

   unsigned int curbe_offset;
   unsigned int urb_size;

   unsigned int max_threads;

   unsigned int nr_surfaces, sampler_count;
   GLboolean uses_depth, computes_depth, uses_kill, is_glsl;
   GLboolean polygon_stipple, stats_wm, line_stipple, offset_enable;
   GLfloat offset_units, offset_factor;
};

static void
wm_unit_populate_key(struct brw_context *brw, struct brw_wm_unit_key *key)
{
   GLcontext *ctx = &brw->intel.ctx;
   const struct gl_fragment_program *fp = brw->fragment_program;
   struct intel_context *intel = &brw->intel;

   memset(key, 0, sizeof(*key));

   if (INTEL_DEBUG & DEBUG_SINGLE_THREAD)
      key->max_threads = 1;
   else {
      /* WM maximum threads is number of EUs times number of threads per EU. */
      if (BRW_IS_G4X(brw))
	 key->max_threads = 10 * 5;
      else
	 key->max_threads = 8 * 4;
   }

   /* CACHE_NEW_WM_PROG */
   key->total_grf = brw->wm.prog_data->total_grf;
   key->urb_entry_read_length = brw->wm.prog_data->urb_read_length;
   key->curb_entry_read_length = brw->wm.prog_data->curb_read_length;
   key->dispatch_grf_start_reg = brw->wm.prog_data->first_curbe_grf;
   key->total_scratch = ALIGN(brw->wm.prog_data->total_scratch, 1024);

   /* BRW_NEW_URB_FENCE */
   key->urb_size = brw->urb.vsize;

   /* BRW_NEW_CURBE_OFFSETS */
   key->curbe_offset = brw->curbe.wm_start;

   /* BRW_NEW_NR_SURFACEs */
   key->nr_surfaces = brw->wm.nr_surfaces;

   /* CACHE_NEW_SAMPLER */
   key->sampler_count = brw->wm.sampler_count;

   /* _NEW_POLYGONSTIPPLE */
   key->polygon_stipple = ctx->Polygon.StippleFlag;

   /* BRW_NEW_FRAGMENT_PROGRAM */
   key->uses_depth = (fp->Base.InputsRead & (1 << FRAG_ATTRIB_WPOS)) != 0;

   /* as far as we can tell */
   key->computes_depth =
      (fp->Base.OutputsWritten & (1 << FRAG_RESULT_DEPR)) != 0;

   /* _NEW_COLOR */
   key->uses_kill = fp->UsesKill || ctx->Color.AlphaEnabled;
   key->is_glsl = brw_wm_is_glsl(fp);

   /* XXX: This needs a flag to indicate when it changes. */
   key->stats_wm = intel->stats_wm;

   /* _NEW_LINE */
   key->line_stipple = ctx->Line.StippleFlag;

   /* _NEW_POLYGON */
   key->offset_enable = ctx->Polygon.OffsetFill;
   key->offset_units = ctx->Polygon.OffsetUnits;
   key->offset_factor = ctx->Polygon.OffsetFactor;
}

static dri_bo *
wm_unit_create_from_key(struct brw_context *brw, struct brw_wm_unit_key *key,
			dri_bo **reloc_bufs)
{
   struct brw_wm_unit_state wm;
   dri_bo *bo;

   memset(&wm, 0, sizeof(wm));

   wm.thread0.grf_reg_count = ALIGN(key->total_grf, 16) / 16 - 1;
   wm.thread0.kernel_start_pointer = brw->wm.prog_bo->offset >> 6; /* reloc */
   wm.thread1.depth_coef_urb_read_offset = 1;
   wm.thread1.floating_point_mode = BRW_FLOATING_POINT_NON_IEEE_754;
   wm.thread1.binding_table_entry_count = key->nr_surfaces;

   if (key->total_scratch != 0) {
      wm.thread2.scratch_space_base_pointer =
	 brw->wm.scratch_buffer->offset >> 10; /* reloc */
      wm.thread2.per_thread_scratch_space = key->total_scratch / 1024 - 1;
   } else {
      wm.thread2.scratch_space_base_pointer = 0;
      wm.thread2.per_thread_scratch_space = 0;
   }

   wm.thread3.dispatch_grf_start_reg = key->dispatch_grf_start_reg;
   wm.thread3.urb_entry_read_length = key->urb_entry_read_length;
   wm.thread3.const_urb_entry_read_length = key->curb_entry_read_length;
   wm.thread3.const_urb_entry_read_offset = key->curbe_offset * 2;
   wm.thread3.urb_entry_read_offset = 0;

   wm.wm4.sampler_count = (key->sampler_count + 1) / 4;
   if (brw->wm.sampler_bo != NULL) {
      /* reloc */
      wm.wm4.sampler_state_pointer = brw->wm.sampler_bo->offset >> 5;
   } else {
      wm.wm4.sampler_state_pointer = 0;
   }

   wm.wm5.program_uses_depth = key->uses_depth;
   wm.wm5.program_computes_depth = key->computes_depth;
   wm.wm5.program_uses_killpixel = key->uses_kill;

   if (key->is_glsl)
      wm.wm5.enable_8_pix = 1;
   else
      wm.wm5.enable_16_pix = 1;

   wm.wm5.max_threads = key->max_threads - 1;
   wm.wm5.thread_dispatch_enable = 1;	/* AKA: color_write */
   wm.wm5.legacy_line_rast = 0;
   wm.wm5.legacy_global_depth_bias = 0;
   wm.wm5.early_depth_test = 1;	        /* never need to disable */
   wm.wm5.line_aa_region_width = 0;
   wm.wm5.line_endcap_aa_region_width = 1;

   wm.wm5.polygon_stipple = key->polygon_stipple;

   if (key->offset_enable) {
      wm.wm5.depth_offset = 1;
      /* Something wierd going on with legacy_global_depth_bias,
       * offset_constant, scaling and MRD.  This value passes glean
       * but gives some odd results elsewere (eg. the
       * quad-offset-units test).
       */
      wm.global_depth_offset_constant = key->offset_units * 2;

      /* This is the only value that passes glean:
       */
      wm.global_depth_offset_scale = key->offset_factor;
   }

   wm.wm5.line_stipple = key->line_stipple;

   if (INTEL_DEBUG & DEBUG_STATS || key->stats_wm)
      wm.wm4.stats_enable = 1;

   bo = brw_upload_cache(&brw->cache, BRW_WM_UNIT,
			 key, sizeof(*key),
			 reloc_bufs, 3,
			 &wm, sizeof(wm),
			 NULL, NULL);

   /* Emit WM program relocation */
   dri_bo_emit_reloc(bo,
		     I915_GEM_DOMAIN_INSTRUCTION, 0,
		     wm.thread0.grf_reg_count << 1,
		     offsetof(struct brw_wm_unit_state, thread0),
		     brw->wm.prog_bo);

   /* Emit scratch space relocation */
   if (key->total_scratch != 0) {
      dri_bo_emit_reloc(bo,
			0, 0,
			wm.thread2.per_thread_scratch_space,
			offsetof(struct brw_wm_unit_state, thread2),
			brw->wm.scratch_buffer);
   }

   /* Emit sampler state relocation */
   if (key->sampler_count != 0) {
      dri_bo_emit_reloc(bo,
			I915_GEM_DOMAIN_INSTRUCTION, 0,
			wm.wm4.stats_enable | (wm.wm4.sampler_count << 2),
			offsetof(struct brw_wm_unit_state, wm4),
			brw->wm.sampler_bo);
   }

   return bo;
}


static void upload_wm_unit( struct brw_context *brw )
{
   struct intel_context *intel = &brw->intel;
   struct brw_wm_unit_key key;
   dri_bo *reloc_bufs[3];
   wm_unit_populate_key(brw, &key);

   /* Allocate the necessary scratch space if we haven't already.  Don't
    * bother reducing the allocation later, since we use scratch so
    * rarely.
    */
   assert(key.total_scratch <= 12 * 1024);
   if (key.total_scratch) {
      GLuint total = key.total_scratch * key.max_threads;

      if (brw->wm.scratch_buffer && total > brw->wm.scratch_buffer->size) {
	 dri_bo_unreference(brw->wm.scratch_buffer);
	 brw->wm.scratch_buffer = NULL;
      }
      if (brw->wm.scratch_buffer == NULL) {
	 brw->wm.scratch_buffer = dri_bo_alloc(intel->bufmgr,
					       "wm scratch",
					       total,
					       4096);
      }
   }

   reloc_bufs[0] = brw->wm.prog_bo;
   reloc_bufs[1] = brw->wm.scratch_buffer;
   reloc_bufs[2] = brw->wm.sampler_bo;

   dri_bo_unreference(brw->wm.state_bo);
   brw->wm.state_bo = brw_search_cache(&brw->cache, BRW_WM_UNIT,
				       &key, sizeof(key),
				       reloc_bufs, 3,
				       NULL);
   if (brw->wm.state_bo == NULL) {
      brw->wm.state_bo = wm_unit_create_from_key(brw, &key, reloc_bufs);
   }
}

const struct brw_tracked_state brw_wm_unit = {
   .dirty = {
      .mesa = (_NEW_POLYGON | 
	       _NEW_POLYGONSTIPPLE | 
	       _NEW_LINE | 
	       _NEW_COLOR),

      .brw = (BRW_NEW_FRAGMENT_PROGRAM | 
	      BRW_NEW_CURBE_OFFSETS |
	      BRW_NEW_NR_SURFACES),

      .cache = (CACHE_NEW_WM_PROG |
		CACHE_NEW_SAMPLER)
   },
   .prepare = upload_wm_unit,
};

