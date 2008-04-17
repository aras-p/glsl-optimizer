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
#include "brw_util.h"
#include "macros.h"
#include "enums.h"

static int upload_cc_vp( struct brw_context *brw )
{
   struct brw_cc_viewport ccv;

   memset(&ccv, 0, sizeof(ccv));

   ccv.min_depth = 0.0;
   ccv.max_depth = 1.0;

   dri_bo_unreference(brw->cc.vp_bo);
   brw->cc.vp_bo = brw_cache_data( &brw->cache, BRW_CC_VP, &ccv, NULL, 0 );
   return dri_bufmgr_check_aperture_space(brw->cc.vp_bo);
}

const struct brw_tracked_state brw_cc_vp = {
   .dirty = {
      .mesa = 0,
      .brw = BRW_NEW_CONTEXT,
      .cache = 0
   },
   .prepare = upload_cc_vp
};

struct brw_cc_unit_key {
   GLboolean stencil, stencil_two_side, color_blend, alpha_enabled;

   GLenum stencil_func[2], stencil_fail_op[2];
   GLenum stencil_pass_depth_fail_op[2], stencil_pass_depth_pass_op[2];
   GLubyte stencil_ref[2], stencil_write_mask[2], stencil_test_mask[2];
   GLenum logic_op;

   GLenum blend_eq_rgb, blend_eq_a;
   GLenum blend_src_rgb, blend_src_a;
   GLenum blend_dst_rgb, blend_dst_a;

   GLenum alpha_func;
   GLclampf alpha_ref;

   GLboolean dither;

   GLboolean depth_test, depth_write;
   GLenum depth_func;
};

static void
cc_unit_populate_key(struct brw_context *brw, struct brw_cc_unit_key *key)
{
   struct gl_stencil_attrib *stencil = brw->attribs.Stencil;

   memset(key, 0, sizeof(*key));

   key->stencil = stencil->Enabled;
   key->stencil_two_side = stencil->_TestTwoSide;

   if (key->stencil) {
      key->stencil_func[0] = stencil->Function[0];
      key->stencil_fail_op[0] = stencil->FailFunc[0];
      key->stencil_pass_depth_fail_op[0] = stencil->ZFailFunc[0];
      key->stencil_pass_depth_pass_op[0] = stencil->ZPassFunc[0];
      key->stencil_ref[0] = stencil->Ref[0];
      key->stencil_write_mask[0] = stencil->WriteMask[0];
      key->stencil_test_mask[0] = stencil->ValueMask[0];
   }
   if (key->stencil_two_side) {
      key->stencil_func[1] = stencil->Function[1];
      key->stencil_fail_op[1] = stencil->FailFunc[1];
      key->stencil_pass_depth_fail_op[1] = stencil->ZFailFunc[1];
      key->stencil_pass_depth_pass_op[1] = stencil->ZPassFunc[1];
      key->stencil_ref[1] = stencil->Ref[1];
      key->stencil_write_mask[1] = stencil->WriteMask[1];
      key->stencil_test_mask[1] = stencil->ValueMask[1];
   }

   if (brw->attribs.Color->_LogicOpEnabled)
      key->logic_op = brw->attribs.Color->LogicOp;
   else
      key->logic_op = GL_COPY;

   key->color_blend = brw->attribs.Color->BlendEnabled;
   if (key->color_blend) {
      key->blend_eq_rgb = brw->attribs.Color->BlendEquationRGB;
      key->blend_eq_a = brw->attribs.Color->BlendEquationA;
      key->blend_src_rgb = brw->attribs.Color->BlendSrcRGB;
      key->blend_dst_rgb = brw->attribs.Color->BlendDstRGB;
      key->blend_src_a = brw->attribs.Color->BlendSrcA;
      key->blend_dst_a = brw->attribs.Color->BlendDstA;
   }

   key->alpha_enabled = brw->attribs.Color->AlphaEnabled;
   if (key->alpha_enabled) {
      key->alpha_func = brw->attribs.Color->AlphaFunc;
      key->alpha_ref = brw->attribs.Color->AlphaRef;
   }

   key->dither = brw->attribs.Color->DitherFlag;

   key->depth_test = brw->attribs.Depth->Test;
   if (key->depth_test) {
      key->depth_func = brw->attribs.Depth->Func;
      key->depth_write = brw->attribs.Depth->Mask;
   }
}

/**
 * Creates the state cache entry for the given CC unit key.
 */
static dri_bo *
cc_unit_create_from_key(struct brw_context *brw, struct brw_cc_unit_key *key)
{
   struct brw_cc_unit_state cc;
   dri_bo *bo;

   memset(&cc, 0, sizeof(cc));

   /* _NEW_STENCIL */
   if (key->stencil) {
      cc.cc0.stencil_enable = 1;
      cc.cc0.stencil_func =
	 intel_translate_compare_func(key->stencil_func[0]);
      cc.cc0.stencil_fail_op =
	 intel_translate_stencil_op(key->stencil_fail_op[0]);
      cc.cc0.stencil_pass_depth_fail_op =
	 intel_translate_stencil_op(key->stencil_pass_depth_fail_op[0]);
      cc.cc0.stencil_pass_depth_pass_op =
	 intel_translate_stencil_op(key->stencil_pass_depth_pass_op[0]);
      cc.cc1.stencil_ref = key->stencil_ref[0];
      cc.cc1.stencil_write_mask = key->stencil_write_mask[0];
      cc.cc1.stencil_test_mask = key->stencil_test_mask[0];

      if (key->stencil_two_side) {
	 cc.cc0.bf_stencil_enable = 1;
	 cc.cc0.bf_stencil_func =
	    intel_translate_compare_func(key->stencil_func[1]);
	 cc.cc0.bf_stencil_fail_op =
	    intel_translate_stencil_op(key->stencil_fail_op[1]);
	 cc.cc0.bf_stencil_pass_depth_fail_op =
	    intel_translate_stencil_op(key->stencil_pass_depth_fail_op[1]);
	 cc.cc0.bf_stencil_pass_depth_pass_op =
	    intel_translate_stencil_op(key->stencil_pass_depth_pass_op[1]);
	 cc.cc1.bf_stencil_ref = key->stencil_ref[1];
	 cc.cc2.bf_stencil_write_mask = key->stencil_write_mask[1];
	 cc.cc2.bf_stencil_test_mask = key->stencil_test_mask[1];
      }

      /* Not really sure about this:
       */
      if (key->stencil_write_mask[0] ||
	  (key->stencil_two_side && key->stencil_write_mask[1]))
	 cc.cc0.stencil_write_enable = 1;
   }

   /* _NEW_COLOR */
   if (key->logic_op != GL_COPY) {
      cc.cc2.logicop_enable = 1;
      cc.cc5.logicop_func = intel_translate_logic_op(key->logic_op);
   } else if (key->color_blend) {
      GLenum eqRGB = key->blend_eq_rgb;
      GLenum eqA = key->blend_eq_a;
      GLenum srcRGB = key->blend_src_rgb;
      GLenum dstRGB = key->blend_dst_rgb;
      GLenum srcA = key->blend_src_a;
      GLenum dstA = key->blend_dst_a;

      if (eqRGB == GL_MIN || eqRGB == GL_MAX) {
	 srcRGB = dstRGB = GL_ONE;
      }

      if (eqA == GL_MIN || eqA == GL_MAX) {
	 srcA = dstA = GL_ONE;
      }

      cc.cc6.dest_blend_factor = brw_translate_blend_factor(dstRGB);
      cc.cc6.src_blend_factor = brw_translate_blend_factor(srcRGB);
      cc.cc6.blend_function = brw_translate_blend_equation(eqRGB);

      cc.cc5.ia_dest_blend_factor = brw_translate_blend_factor(dstA);
      cc.cc5.ia_src_blend_factor = brw_translate_blend_factor(srcA);
      cc.cc5.ia_blend_function = brw_translate_blend_equation(eqA);

      cc.cc3.blend_enable = 1;
      cc.cc3.ia_blend_enable = (srcA != srcRGB ||
				dstA != dstRGB ||
				eqA != eqRGB);
   }

   if (key->alpha_enabled) {
      cc.cc3.alpha_test = 1;
      cc.cc3.alpha_test_func = intel_translate_compare_func(key->alpha_func);
      cc.cc3.alpha_test_format = BRW_ALPHATEST_FORMAT_UNORM8;

      UNCLAMPED_FLOAT_TO_UBYTE(cc.cc7.alpha_ref.ub[0], key->alpha_ref);
   }

   if (key->dither) {
      cc.cc5.dither_enable = 1;
      cc.cc6.y_dither_offset = 0;
      cc.cc6.x_dither_offset = 0;
   }

   /* _NEW_DEPTH */
   if (key->depth_test) {
      cc.cc2.depth_test = 1;
      cc.cc2.depth_test_function = intel_translate_compare_func(key->depth_func);
      cc.cc2.depth_write_enable = key->depth_write;
   }

   /* CACHE_NEW_CC_VP */
   cc.cc4.cc_viewport_state_offset = brw->cc.vp_bo->offset >> 5; /* reloc */

   if (INTEL_DEBUG & DEBUG_STATS)
      cc.cc5.statistics_enable = 1;

   bo = brw_upload_cache(&brw->cache, BRW_CC_UNIT,
			 key, sizeof(*key),
			 &brw->cc.vp_bo, 1,
			 &cc, sizeof(cc),
			 NULL, NULL);

   /* Emit CC viewport relocation */
   dri_emit_reloc(bo,
		  DRM_BO_FLAG_MEM_TT | DRM_BO_FLAG_READ,
		  0,
		  offsetof(struct brw_cc_unit_state, cc4),
		  brw->cc.vp_bo);

   return bo;
}

static int prepare_cc_unit( struct brw_context *brw )
{
   struct brw_cc_unit_key key;

   cc_unit_populate_key(brw, &key);

   dri_bo_unreference(brw->cc.state_bo);
   brw->cc.state_bo = brw_search_cache(&brw->cache, BRW_CC_UNIT,
				       &key, sizeof(key),
				       &brw->cc.vp_bo, 1,
				       NULL);

   if (brw->cc.state_bo == NULL)
      brw->cc.state_bo = cc_unit_create_from_key(brw, &key);
   return dri_bufmgr_check_aperture_space(brw->cc.state_bo);
}

const struct brw_tracked_state brw_cc_unit = {
   .dirty = {
      .mesa = _NEW_STENCIL | _NEW_COLOR | _NEW_DEPTH,
      .brw = 0,
      .cache = CACHE_NEW_CC_VP
   },
   .prepare = prepare_cc_unit,
};



