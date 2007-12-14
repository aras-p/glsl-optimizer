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
#include "dri_bufmgr.h"
#include "brw_wm.h"

/***********************************************************************
 * WM unit - fragment programs and rasterization
 */

static void upload_wm_unit(struct brw_context *brw )
{
   struct intel_context *intel = &brw->intel;
   struct brw_wm_unit_state wm;
   GLuint max_threads;
   GLuint per_thread;
   dri_bo *reloc_bufs[3];

   if (INTEL_DEBUG & DEBUG_SINGLE_THREAD)
      max_threads = 0; 
   else
      max_threads = 31;


   memset(&wm, 0, sizeof(wm));

   /* CACHE_NEW_WM_PROG */
   wm.thread0.grf_reg_count = ALIGN(brw->wm.prog_data->total_grf, 16) / 16 - 1;
   wm.thread0.kernel_start_pointer = brw->wm.prog_bo->offset >> 6; /* reloc */
   wm.thread3.dispatch_grf_start_reg = brw->wm.prog_data->first_curbe_grf;
   wm.thread3.urb_entry_read_length = brw->wm.prog_data->urb_read_length;
   wm.thread3.const_urb_entry_read_length = brw->wm.prog_data->curb_read_length;

   wm.wm5.max_threads = max_threads;      

   per_thread = ALIGN(brw->wm.prog_data->total_scratch, 1024);
   assert(per_thread <= 12 * 1024);

   if (brw->wm.prog_data->total_scratch) {
      GLuint total = per_thread * (max_threads + 1);

      /* Scratch space -- just have to make sure there is sufficient
       * allocated for the active program and current number of threads.
       */
      brw->wm.scratch_buffer_size = total;
      if (brw->wm.scratch_buffer &&
	  brw->wm.scratch_buffer_size > brw->wm.scratch_buffer->size) {
	 dri_bo_unreference(brw->wm.scratch_buffer);
	 brw->wm.scratch_buffer = NULL;
      }
      if (!brw->wm.scratch_buffer) {
	 brw->wm.scratch_buffer = dri_bo_alloc(intel->bufmgr,
					       "wm scratch",
					       brw->wm.scratch_buffer_size,
					       4096, DRM_BO_FLAG_MEM_TT);
      }
   }

   /* CACHE_NEW_SURFACE */
   wm.thread1.binding_table_entry_count = brw->wm.nr_surfaces;

   /* CACHE_NEW_WM_PROG */
   if (per_thread != 0) {
   /* reloc */
      wm.thread2.scratch_space_base_pointer =
	 brw->wm.scratch_buffer->offset >> 10;
      wm.thread2.per_thread_scratch_space = per_thread / 1024 - 1;
   } else {
      wm.thread2.scratch_space_base_pointer = 0;
      wm.thread2.per_thread_scratch_space = 0;
   }

   /* BRW_NEW_CURBE_OFFSETS */
   wm.thread3.const_urb_entry_read_offset = brw->curbe.wm_start * 2;

   wm.thread3.urb_entry_read_offset = 0;
   wm.thread1.depth_coef_urb_read_offset = 1;
   wm.thread1.floating_point_mode = BRW_FLOATING_POINT_NON_IEEE_754;

   /* CACHE_NEW_SAMPLER */
   wm.wm4.sampler_count = (brw->wm.sampler_count + 1) / 4;
   if (brw->wm.sampler_bo != NULL) {
      /* reloc */
      wm.wm4.sampler_state_pointer = brw->wm.sampler_bo->offset >> 5;
   } else {
      wm.wm4.sampler_state_pointer = 0;
   }

   /* BRW_NEW_FRAGMENT_PROGRAM */
   {
      const struct gl_fragment_program *fp = brw->fragment_program; 

      if (fp->Base.InputsRead & (1<<FRAG_ATTRIB_WPOS)) 
	 wm.wm5.program_uses_depth = 1; /* as far as we can tell */
   
      if (fp->Base.OutputsWritten & (1<<FRAG_RESULT_DEPR)) 
	 wm.wm5.program_computes_depth = 1;
   
      /* _NEW_COLOR */
      if (fp->UsesKill || 
	  brw->attribs.Color->AlphaEnabled) 
	 wm.wm5.program_uses_killpixel = 1; 
      
      if (brw_wm_is_glsl(fp))
	  wm.wm5.enable_8_pix = 1;
      else
	  wm.wm5.enable_16_pix = 1;
   }

   wm.wm5.thread_dispatch_enable = 1;	/* AKA: color_write */
   wm.wm5.legacy_line_rast = 0;
   wm.wm5.legacy_global_depth_bias = 0;
   wm.wm5.early_depth_test = 1;	        /* never need to disable */
   wm.wm5.line_aa_region_width = 0;
   wm.wm5.line_endcap_aa_region_width = 1;

   /* _NEW_POLYGONSTIPPLE */
   if (brw->attribs.Polygon->StippleFlag) 
      wm.wm5.polygon_stipple = 1;

   /* _NEW_POLYGON */
   if (brw->attribs.Polygon->OffsetFill) {
      wm.wm5.depth_offset = 1;
      /* Something wierd going on with legacy_global_depth_bias,
       * offset_constant, scaling and MRD.  This value passes glean
       * but gives some odd results elsewere (eg. the
       * quad-offset-units test).
       */
      wm.global_depth_offset_constant = brw->attribs.Polygon->OffsetUnits * 2;

      /* This is the only value that passes glean:
       */
      wm.global_depth_offset_scale = brw->attribs.Polygon->OffsetFactor;
   }

   /* _NEW_LINE */
   if (brw->attribs.Line->StippleFlag) {
      wm.wm5.line_stipple = 1;
   }

   if (INTEL_DEBUG & DEBUG_STATS || intel->stats_wm)
      wm.wm4.stats_enable = 1;

   reloc_bufs[0] = brw->wm.prog_bo;
   reloc_bufs[1] = brw->wm.scratch_buffer;
   reloc_bufs[2] = brw->wm.sampler_bo;

   brw->wm.thread0_delta = wm.thread0.grf_reg_count << 1;
   brw->wm.thread2_delta = wm.thread2.per_thread_scratch_space;
   brw->wm.wm4_delta = wm.wm4.stats_enable | (wm.wm4.sampler_count << 2);

   dri_bo_unreference(brw->wm.state_bo);
   brw->wm.state_bo = brw_cache_data( &brw->cache, BRW_WM_UNIT, &wm,
				      reloc_bufs, 3 );
}

static void emit_reloc_wm_unit(struct brw_context *brw)
{
   /* Emit WM program relocation */
   dri_emit_reloc(brw->wm.state_bo,
		  DRM_BO_FLAG_MEM_TT | DRM_BO_FLAG_READ,
		  brw->wm.thread0_delta,
		  offsetof(struct brw_wm_unit_state, thread0),
		  brw->wm.prog_bo);

   /* Emit scratch space relocation */
   if (brw->wm.scratch_buffer != NULL) {
      dri_emit_reloc(brw->wm.state_bo,
		     DRM_BO_FLAG_MEM_TT | DRM_BO_FLAG_READ | DRM_BO_FLAG_WRITE,
		     brw->wm.thread2_delta,
		     offsetof(struct brw_wm_unit_state, thread2),
		     brw->wm.scratch_buffer);
   }

   /* Emit sampler state relocation */
   if (brw->wm.sampler_bo != NULL) {
      dri_emit_reloc(brw->wm.state_bo,
		     DRM_BO_FLAG_MEM_TT | DRM_BO_FLAG_READ,
		     brw->wm.wm4_delta,
		     offsetof(struct brw_wm_unit_state, wm4),
		     brw->wm.sampler_bo);
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
	      BRW_NEW_LOCK),

      .cache = (CACHE_NEW_SURFACE | 
		CACHE_NEW_WM_PROG | 
		CACHE_NEW_SAMPLER)
   },
   .update = upload_wm_unit,
   .emit_reloc = emit_reloc_wm_unit,
};

