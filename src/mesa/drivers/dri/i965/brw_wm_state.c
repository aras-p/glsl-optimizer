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

   unsigned int nr_surfaces, sampler_count;
   GLboolean uses_depth, computes_depth, uses_kill, is_glsl;
   GLboolean polygon_stipple, stats_wm, line_stipple, offset_enable;
   GLboolean color_write_enable;
   GLfloat offset_units, offset_factor;
};

bool
brw_color_buffer_write_enabled(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->intel.ctx;
   const struct gl_fragment_program *fp = brw->fragment_program;
   int i;

   /* _NEW_BUFFERS */
   for (i = 0; i < ctx->DrawBuffer->_NumColorDrawBuffers; i++) {
      struct gl_renderbuffer *rb = ctx->DrawBuffer->_ColorDrawBuffers[i];

      /* _NEW_COLOR */
      if (rb &&
	  (fp->Base.OutputsWritten & BITFIELD64_BIT(FRAG_RESULT_COLOR) ||
	   fp->Base.OutputsWritten & BITFIELD64_BIT(FRAG_RESULT_DATA0 + i)) &&
	  (ctx->Color.ColorMask[i][0] ||
	   ctx->Color.ColorMask[i][1] ||
	   ctx->Color.ColorMask[i][2] ||
	   ctx->Color.ColorMask[i][3])) {
	 return true;
      }
   }

   return false;
}

static void
wm_unit_populate_key(struct brw_context *brw, struct brw_wm_unit_key *key)
{
   struct gl_context *ctx = &brw->intel.ctx;
   const struct gl_fragment_program *fp = brw->fragment_program;
   struct intel_context *intel = &brw->intel;

   memset(key, 0, sizeof(*key));

   /* CACHE_NEW_WM_PROG */
   key->total_grf = brw->wm.prog_data->total_grf;
   key->urb_entry_read_length = brw->wm.prog_data->urb_read_length;
   key->curb_entry_read_length = brw->wm.prog_data->curb_read_length;
   key->dispatch_grf_start_reg = brw->wm.prog_data->first_curbe_grf;
   key->total_scratch = brw->wm.prog_data->total_scratch;

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
      (fp->Base.OutputsWritten & BITFIELD64_BIT(FRAG_RESULT_DEPTH)) != 0;
   /* BRW_NEW_DEPTH_BUFFER
    * Override for NULL depthbuffer case, required by the Pixel Shader Computed
    * Depth field.
    */
   if (brw->state.depth_region == NULL)
      key->computes_depth = 0;

   /* _NEW_BUFFERS | _NEW_COLOR */
   key->color_write_enable = brw_color_buffer_write_enabled(brw);

   /* _NEW_COLOR */
   key->uses_kill = fp->UsesKill || ctx->Color.AlphaEnabled;

   /* If using the fragment shader backend, the program is always
    * 8-wide.
    */
   if (ctx->Shader.CurrentFragmentProgram) {
      struct brw_shader *shader = (struct brw_shader *)
	 ctx->Shader.CurrentFragmentProgram->_LinkedShaders[MESA_SHADER_FRAGMENT];

      if (shader != NULL && shader->ir != NULL) {
	 key->is_glsl = GL_TRUE;
      }
   }

   /* _NEW_DEPTH */
   key->stats_wm = intel->stats_wm;

   /* _NEW_LINE */
   key->line_stipple = ctx->Line.StippleFlag;

   /* _NEW_POLYGON */
   key->offset_enable = ctx->Polygon.OffsetFill;
   key->offset_units = ctx->Polygon.OffsetUnits;
   key->offset_factor = ctx->Polygon.OffsetFactor;
}

/**
 * Setup wm hardware state.  See page 225 of Volume 2
 */
static drm_intel_bo *
wm_unit_create_from_key(struct brw_context *brw, struct brw_wm_unit_key *key,
			drm_intel_bo **reloc_bufs)
{
   struct intel_context *intel = &brw->intel;
   struct brw_wm_unit_state wm;
   drm_intel_bo *bo;

   memset(&wm, 0, sizeof(wm));

   wm.thread0.grf_reg_count = ALIGN(key->total_grf, 16) / 16 - 1;
   wm.thread0.kernel_start_pointer = brw->wm.prog_bo->offset >> 6; /* reloc */
   wm.thread1.depth_coef_urb_read_offset = 1;
   wm.thread1.floating_point_mode = BRW_FLOATING_POINT_NON_IEEE_754;

   if (intel->gen == 5)
      wm.thread1.binding_table_entry_count = 0; /* hardware requirement */
   else
      wm.thread1.binding_table_entry_count = key->nr_surfaces;

   if (key->total_scratch != 0) {
      wm.thread2.scratch_space_base_pointer =
	 brw->wm.scratch_bo->offset >> 10; /* reloc */
      wm.thread2.per_thread_scratch_space = ffs(key->total_scratch) - 11;
   } else {
      wm.thread2.scratch_space_base_pointer = 0;
      wm.thread2.per_thread_scratch_space = 0;
   }

   wm.thread3.dispatch_grf_start_reg = key->dispatch_grf_start_reg;
   wm.thread3.urb_entry_read_length = key->urb_entry_read_length;
   wm.thread3.urb_entry_read_offset = 0;
   wm.thread3.const_urb_entry_read_length = key->curb_entry_read_length;
   wm.thread3.const_urb_entry_read_offset = key->curbe_offset * 2;

   if (intel->gen == 5)
      wm.wm4.sampler_count = 0; /* hardware requirement */
   else
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

   wm.wm5.max_threads = brw->wm_max_threads - 1;

   if (key->color_write_enable ||
       key->uses_kill ||
       key->computes_depth) {
      wm.wm5.thread_dispatch_enable = 1;
   }

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

   if (unlikely(INTEL_DEBUG & DEBUG_STATS) || key->stats_wm)
      wm.wm4.stats_enable = 1;

   bo = brw_upload_cache(&brw->cache, BRW_WM_UNIT,
			 key, sizeof(*key),
			 reloc_bufs, 3,
			 &wm, sizeof(wm));

   /* Emit WM program relocation */
   drm_intel_bo_emit_reloc(bo, offsetof(struct brw_wm_unit_state, thread0),
			   brw->wm.prog_bo, wm.thread0.grf_reg_count << 1,
			   I915_GEM_DOMAIN_INSTRUCTION, 0);

   /* Emit scratch space relocation */
   if (key->total_scratch != 0) {
      drm_intel_bo_emit_reloc(bo, offsetof(struct brw_wm_unit_state, thread2),
			      brw->wm.scratch_bo,
			      wm.thread2.per_thread_scratch_space,
			      I915_GEM_DOMAIN_RENDER, I915_GEM_DOMAIN_RENDER);
   }

   /* Emit sampler state relocation */
   if (key->sampler_count != 0) {
      drm_intel_bo_emit_reloc(bo, offsetof(struct brw_wm_unit_state, wm4),
			      brw->wm.sampler_bo, (wm.wm4.stats_enable |
						   (wm.wm4.sampler_count << 2)),
			      I915_GEM_DOMAIN_INSTRUCTION, 0);
   }

   return bo;
}


static void upload_wm_unit( struct brw_context *brw )
{
   struct brw_wm_unit_key key;
   drm_intel_bo *reloc_bufs[3];
   wm_unit_populate_key(brw, &key);

   reloc_bufs[0] = brw->wm.prog_bo;
   reloc_bufs[1] = brw->wm.scratch_bo;
   reloc_bufs[2] = brw->wm.sampler_bo;

   drm_intel_bo_unreference(brw->wm.state_bo);
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
	       _NEW_COLOR |
	       _NEW_DEPTH |
	       _NEW_BUFFERS),

      .brw = (BRW_NEW_FRAGMENT_PROGRAM | 
	      BRW_NEW_CURBE_OFFSETS |
	      BRW_NEW_DEPTH_BUFFER |
	      BRW_NEW_NR_WM_SURFACES),

      .cache = (CACHE_NEW_WM_PROG |
		CACHE_NEW_SAMPLER)
   },
   .prepare = upload_wm_unit,
};

