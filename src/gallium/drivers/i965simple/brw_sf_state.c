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
#include "util/u_math.h"
#include "util/u_memory.h"


static void upload_sf_vp(struct brw_context *brw)
{
   struct brw_sf_viewport sfv;

   memset(&sfv, 0, sizeof(sfv));


   /* BRW_NEW_VIEWPORT */
   {
      const float *scale = brw->attribs.Viewport.scale;
      const float *trans = brw->attribs.Viewport.translate;

      sfv.viewport.m00 = scale[0];
      sfv.viewport.m11 = scale[1];
      sfv.viewport.m22 = scale[2]; 
      sfv.viewport.m30 = trans[0];
      sfv.viewport.m31 = trans[1];
      sfv.viewport.m32 = trans[2];
   }

   /* _NEW_SCISSOR */
   sfv.scissor.xmin = brw->attribs.Scissor.minx;
   sfv.scissor.xmax = brw->attribs.Scissor.maxx - 1;
   sfv.scissor.ymin = brw->attribs.Scissor.miny;
   sfv.scissor.ymax = brw->attribs.Scissor.maxy - 1;

   brw->sf.vp_gs_offset = brw_cache_data( &brw->cache[BRW_SF_VP], &sfv );
}

const struct brw_tracked_state brw_sf_vp = {
   .dirty = {
      .brw   = (BRW_NEW_SCISSOR |
		BRW_NEW_VIEWPORT),
      .cache = 0
   },
   .update = upload_sf_vp
};

static void upload_sf_unit( struct brw_context *brw )
{
   struct brw_sf_unit_state sf;
   memset(&sf, 0, sizeof(sf));

   /* CACHE_NEW_SF_PROG */
   sf.thread0.grf_reg_count = align(brw->sf.prog_data->total_grf, 16) / 16 - 1;
   sf.thread0.kernel_start_pointer = brw->sf.prog_gs_offset >> 6;
   sf.thread3.urb_entry_read_length = brw->sf.prog_data->urb_read_length;

   sf.thread1.floating_point_mode = BRW_FLOATING_POINT_NON_IEEE_754;
   sf.thread3.dispatch_grf_start_reg = 3;
   sf.thread3.urb_entry_read_offset = 1;

   /* BRW_NEW_URB_FENCE */
   sf.thread4.nr_urb_entries = brw->urb.nr_sf_entries;
   sf.thread4.urb_entry_allocation_size = brw->urb.sfsize - 1;
   sf.thread4.max_threads = MIN2(12, brw->urb.nr_sf_entries / 2) - 1;

   if (BRW_DEBUG & DEBUG_SINGLE_THREAD)
      sf.thread4.max_threads = 0;

   if (BRW_DEBUG & DEBUG_STATS)
      sf.thread4.stats_enable = 1;

   /* CACHE_NEW_SF_VP */
   sf.sf5.sf_viewport_state_offset = brw->sf.vp_gs_offset >> 5;
   sf.sf5.viewport_transform = 1;

   /* BRW_NEW_RASTER */
   if (brw->attribs.Raster->scissor)
      sf.sf6.scissor = 1;

#if 0
   if (brw->attribs.Polygon->FrontFace == GL_CCW)
      sf.sf5.front_winding = BRW_FRONTWINDING_CCW;
   else
      sf.sf5.front_winding = BRW_FRONTWINDING_CW;


   if (brw->attribs.Polygon->CullFlag) {
      switch (brw->attribs.Polygon->CullFaceMode) {
      case GL_FRONT:
	 sf.sf6.cull_mode = BRW_CULLMODE_FRONT;
	 break;
      case GL_BACK:
	 sf.sf6.cull_mode = BRW_CULLMODE_BACK;
	 break;
      case GL_FRONT_AND_BACK:
	 sf.sf6.cull_mode = BRW_CULLMODE_BOTH;
	 break;
      default:
	 assert(0);
	 break;
      }
   }
   else
      sf.sf6.cull_mode = BRW_CULLMODE_NONE;
#else
   sf.sf5.front_winding = BRW_FRONTWINDING_CCW;
   sf.sf6.cull_mode = BRW_CULLMODE_NONE;
#endif

   sf.sf6.line_width = CLAMP(brw->attribs.Raster->line_width, 1.0, 5.0) * (1<<1);

   sf.sf6.line_endcap_aa_region_width = 1;
   if (brw->attribs.Raster->line_smooth)
      sf.sf6.aa_enable = 1;
   else if (sf.sf6.line_width <= 0x2)
       sf.sf6.line_width = 0;

   sf.sf6.point_rast_rule = 1;	/* opengl conventions */

   sf.sf7.sprite_point = brw->attribs.Raster->point_sprite;
   sf.sf7.point_size = CLAMP(brw->attribs.Raster->line_width, 1.0, 255.0) * (1<<3);
   sf.sf7.use_point_size_state = !brw->attribs.Raster->point_size_per_vertex;

   /* might be BRW_NEW_PRIMITIVE if we have to adjust pv for polygons:
    */
   sf.sf7.trifan_pv = 2;
   sf.sf7.linestrip_pv = 1;
   sf.sf7.tristrip_pv = 2;
   sf.sf7.line_last_pixel_enable = 0;

   /* Set bias for OpenGL rasterization rules:
    */
   sf.sf6.dest_org_vbias = 0x8;
   sf.sf6.dest_org_hbias = 0x8;

   brw->sf.state_gs_offset = brw_cache_data( &brw->cache[BRW_SF_UNIT], &sf );
}


const struct brw_tracked_state brw_sf_unit = {
   .dirty = {
      .brw   = (BRW_NEW_RASTERIZER |
		BRW_NEW_URB_FENCE),
      .cache = (CACHE_NEW_SF_VP |
		CACHE_NEW_SF_PROG)
   },
   .update = upload_sf_unit
};


