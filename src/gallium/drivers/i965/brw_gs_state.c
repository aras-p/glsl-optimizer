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
#include "brw_debug.h"

struct brw_gs_unit_key {
   unsigned int total_grf;
   unsigned int urb_entry_read_length;

   unsigned int curbe_offset;

   unsigned int nr_urb_entries, urb_size;
   GLboolean prog_active;
};

static void
gs_unit_populate_key(struct brw_context *brw, struct brw_gs_unit_key *key)
{
   memset(key, 0, sizeof(*key));

   /* CACHE_NEW_GS_PROG */
   key->prog_active = brw->gs.prog_active;
   if (key->prog_active) {
      key->total_grf = brw->gs.prog_data->total_grf;
      key->urb_entry_read_length = brw->gs.prog_data->urb_read_length;
   } else {
      key->total_grf = 1;
      key->urb_entry_read_length = 1;
   }

   /* BRW_NEW_CURBE_OFFSETS */
   key->curbe_offset = brw->curbe.clip_start;

   /* BRW_NEW_URB_FENCE */
   key->nr_urb_entries = brw->urb.nr_gs_entries;
   key->urb_size = brw->urb.vsize;
}

static enum pipe_error
gs_unit_create_from_key(struct brw_context *brw, 
                        struct brw_gs_unit_key *key,
                        struct brw_winsys_reloc *reloc,
                        unsigned nr_reloc,
                        struct brw_winsys_buffer **bo_out)
{
   struct brw_gs_unit_state gs;
   enum pipe_error ret;


   memset(&gs, 0, sizeof(gs));

   /* reloc */
   gs.thread0.grf_reg_count = align(key->total_grf, 16) / 16 - 1;
   gs.thread0.kernel_start_pointer = 0;

   gs.thread1.floating_point_mode = BRW_FLOATING_POINT_NON_IEEE_754;
   gs.thread1.single_program_flow = 1;

   gs.thread3.dispatch_grf_start_reg = 1;
   gs.thread3.const_urb_entry_read_offset = 0;
   gs.thread3.const_urb_entry_read_length = 0;
   gs.thread3.urb_entry_read_offset = 0;
   gs.thread3.urb_entry_read_length = key->urb_entry_read_length;

   gs.thread4.nr_urb_entries = key->nr_urb_entries;
   gs.thread4.urb_entry_allocation_size = key->urb_size - 1;

   if (key->nr_urb_entries >= 8)
      gs.thread4.max_threads = 1;
   else
      gs.thread4.max_threads = 0;

   if (BRW_IS_IGDNG(brw))
      gs.thread4.rendering_enable = 1;

   if (BRW_DEBUG & DEBUG_STATS)
      gs.thread4.stats_enable = 1;

   ret = brw_upload_cache(&brw->cache, BRW_GS_UNIT,
                          key, sizeof(*key),
                          reloc, nr_reloc,
                          &gs, sizeof(gs),
                          NULL, NULL,
                          bo_out);
   if (ret)
      return ret;

   return PIPE_OK;
}

static enum pipe_error prepare_gs_unit(struct brw_context *brw)
{
   struct brw_gs_unit_key key;
   enum pipe_error ret;
   struct brw_winsys_reloc reloc[1];
   unsigned nr_reloc = 0;
   unsigned grf_reg_count;

   gs_unit_populate_key(brw, &key);

   grf_reg_count = (align(key.total_grf, 16) / 16 - 1);

   /* GS program relocation */
   if (key.prog_active) {
      make_reloc(&reloc[nr_reloc++],
                 BRW_USAGE_STATE,
                 grf_reg_count << 1,
                 offsetof(struct brw_gs_unit_state, thread0),
                 brw->gs.prog_bo);
   }

   if (brw_search_cache(&brw->cache, BRW_GS_UNIT,
                        &key, sizeof(key),
                        reloc, nr_reloc,
                        NULL,
                        &brw->gs.state_bo))
      return PIPE_OK;

   ret = gs_unit_create_from_key(brw, &key,
                                 reloc, nr_reloc,
                                 &brw->gs.state_bo);
   if (ret)
      return ret;

   return PIPE_OK;
}

const struct brw_tracked_state brw_gs_unit = {
   .dirty = {
      .mesa  = 0,
      .brw   = (BRW_NEW_CURBE_OFFSETS |
		BRW_NEW_URB_FENCE),
      .cache = CACHE_NEW_GS_PROG
   },
   .prepare = prepare_gs_unit,
};
