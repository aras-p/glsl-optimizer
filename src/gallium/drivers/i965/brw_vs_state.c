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


#include "brw_debug.h"
#include "brw_context.h"
#include "brw_state.h"
#include "brw_defines.h"

struct brw_vs_unit_key {
   unsigned int total_grf;
   unsigned int urb_entry_read_length;
   unsigned int curb_entry_read_length;

   unsigned int curbe_offset;

   unsigned int nr_urb_entries, urb_size;

   unsigned int nr_surfaces;
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

   /* BRW_NEW_NR_VS_SURFACES */
   key->nr_surfaces = brw->vs.nr_surfaces;

   /* PIPE_NEW_CLIP */
   if (brw->curr.ucp.nr) {
      /* Note that we read in the userclip planes as well, hence
       * clip_start:
       */
      key->curbe_offset = brw->curbe.clip_start;
   }
   else {
      key->curbe_offset = brw->curbe.vs_start;
   }
}

static enum pipe_error
vs_unit_create_from_key(struct brw_context *brw, 
                        struct brw_vs_unit_key *key,
                        struct brw_winsys_reloc *reloc,
                        struct brw_winsys_buffer **bo_out)
{
   enum pipe_error ret;
   struct brw_vs_unit_state vs;
   int chipset_max_threads;

   memset(&vs, 0, sizeof(vs));

   vs.thread0.kernel_start_pointer = 0; /* reloc */
   vs.thread0.grf_reg_count = align(key->total_grf, 16) / 16 - 1;
   vs.thread1.floating_point_mode = BRW_FLOATING_POINT_NON_IEEE_754;
   /* Choosing multiple program flow means that we may get 2-vertex threads,
    * which will have the channel mask for dwords 4-7 enabled in the thread,
    * and those dwords will be written to the second URB handle when we
    * brw_urb_WRITE() results.
    */
   vs.thread1.single_program_flow = 0;

   if (BRW_IS_IGDNG(brw))
      vs.thread1.binding_table_entry_count = 0; /* hardware requirement */
   else
      vs.thread1.binding_table_entry_count = key->nr_surfaces;

   vs.thread3.urb_entry_read_length = key->urb_entry_read_length;
   vs.thread3.const_urb_entry_read_length = key->curb_entry_read_length;
   vs.thread3.dispatch_grf_start_reg = 1;
   vs.thread3.urb_entry_read_offset = 0;
   vs.thread3.const_urb_entry_read_offset = key->curbe_offset * 2;

   if (BRW_IS_IGDNG(brw))
       vs.thread4.nr_urb_entries = key->nr_urb_entries >> 2;
   else
       vs.thread4.nr_urb_entries = key->nr_urb_entries;

   vs.thread4.urb_entry_allocation_size = key->urb_size - 1;

   if (BRW_IS_IGDNG(brw))
      chipset_max_threads = 72;
   else if (BRW_IS_G4X(brw))
      chipset_max_threads = 32;
   else
      chipset_max_threads = 16;

   vs.thread4.max_threads = CLAMP(key->nr_urb_entries / 2,
				  1, chipset_max_threads) - 1;

   if (BRW_DEBUG & DEBUG_SINGLE_THREAD)
      vs.thread4.max_threads = 0;

   /* No samplers for ARB_vp programs:
    */
   /* It has to be set to 0 for IGDNG
    */
   vs.vs5.sampler_count = 0;

   if (BRW_DEBUG & DEBUG_STATS)
      vs.thread4.stats_enable = 1;

   /* Vertex program always enabled:
    */
   vs.vs6.vs_enable = 1;

   ret = brw_upload_cache(&brw->cache, BRW_VS_UNIT,
                          key, sizeof(*key),
                          reloc, 1,
                          &vs, sizeof(vs),
                          NULL, NULL,
                          bo_out);
   if (ret)
      return ret;

   return PIPE_OK;
}

static int prepare_vs_unit(struct brw_context *brw)
{
   struct brw_vs_unit_key key;
   enum pipe_error ret;
   struct brw_winsys_reloc reloc[1];
   unsigned grf_reg_count;

   vs_unit_populate_key(brw, &key);

   grf_reg_count = (align(key.total_grf, 16) / 16 - 1);

   /* Emit VS program relocation */
   make_reloc(&reloc[0],
              BRW_USAGE_STATE,
              grf_reg_count << 1,
              offsetof(struct brw_vs_unit_state, thread0),
              brw->vs.prog_bo);


   if (brw_search_cache(&brw->cache, BRW_VS_UNIT,
                        &key, sizeof(key),
                        reloc, 1,
                        NULL,
                        &brw->vs.state_bo))
      return PIPE_OK;

   ret = vs_unit_create_from_key(brw, &key, reloc, &brw->vs.state_bo);
   if (ret)
      return ret;

   return PIPE_OK;
}

const struct brw_tracked_state brw_vs_unit = {
   .dirty = {
      .mesa  = (PIPE_NEW_CLIP),
      .brw   = (BRW_NEW_CURBE_OFFSETS |
                BRW_NEW_NR_VS_SURFACES |
		BRW_NEW_URB_FENCE),
      .cache = CACHE_NEW_VS_PROG
   },
   .prepare = prepare_vs_unit,
};
