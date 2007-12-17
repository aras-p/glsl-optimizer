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

static void upload_sf_vp(struct brw_context *brw)
{
   GLcontext *ctx = &brw->intel.ctx;
   const GLfloat depth_scale = 1.0F / ctx->DrawBuffer->_DepthMaxF;
   struct brw_sf_viewport sfv;

   memset(&sfv, 0, sizeof(sfv));

   /* _NEW_VIEWPORT, BRW_NEW_METAOPS */

   if (!brw->metaops.active) {
      const GLfloat *v = ctx->Viewport._WindowMap.m;

      sfv.viewport.m00 =   v[MAT_SX];
      sfv.viewport.m11 = - v[MAT_SY];
      sfv.viewport.m22 =   v[MAT_SZ] * depth_scale;
      sfv.viewport.m30 =   v[MAT_TX];
      sfv.viewport.m31 = - v[MAT_TY] + ctx->DrawBuffer->Height;
      sfv.viewport.m32 =   v[MAT_TZ] * depth_scale;
   } else {
      sfv.viewport.m00 =   1;
      sfv.viewport.m11 = - 1;
      sfv.viewport.m22 =   1;
      sfv.viewport.m30 =   0;
      sfv.viewport.m31 =   ctx->DrawBuffer->Height;
      sfv.viewport.m32 =   0;
   }

   /* _NEW_SCISSOR */

   /* The scissor only needs to handle the intersection of drawable and
    * scissor rect.  Clipping to the boundaries of static shared buffers
    * for front/back/depth is covered by looping over cliprects in brw_draw.c.
    *
    * Note that the hardware's coordinates are inclusive, while Mesa's min is
    * inclusive but max is exclusive.
    */
   sfv.scissor.xmin = ctx->DrawBuffer->_Xmin;
   sfv.scissor.xmax = ctx->DrawBuffer->_Xmax - 1;
   sfv.scissor.ymin = ctx->DrawBuffer->Height - ctx->DrawBuffer->_Ymax;
   sfv.scissor.ymax = ctx->DrawBuffer->Height - ctx->DrawBuffer->_Ymin - 1;

   dri_bo_unreference(brw->sf.vp_bo);
   brw->sf.vp_bo = brw_cache_data( &brw->cache, BRW_SF_VP, &sfv, NULL, 0 );
}

const struct brw_tracked_state brw_sf_vp = {
   .dirty = {
      .mesa  = (_NEW_VIEWPORT | 
		_NEW_SCISSOR),
      .brw   = BRW_NEW_METAOPS,
      .cache = 0
   },
   .update = upload_sf_vp
};



static void upload_sf_unit( struct brw_context *brw )
{
   struct brw_sf_unit_state sf;
   memset(&sf, 0, sizeof(sf));
   dri_bo *reloc_bufs[2];

   /* CACHE_NEW_SF_PROG */
   sf.thread0.grf_reg_count = ALIGN(brw->sf.prog_data->total_grf, 16) / 16 - 1;
   sf.thread0.kernel_start_pointer = brw->sf.prog_bo->offset >> 6; /* reloc */
   sf.thread3.urb_entry_read_length = brw->sf.prog_data->urb_read_length;

   sf.thread1.floating_point_mode = BRW_FLOATING_POINT_NON_IEEE_754;
   sf.thread3.dispatch_grf_start_reg = 3;
   sf.thread3.urb_entry_read_offset = 1;

   /* BRW_NEW_URB_FENCE */
   sf.thread4.nr_urb_entries = brw->urb.nr_sf_entries;
   sf.thread4.urb_entry_allocation_size = brw->urb.sfsize - 1;
   sf.thread4.max_threads = MIN2(12, brw->urb.nr_sf_entries / 2) - 1;

   if (INTEL_DEBUG & DEBUG_SINGLE_THREAD)
      sf.thread4.max_threads = 0; 

   if (INTEL_DEBUG & DEBUG_STATS)
      sf.thread4.stats_enable = 1; 

   /* CACHE_NEW_SF_VP */
   sf.sf5.sf_viewport_state_offset = brw->sf.vp_bo->offset >> 5; /* reloc */
   
   sf.sf5.viewport_transform = 1;
   
   /* _NEW_SCISSOR */
   if (brw->attribs.Scissor->Enabled) 
      sf.sf6.scissor = 1;  

   /* _NEW_POLYGON */
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
      

   /* _NEW_LINE */
   /* XXX use ctx->Const.Min/MaxLineWidth here */
   sf.sf6.line_width = CLAMP(brw->attribs.Line->Width, 1.0, 5.0) * (1<<1);

   sf.sf6.line_endcap_aa_region_width = 1;
   if (brw->attribs.Line->SmoothFlag)
      sf.sf6.aa_enable = 1;
   else if (sf.sf6.line_width <= 0x2) 
       sf.sf6.line_width = 0; 

   /* _NEW_POINT */
   sf.sf6.point_rast_rule = 1;	/* opengl conventions */
   /* XXX clamp max depends on AA vs. non-AA */

   sf.sf7.sprite_point = brw->attribs.Point->PointSprite;
   sf.sf7.point_size = CLAMP(brw->attribs.Point->Size, 1.0, 255.0) * (1<<3);
   sf.sf7.use_point_size_state = !brw->attribs.Point->_Attenuated;
      
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

   reloc_bufs[0] = brw->sf.prog_bo;
   reloc_bufs[1] = brw->sf.vp_bo;

   brw->sf.thread0_delta = sf.thread0.grf_reg_count << 1;
   brw->sf.sf5_delta = sf.sf5.front_winding | (sf.sf5.viewport_transform << 1);

   dri_bo_unreference(brw->sf.state_bo);
   brw->sf.state_bo = brw_cache_data( &brw->cache, BRW_SF_UNIT, &sf,
				      reloc_bufs, 2 );
}

static void emit_reloc_sf_unit(struct brw_context *brw)
{
   /* Emit SF program relocation */
   dri_emit_reloc(brw->sf.state_bo,
		  DRM_BO_FLAG_MEM_TT | DRM_BO_FLAG_READ,
		  brw->sf.thread0_delta,
		  offsetof(struct brw_sf_unit_state, thread0),
		  brw->sf.prog_bo);

   /* Emit SF viewport relocation */
   dri_emit_reloc(brw->sf.state_bo,
		  DRM_BO_FLAG_MEM_TT | DRM_BO_FLAG_READ,
		  brw->sf.sf5_delta,
		  offsetof(struct brw_sf_unit_state, sf5),
		  brw->sf.vp_bo);
}

const struct brw_tracked_state brw_sf_unit = {
   .dirty = {
      .mesa  = (_NEW_POLYGON | 
		_NEW_LINE | 
		_NEW_POINT | 
		_NEW_SCISSOR),
      .brw   = (BRW_NEW_URB_FENCE |
		BRW_NEW_METAOPS),
      .cache = (CACHE_NEW_SF_VP |
		CACHE_NEW_SF_PROG)
   },
   .update = upload_sf_unit,
   .emit_reloc = emit_reloc_sf_unit,
};
