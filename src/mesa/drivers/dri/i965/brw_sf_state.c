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
#include "main/macros.h"
#include "intel_fbo.h"

static void upload_sf_vp(struct brw_context *brw)
{
   GLcontext *ctx = &brw->intel.ctx;
   const GLfloat depth_scale = 1.0F / ctx->DrawBuffer->_DepthMaxF;
   struct brw_sf_viewport sfv;
   struct intel_renderbuffer *irb =
      intel_renderbuffer(ctx->DrawBuffer->_ColorDrawBuffers[0]);
   GLfloat y_scale, y_bias;

   memset(&sfv, 0, sizeof(sfv));

   if (ctx->DrawBuffer->Name) {
      /* User-created FBO */
      if (irb && !irb->RenderToTexture) {
	 y_scale = -1.0;
	 y_bias = ctx->DrawBuffer->Height;
      } else {
	 y_scale = 1.0;
	 y_bias = 0;
      }
   } else {
      y_scale = -1.0;
      y_bias = ctx->DrawBuffer->Height;
   }

   /* _NEW_VIEWPORT, BRW_NEW_METAOPS */

   if (!brw->metaops.active) {
      const GLfloat *v = ctx->Viewport._WindowMap.m;

      sfv.viewport.m00 = v[MAT_SX];
      sfv.viewport.m11 = v[MAT_SY] * y_scale;
      sfv.viewport.m22 = v[MAT_SZ] * depth_scale;
      sfv.viewport.m30 = v[MAT_TX];
      sfv.viewport.m31 = v[MAT_TY] * y_scale + y_bias;
      sfv.viewport.m32 = v[MAT_TZ] * depth_scale;
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
   .prepare = upload_sf_vp
};

struct brw_sf_unit_key {
   unsigned int total_grf;
   unsigned int urb_entry_read_length;

   unsigned int nr_urb_entries, urb_size, sfsize;

   GLenum front_face, cull_face;
   GLboolean scissor, line_smooth, point_sprite, point_attenuated;
   float line_width;
   float point_size;
};

static void
sf_unit_populate_key(struct brw_context *brw, struct brw_sf_unit_key *key)
{
   memset(key, 0, sizeof(*key));

   /* CACHE_NEW_SF_PROG */
   key->total_grf = brw->sf.prog_data->total_grf;
   key->urb_entry_read_length = brw->sf.prog_data->urb_read_length;

   /* BRW_NEW_URB_FENCE */
   key->nr_urb_entries = brw->urb.nr_sf_entries;
   key->urb_size = brw->urb.vsize;
   key->sfsize = brw->urb.sfsize;

   key->scissor = brw->attribs.Scissor->Enabled;
   key->front_face = brw->attribs.Polygon->FrontFace;

   if (brw->attribs.Polygon->CullFlag)
      key->cull_face = brw->attribs.Polygon->CullFaceMode;
   else
      key->cull_face = GL_NONE;

   key->line_width = brw->attribs.Line->Width;
   key->line_smooth = brw->attribs.Line->SmoothFlag;

   key->point_sprite = brw->attribs.Point->PointSprite;
   key->point_size = brw->attribs.Point->Size;
   key->point_attenuated = brw->attribs.Point->_Attenuated;
}

static dri_bo *
sf_unit_create_from_key(struct brw_context *brw, struct brw_sf_unit_key *key,
			dri_bo **reloc_bufs)
{
   struct brw_sf_unit_state sf;
   dri_bo *bo;

   memset(&sf, 0, sizeof(sf));

   sf.thread0.grf_reg_count = ALIGN(key->total_grf, 16) / 16 - 1;
   sf.thread0.kernel_start_pointer = brw->sf.prog_bo->offset >> 6; /* reloc */

   sf.thread1.floating_point_mode = BRW_FLOATING_POINT_NON_IEEE_754;

   sf.thread3.dispatch_grf_start_reg = 3;
   sf.thread3.urb_entry_read_offset = 1;
   sf.thread3.urb_entry_read_length = key->urb_entry_read_length;

   sf.thread4.nr_urb_entries = key->nr_urb_entries;
   sf.thread4.urb_entry_allocation_size = key->sfsize - 1;
   /* Each SF thread produces 1 PUE, and there can be up to 24 threads */
   sf.thread4.max_threads = MIN2(24, key->nr_urb_entries) - 1;

   if (INTEL_DEBUG & DEBUG_SINGLE_THREAD)
      sf.thread4.max_threads = 0;

   if (INTEL_DEBUG & DEBUG_STATS)
      sf.thread4.stats_enable = 1;

   /* CACHE_NEW_SF_VP */
   sf.sf5.sf_viewport_state_offset = brw->sf.vp_bo->offset >> 5; /* reloc */

   sf.sf5.viewport_transform = 1;

   /* _NEW_SCISSOR */
   if (key->scissor)
      sf.sf6.scissor = 1;

   /* _NEW_POLYGON */
   if (key->front_face == GL_CCW)
      sf.sf5.front_winding = BRW_FRONTWINDING_CCW;
   else
      sf.sf5.front_winding = BRW_FRONTWINDING_CW;

   switch (key->cull_face) {
   case GL_FRONT:
      sf.sf6.cull_mode = BRW_CULLMODE_FRONT;
      break;
   case GL_BACK:
      sf.sf6.cull_mode = BRW_CULLMODE_BACK;
      break;
   case GL_FRONT_AND_BACK:
      sf.sf6.cull_mode = BRW_CULLMODE_BOTH;
      break;
   case GL_NONE:
      sf.sf6.cull_mode = BRW_CULLMODE_NONE;
      break;
   default:
      assert(0);
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

   /* _NEW_POINT */
   sf.sf6.point_rast_rule = BRW_RASTRULE_UPPER_RIGHT;	/* opengl conventions */
   /* XXX clamp max depends on AA vs. non-AA */

   sf.sf7.sprite_point = key->point_sprite;
   sf.sf7.point_size = CLAMP(nearbyint(key->point_size), 1, 255) * (1<<3);
   sf.sf7.use_point_size_state = !key->point_attenuated;
   sf.sf7.aa_line_distance_mode = 0;

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

   bo = brw_upload_cache(&brw->cache, BRW_SF_UNIT,
			 key, sizeof(*key),
			 reloc_bufs, 2,
			 &sf, sizeof(sf),
			 NULL, NULL);

   /* Emit SF program relocation */
   dri_bo_emit_reloc(bo,
		     I915_GEM_DOMAIN_INSTRUCTION, 0,
		     sf.thread0.grf_reg_count << 1,
		     offsetof(struct brw_sf_unit_state, thread0),
		     brw->sf.prog_bo);

   /* Emit SF viewport relocation */
   dri_bo_emit_reloc(bo,
		     I915_GEM_DOMAIN_INSTRUCTION, 0,
		     sf.sf5.front_winding | (sf.sf5.viewport_transform << 1),
		     offsetof(struct brw_sf_unit_state, sf5),
		     brw->sf.vp_bo);

   return bo;
}

static void upload_sf_unit( struct brw_context *brw )
{
   struct brw_sf_unit_key key;
   dri_bo *reloc_bufs[2];

   sf_unit_populate_key(brw, &key);

   reloc_bufs[0] = brw->sf.prog_bo;
   reloc_bufs[1] = brw->sf.vp_bo;

   dri_bo_unreference(brw->sf.state_bo);
   brw->sf.state_bo = brw_search_cache(&brw->cache, BRW_SF_UNIT,
				       &key, sizeof(key),
				       reloc_bufs, 2,
				       NULL);
   if (brw->sf.state_bo == NULL) {
      brw->sf.state_bo = sf_unit_create_from_key(brw, &key, reloc_bufs);
   }
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
   .prepare = upload_sf_unit,
};
