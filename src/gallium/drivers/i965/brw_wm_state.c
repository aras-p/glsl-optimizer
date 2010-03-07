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
                   
#include "util/u_math.h"

#include "brw_context.h"
#include "brw_state.h"
#include "brw_defines.h"
#include "brw_wm.h"
#include "brw_debug.h"
#include "brw_pipe_rast.h"

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
   GLboolean uses_depth, computes_depth, uses_kill, has_flow_control;
   GLboolean polygon_stipple, stats_wm, line_stipple, offset_enable;
   GLfloat offset_units, offset_factor;
};

static void
wm_unit_populate_key(struct brw_context *brw, struct brw_wm_unit_key *key)
{
   const struct brw_fragment_shader *fp = brw->curr.fragment_shader;

   memset(key, 0, sizeof(*key));

   if (BRW_DEBUG & DEBUG_SINGLE_THREAD)
      key->max_threads = 1;
   else {
      /* WM maximum threads is number of EUs times number of threads per EU. */
      if (BRW_IS_IGDNG(brw))
         key->max_threads = 12 * 6;
      else if (BRW_IS_G4X(brw))
	 key->max_threads = 10 * 5;
      else
	 key->max_threads = 8 * 4;
   }

   /* CACHE_NEW_WM_PROG */
   key->total_grf = brw->wm.prog_data->total_grf;
   key->urb_entry_read_length = brw->wm.prog_data->urb_read_length;
   key->curb_entry_read_length = brw->wm.prog_data->curb_read_length;
   key->dispatch_grf_start_reg = brw->wm.prog_data->first_curbe_grf;
   key->total_scratch = align(brw->wm.prog_data->total_scratch, 1024);

   /* BRW_NEW_URB_FENCE */
   key->urb_size = brw->urb.vsize;

   /* BRW_NEW_CURBE_OFFSETS */
   key->curbe_offset = brw->curbe.wm_start;

   /* BRW_NEW_NR_SURFACEs */
   key->nr_surfaces = brw->wm.nr_surfaces;

   /* CACHE_NEW_SAMPLER */
   key->sampler_count = brw->wm.sampler_count;

   /* PIPE_NEW_RAST */
   key->polygon_stipple = brw->curr.rast->templ.poly_stipple_enable;

   /* PIPE_NEW_FRAGMENT_PROGRAM */
   key->uses_depth = fp->uses_depth;
   key->computes_depth = fp->info.writes_z;

   /* PIPE_NEW_DEPTH_BUFFER
    *
    * Override for NULL depthbuffer case, required by the Pixel Shader Computed
    * Depth field.
    */
   if (brw->curr.fb.zsbuf == NULL)
      key->computes_depth = 0;

   /* PIPE_NEW_DEPTH_STENCIL_ALPHA */
   key->uses_kill = (fp->info.uses_kill || 
		     brw->curr.zstencil->cc3.alpha_test);

   key->has_flow_control = fp->has_flow_control;

   /* temporary sanity check assertion */
   assert(fp->has_flow_control == 0);

   /* PIPE_NEW_QUERY */
   key->stats_wm = (brw->query.stats_wm != 0);

   /* PIPE_NEW_RAST */
   key->line_stipple = brw->curr.rast->templ.line_stipple_enable;


   key->offset_enable = (brw->curr.rast->templ.offset_cw ||
			 brw->curr.rast->templ.offset_ccw);

   key->offset_units = brw->curr.rast->templ.offset_units;
   key->offset_factor = brw->curr.rast->templ.offset_scale;
}

/**
 * Setup wm hardware state.  See page 225 of Volume 2
 */
static enum pipe_error
wm_unit_create_from_key(struct brw_context *brw, struct brw_wm_unit_key *key,
			struct brw_winsys_reloc *reloc,
                        unsigned nr_reloc,
                        struct brw_winsys_buffer **bo_out)
{
   struct brw_wm_unit_state wm;
   enum pipe_error ret;

   memset(&wm, 0, sizeof(wm));

   wm.thread0.grf_reg_count = align(key->total_grf, 16) / 16 - 1;
   wm.thread0.kernel_start_pointer = 0; /* reloc */
   wm.thread1.depth_coef_urb_read_offset = 1;
   wm.thread1.floating_point_mode = BRW_FLOATING_POINT_NON_IEEE_754;

   if (BRW_IS_IGDNG(brw))
      wm.thread1.binding_table_entry_count = 0; /* hardware requirement */
   else
      wm.thread1.binding_table_entry_count = key->nr_surfaces;

   if (key->total_scratch != 0) {
      wm.thread2.scratch_space_base_pointer = 0; /* reloc */
      wm.thread2.per_thread_scratch_space = key->total_scratch / 1024 - 1;
   } else {
      wm.thread2.scratch_space_base_pointer = 0;
      wm.thread2.per_thread_scratch_space = 0;
   }

   wm.thread3.dispatch_grf_start_reg = key->dispatch_grf_start_reg;
   wm.thread3.urb_entry_read_length = key->urb_entry_read_length;
   wm.thread3.urb_entry_read_offset = 0;
   wm.thread3.const_urb_entry_read_length = key->curb_entry_read_length;
   wm.thread3.const_urb_entry_read_offset = key->curbe_offset * 2;

   if (BRW_IS_IGDNG(brw)) 
      wm.wm4.sampler_count = 0; /* hardware requirement */
   else
      wm.wm4.sampler_count = (key->sampler_count + 1) / 4;

   /* reloc */
   wm.wm4.sampler_state_pointer = 0;

   wm.wm5.program_uses_depth = key->uses_depth;
   wm.wm5.program_computes_depth = key->computes_depth;
   wm.wm5.program_uses_killpixel = key->uses_kill;

   if (key->has_flow_control)
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

   if ((BRW_DEBUG & DEBUG_STATS) || key->stats_wm)
      wm.wm4.stats_enable = 1;

   ret = brw_upload_cache(&brw->cache, BRW_WM_UNIT,
                          key, sizeof(*key),
                          reloc, nr_reloc,
                          &wm, sizeof(wm),
                          NULL, NULL,
                          bo_out);
   if (ret)
      return ret;

   return PIPE_OK;
}


static enum pipe_error upload_wm_unit( struct brw_context *brw )
{
   struct brw_wm_unit_key key;
   struct brw_winsys_reloc reloc[3];
   unsigned nr_reloc = 0;
   enum pipe_error ret;
   unsigned grf_reg_count;
   unsigned per_thread_scratch_space;
   unsigned stats_enable;
   unsigned sampler_count;

   wm_unit_populate_key(brw, &key);


   /* Allocate the necessary scratch space if we haven't already.  Don't
    * bother reducing the allocation later, since we use scratch so
    * rarely.
    */
   assert(key.total_scratch <= 12 * 1024);
   if (key.total_scratch) {
      GLuint total = key.total_scratch * key.max_threads;

      /* Do we need a new buffer:
       */
      if (brw->wm.scratch_bo && total > brw->wm.scratch_bo->size) 
	 bo_reference(&brw->wm.scratch_bo, NULL);

      if (brw->wm.scratch_bo == NULL) {
	 ret = brw->sws->bo_alloc(brw->sws,
                                  BRW_BUFFER_TYPE_SHADER_SCRATCH,
                                  total,
                                  4096,
                                  &brw->wm.scratch_bo);
         if (ret)
            return ret;
      }
   }


   /* XXX: temporary:
    */
   grf_reg_count = (align(key.total_grf, 16) / 16 - 1);
   per_thread_scratch_space = key.total_scratch / 1024 - 1;
   stats_enable = (BRW_DEBUG & DEBUG_STATS) || key.stats_wm;
   sampler_count = BRW_IS_IGDNG(brw) ? 0 :(key.sampler_count + 1) / 4;

   /* Emit WM program relocation */
   make_reloc(&reloc[nr_reloc++],
              BRW_USAGE_STATE,
              grf_reg_count << 1,
              offsetof(struct brw_wm_unit_state, thread0),
              brw->wm.prog_bo);

   /* Emit scratch space relocation */
   if (key.total_scratch != 0) {
      make_reloc(&reloc[nr_reloc++],
                 BRW_USAGE_SCRATCH,
                 per_thread_scratch_space,
                 offsetof(struct brw_wm_unit_state, thread2),
                 brw->wm.scratch_bo);
   }

   /* Emit sampler state relocation */
   if (key.sampler_count != 0) {
      make_reloc(&reloc[nr_reloc++],
                 BRW_USAGE_STATE,
                 stats_enable | (sampler_count << 2),
                 offsetof(struct brw_wm_unit_state, wm4),
                 brw->wm.sampler_bo);
   }


   if (brw_search_cache(&brw->cache, BRW_WM_UNIT,
                        &key, sizeof(key),
                        reloc, nr_reloc,
                        NULL,
                        &brw->wm.state_bo))
      return PIPE_OK;

   ret = wm_unit_create_from_key(brw, &key, 
                                 reloc, nr_reloc,
                                 &brw->wm.state_bo);
   if (ret)
      return ret;

   return PIPE_OK;
}

const struct brw_tracked_state brw_wm_unit = {
   .dirty = {
      .mesa = (PIPE_NEW_FRAGMENT_SHADER |
	       PIPE_NEW_DEPTH_BUFFER |
	       PIPE_NEW_RAST | 
	       PIPE_NEW_DEPTH_STENCIL_ALPHA |
	       PIPE_NEW_QUERY),

      .brw = (BRW_NEW_CURBE_OFFSETS |
	      BRW_NEW_NR_WM_SURFACES),

      .cache = (CACHE_NEW_WM_PROG |
		CACHE_NEW_SAMPLER)
   },
   .prepare = upload_wm_unit,
};

