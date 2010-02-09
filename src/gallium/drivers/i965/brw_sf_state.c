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

#include "pipe/p_state.h"

#include "brw_context.h"
#include "brw_state.h"
#include "brw_defines.h"
#include "brw_debug.h"
#include "brw_pipe_rast.h"

static enum pipe_error upload_sf_vp(struct brw_context *brw)
{
   const struct pipe_viewport_state *vp = &brw->curr.viewport;
   const struct pipe_scissor_state *scissor = &brw->curr.scissor;
   struct brw_sf_viewport sfv;
   enum pipe_error ret;

   memset(&sfv, 0, sizeof(sfv));

   /* PIPE_NEW_VIEWPORT, PIPE_NEW_SCISSOR */

   sfv.viewport.m00 = vp->scale[0];
   sfv.viewport.m11 = vp->scale[1];
   sfv.viewport.m22 = vp->scale[2];
   sfv.viewport.m30 = vp->translate[0];
   sfv.viewport.m31 = vp->translate[1];
   sfv.viewport.m32 = vp->translate[2];

   sfv.scissor.xmin = scissor->minx;
   sfv.scissor.xmax = scissor->maxx - 1; /* ? */
   sfv.scissor.ymin = scissor->miny;
   sfv.scissor.ymax = scissor->maxy - 1; /* ? */

   ret = brw_cache_data( &brw->cache, BRW_SF_VP, &sfv, NULL, 0,
                         &brw->sf.vp_bo );
   if (ret)
      return ret;

   return PIPE_OK;
}

const struct brw_tracked_state brw_sf_vp = {
   .dirty = {
      .mesa  = (PIPE_NEW_VIEWPORT | 
		PIPE_NEW_SCISSOR),
      .brw   = 0,
      .cache = 0
   },
   .prepare = upload_sf_vp
};

struct brw_sf_unit_key {
   unsigned int total_grf;
   unsigned int urb_entry_read_length;
   unsigned int nr_urb_entries, urb_size, sfsize;
   
   unsigned scissor:1;
   unsigned line_smooth:1;
   unsigned point_sprite:1;
   unsigned point_attenuated:1;
   unsigned front_face:2;
   unsigned cull_mode:2;
   unsigned flatshade_first:1;
   unsigned gl_rasterization_rules:1;
   unsigned line_last_pixel_enable:1;
   float line_width;
   float point_size;
};

static void
sf_unit_populate_key(struct brw_context *brw, struct brw_sf_unit_key *key)
{
   const struct pipe_rasterizer_state *rast = &brw->curr.rast->templ;
   memset(key, 0, sizeof(*key));

   /* CACHE_NEW_SF_PROG */
   key->total_grf = brw->sf.prog_data->total_grf;
   key->urb_entry_read_length = brw->sf.prog_data->urb_read_length;

   /* BRW_NEW_URB_FENCE */
   key->nr_urb_entries = brw->urb.nr_sf_entries;
   key->urb_size = brw->urb.vsize;
   key->sfsize = brw->urb.sfsize;

   /* PIPE_NEW_RAST */
   key->scissor = rast->scissor;
   key->front_face = rast->front_winding;
   key->cull_mode = rast->cull_mode;
   key->line_smooth = rast->line_smooth;
   key->line_width = rast->line_width;
   key->flatshade_first = rast->flatshade_first;
   key->line_last_pixel_enable = rast->line_last_pixel;
   key->gl_rasterization_rules = rast->gl_rasterization_rules;

   key->point_sprite = rast->sprite_coord_enable ? 1 : 0;
   key->point_attenuated = rast->point_size_per_vertex;

   key->point_size = rast->point_size;
}

static enum pipe_error
sf_unit_create_from_key(struct brw_context *brw,
                        struct brw_sf_unit_key *key,
                        struct brw_winsys_reloc *reloc,
                        struct brw_winsys_buffer **bo_out)
{
   struct brw_sf_unit_state sf;
   enum pipe_error ret;
   int chipset_max_threads;
   memset(&sf, 0, sizeof(sf));

   sf.thread0.grf_reg_count = align(key->total_grf, 16) / 16 - 1;
   /* reloc */
   sf.thread0.kernel_start_pointer = 0;

   sf.thread1.floating_point_mode = BRW_FLOATING_POINT_NON_IEEE_754;

   sf.thread3.dispatch_grf_start_reg = 3;

   if (BRW_IS_IGDNG(brw))
       sf.thread3.urb_entry_read_offset = 3;
   else
       sf.thread3.urb_entry_read_offset = 1;

   sf.thread3.urb_entry_read_length = key->urb_entry_read_length;

   sf.thread4.nr_urb_entries = key->nr_urb_entries;
   sf.thread4.urb_entry_allocation_size = key->sfsize - 1;

   /* Each SF thread produces 1 PUE, and there can be up to 24(Pre-IGDNG) or 
    * 48(IGDNG) threads 
    */
   if (BRW_IS_IGDNG(brw))
      chipset_max_threads = 48;
   else
      chipset_max_threads = 24;

   sf.thread4.max_threads = MIN2(chipset_max_threads, key->nr_urb_entries) - 1;

   if (BRW_DEBUG & DEBUG_SINGLE_THREAD)
      sf.thread4.max_threads = 0;

   if (BRW_DEBUG & DEBUG_STATS)
      sf.thread4.stats_enable = 1;

   /* CACHE_NEW_SF_VP */
   /* reloc */
   sf.sf5.sf_viewport_state_offset = 0;

   sf.sf5.viewport_transform = 1;

   if (key->scissor)
      sf.sf6.scissor = 1;

   if (key->front_face == PIPE_WINDING_CCW)
      sf.sf5.front_winding = BRW_FRONTWINDING_CCW;
   else
      sf.sf5.front_winding = BRW_FRONTWINDING_CW;

   switch (key->cull_mode) {
   case PIPE_WINDING_CCW:
   case PIPE_WINDING_CW:
      sf.sf6.cull_mode = (key->front_face == key->cull_mode ?
			  BRW_CULLMODE_FRONT :
			  BRW_CULLMODE_BACK);
      break;
   case PIPE_WINDING_BOTH:
      sf.sf6.cull_mode = BRW_CULLMODE_BOTH;
      break;
   case PIPE_WINDING_NONE:
      sf.sf6.cull_mode = BRW_CULLMODE_NONE;
      break;
   default:
      assert(0);
      sf.sf6.cull_mode = BRW_CULLMODE_NONE;
      break;
   }

   /* _NEW_LINE */
   /* XXX use ctx->Const.Min/MaxLineWidth here */
   sf.sf6.line_width = CLAMP(key->line_width, 1.0, 5.0) * (1<<1);

   sf.sf6.line_endcap_aa_region_width = 1;
   if (key->line_smooth)
      sf.sf6.aa_enable = 1;
   else if (sf.sf6.line_width <= 0x2)
       sf.sf6.line_width = 0;

   /* XXX: gl_rasterization_rules?  something else?
    */
   sf.sf6.point_rast_rule = BRW_RASTRULE_UPPER_RIGHT;
   sf.sf6.point_rast_rule = BRW_RASTRULE_LOWER_RIGHT;
   sf.sf6.point_rast_rule = 1;

   /* XXX clamp max depends on AA vs. non-AA */

   /* _NEW_POINT */
   sf.sf7.sprite_point = key->point_sprite;
   sf.sf7.point_size = CLAMP(rint(key->point_size), 1, 255) * (1<<3);
   sf.sf7.use_point_size_state = !key->point_attenuated;
   sf.sf7.aa_line_distance_mode = 0;

   /* might be BRW_NEW_PRIMITIVE if we have to adjust pv for polygons:
    */
   if (!key->flatshade_first) {
      sf.sf7.trifan_pv = 2;
      sf.sf7.linestrip_pv = 1;
      sf.sf7.tristrip_pv = 2;
   } else {
      sf.sf7.trifan_pv = 1;
      sf.sf7.linestrip_pv = 0;
      sf.sf7.tristrip_pv = 0;
   }

   sf.sf7.line_last_pixel_enable = key->line_last_pixel_enable;

   /* Set bias for OpenGL rasterization rules:
    */
   if (key->gl_rasterization_rules) {
      sf.sf6.dest_org_vbias = 0x8;
      sf.sf6.dest_org_hbias = 0x8;
   }
   else {
      sf.sf6.dest_org_vbias = 0x0;
      sf.sf6.dest_org_hbias = 0x0;
   }

   ret = brw_upload_cache(&brw->cache, BRW_SF_UNIT,
                          key, sizeof(*key),
                          reloc, 2,
                          &sf, sizeof(sf),
                          NULL, NULL,
                          bo_out);
   if (ret)
      return ret;

   
   return PIPE_OK;
}

static enum pipe_error upload_sf_unit( struct brw_context *brw )
{
   struct brw_sf_unit_key key;
   struct brw_winsys_reloc reloc[2];
   unsigned total_grf;
   unsigned viewport_transform;
   unsigned front_winding;
   enum pipe_error ret;

   sf_unit_populate_key(brw, &key);
   
   /* XXX: cut this crap and pre calculate the key:
    */
   total_grf = (align(key.total_grf, 16) / 16 - 1);
   viewport_transform = 1;
   front_winding = (key.front_face == PIPE_WINDING_CCW ?
                    BRW_FRONTWINDING_CCW :
                    BRW_FRONTWINDING_CW);

   /* Emit SF program relocation */
   make_reloc(&reloc[0],
              BRW_USAGE_STATE,
              total_grf << 1,
              offsetof(struct brw_sf_unit_state, thread0),
              brw->sf.prog_bo);

   /* Emit SF viewport relocation */
   make_reloc(&reloc[1],
              BRW_USAGE_STATE,
              front_winding | (viewport_transform << 1),
              offsetof(struct brw_sf_unit_state, sf5),
              brw->sf.vp_bo);


   if (brw_search_cache(&brw->cache, BRW_SF_UNIT,
                        &key, sizeof(key),
                        reloc, 2,
                        NULL,
                        &brw->sf.state_bo))
      return PIPE_OK;


   ret = sf_unit_create_from_key(brw, &key,
                                 reloc,
                                 &brw->sf.state_bo);
   if (ret)
      return ret;

   return PIPE_OK;
}

const struct brw_tracked_state brw_sf_unit = {
   .dirty = {
      .mesa  = (PIPE_NEW_RAST),
      .brw   = BRW_NEW_URB_FENCE,
      .cache = (CACHE_NEW_SF_VP |
		CACHE_NEW_SF_PROG)
   },
   .prepare = upload_sf_unit,
};
