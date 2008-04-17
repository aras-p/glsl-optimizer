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
#include "macros.h"

struct brw_vs_unit_key {
   unsigned int total_grf;
   unsigned int urb_entry_read_length;
   unsigned int curb_entry_read_length;

   unsigned int curbe_offset;

   unsigned int nr_urb_entries, urb_size;
};

static void
vs_unit_populate_key(struct brw_context *brw, struct brw_vs_unit_key *key)
{
   memset(key, 0, sizeof(*key));

   /* CACHE_NEW_VS_PROG */
   key->total_grf = brw->vs.prog_data->total_grf;
   key->urb_entry_read_length = brw->vs.prog_data->urb_read_length;
   key->curb_entry_read_length = brw->vs.prog_data->curb_read_length;

   /* BRW_NEW_URB_FENCE */
   key->nr_urb_entries = brw->urb.nr_vs_entries;
   key->urb_size = brw->urb.vsize;

   /* BRW_NEW_CURBE_OFFSETS, _NEW_TRANSFORM */
   if (brw->attribs.Transform->ClipPlanesEnabled) {
      /* Note that we read in the userclip planes as well, hence
       * clip_start:
       */
      key->curbe_offset = brw->curbe.clip_start;
   }
   else {
      key->curbe_offset = brw->curbe.vs_start;
   }
}

static dri_bo *
vs_unit_create_from_key(struct brw_context *brw, struct brw_vs_unit_key *key)
{
   struct brw_vs_unit_state vs;
   dri_bo *bo;

   memset(&vs, 0, sizeof(vs));

   vs.thread0.kernel_start_pointer = brw->vs.prog_bo->offset >> 6; /* reloc */
   vs.thread0.grf_reg_count = ALIGN(key->total_grf, 16) / 16 - 1;
   vs.thread1.floating_point_mode = BRW_FLOATING_POINT_NON_IEEE_754;
   vs.thread3.urb_entry_read_length = key->urb_entry_read_length;
   vs.thread3.const_urb_entry_read_length = key->curb_entry_read_length;
   vs.thread3.dispatch_grf_start_reg = 1;
   vs.thread3.urb_entry_read_offset = 0;
   vs.thread3.const_urb_entry_read_offset = key->curbe_offset * 2;

   vs.thread4.nr_urb_entries = key->nr_urb_entries;
   vs.thread4.urb_entry_allocation_size = key->urb_size - 1;
   vs.thread4.max_threads = MIN2(MAX2(0, (key->nr_urb_entries - 6) / 2 - 1),
				 15);

   if (INTEL_DEBUG & DEBUG_SINGLE_THREAD)
      vs.thread4.max_threads = 0;

   /* No samplers for ARB_vp programs:
    */
   vs.vs5.sampler_count = 0;

   if (INTEL_DEBUG & DEBUG_STATS)
      vs.thread4.stats_enable = 1;

   /* Vertex program always enabled:
    */
   vs.vs6.vs_enable = 1;

   bo = brw_upload_cache(&brw->cache, BRW_VS_UNIT,
			 key, sizeof(*key),
			 &brw->vs.prog_bo, 1,
			 &vs, sizeof(vs),
			 NULL, NULL);

   /* Emit VS program relocation */
   dri_emit_reloc(bo,
		  DRM_BO_FLAG_MEM_TT | DRM_BO_FLAG_READ,
		  vs.thread0.grf_reg_count << 1,
		  offsetof(struct brw_vs_unit_state, thread0),
		  brw->vs.prog_bo);

   return bo;
}

static int prepare_vs_unit( struct brw_context *brw )
{
   struct brw_vs_unit_key key;

   vs_unit_populate_key(brw, &key);

   dri_bo_unreference(brw->vs.state_bo);
   brw->vs.state_bo = brw_search_cache(&brw->cache, BRW_VS_UNIT,
				       &key, sizeof(key),
				       &brw->vs.prog_bo, 1,
				       NULL);
   if (brw->vs.state_bo == NULL) {
      brw->vs.state_bo = vs_unit_create_from_key(brw, &key);
   }
   return dri_bufmgr_check_aperture_space(brw->vs.state_bo);
}

const struct brw_tracked_state brw_vs_unit = {
   .dirty = {
      .mesa  = _NEW_TRANSFORM,
      .brw   = (BRW_NEW_CURBE_OFFSETS |
		BRW_NEW_URB_FENCE),
      .cache = CACHE_NEW_VS_PROG
   },
   .prepare = prepare_vs_unit,
};
