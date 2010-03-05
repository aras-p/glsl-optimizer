/*
 * Copyright © 2009 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Authors:
 *    Eric Anholt <eric@anholt.net>
 *
 */

#include "brw_context.h"
#include "brw_state.h"
#include "brw_defines.h"
#include "brw_util.h"
#include "intel_batchbuffer.h"
#include "main/macros.h"

struct gen6_blend_state_key {
   GLboolean color_blend, alpha_enabled;
   GLboolean dither;

   GLenum logic_op;

   GLenum blend_eq_rgb, blend_eq_a;
   GLenum blend_src_rgb, blend_src_a;
   GLenum blend_dst_rgb, blend_dst_a;

   GLenum alpha_func;
};

static void
blend_state_populate_key(struct brw_context *brw,
			 struct gen6_blend_state_key *key)
{
   GLcontext *ctx = &brw->intel.ctx;

   memset(key, 0, sizeof(*key));

   /* _NEW_COLOR */
   if (ctx->Color._LogicOpEnabled)
      key->logic_op = ctx->Color.LogicOp;
   else
      key->logic_op = GL_COPY;

   /* _NEW_COLOR */
   key->color_blend = ctx->Color.BlendEnabled;
   if (key->color_blend) {
      key->blend_eq_rgb = ctx->Color.BlendEquationRGB;
      key->blend_eq_a = ctx->Color.BlendEquationA;
      key->blend_src_rgb = ctx->Color.BlendSrcRGB;
      key->blend_dst_rgb = ctx->Color.BlendDstRGB;
      key->blend_src_a = ctx->Color.BlendSrcA;
      key->blend_dst_a = ctx->Color.BlendDstA;
   }

   /* _NEW_COLOR */
   key->alpha_enabled = ctx->Color.AlphaEnabled;
   if (key->alpha_enabled) {
      key->alpha_func = ctx->Color.AlphaFunc;
   }

   /* _NEW_COLOR */
   key->dither = ctx->Color.DitherFlag;
}

/**
 * Creates the state cache entry for the given CC unit key.
 */
static drm_intel_bo *
blend_state_create_from_key(struct brw_context *brw,
			    struct gen6_blend_state_key *key)
{
   struct gen6_blend_state blend;
   drm_intel_bo *bo;

   memset(&blend, 0, sizeof(blend));

   if (key->logic_op != GL_COPY) {
      blend.blend1.logic_op_enable = 1;
      blend.blend1.logic_op_func = intel_translate_logic_op(key->logic_op);
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

      blend.blend0.dest_blend_factor = brw_translate_blend_factor(dstRGB);
      blend.blend0.source_blend_factor = brw_translate_blend_factor(srcRGB);
      blend.blend0.blend_func = brw_translate_blend_equation(eqRGB);

      blend.blend0.ia_dest_blend_factor = brw_translate_blend_factor(dstA);
      blend.blend0.ia_source_blend_factor = brw_translate_blend_factor(srcA);
      blend.blend0.ia_blend_func = brw_translate_blend_equation(eqA);

      blend.blend0.blend_enable = 1;
      blend.blend0.ia_blend_enable = (srcA != srcRGB ||
				      dstA != dstRGB ||
				      eqA != eqRGB);
   }

   if (key->alpha_enabled) {
      blend.blend1.alpha_test_enable = 1;
      blend.blend1.alpha_test_func = intel_translate_compare_func(key->alpha_func);

   }

   if (key->dither) {
      blend.blend1.dither_enable = 1;
      blend.blend1.y_dither_offset = 0;
      blend.blend1.x_dither_offset = 0;
   }

   bo = brw_upload_cache(&brw->cache, BRW_BLEND_STATE,
			 key, sizeof(*key),
			 NULL, 0,
			 &blend, sizeof(blend));

   return bo;
}

static void
prepare_blend_state(struct brw_context *brw)
{
   struct gen6_blend_state_key key;

   blend_state_populate_key(brw, &key);

   drm_intel_bo_unreference(brw->cc.blend_state_bo);
   brw->cc.blend_state_bo = brw_search_cache(&brw->cache, BRW_BLEND_STATE,
					     &key, sizeof(key),
					     NULL, 0,
					     NULL);

   if (brw->cc.blend_state_bo == NULL)
      brw->cc.blend_state_bo = blend_state_create_from_key(brw, &key);
}

const struct brw_tracked_state gen6_blend_state = {
   .dirty = {
      .mesa = _NEW_COLOR,
      .brw = 0,
      .cache = 0,
   },
   .prepare = prepare_blend_state,
};

struct gen6_color_calc_state_key {
   GLubyte blend_constant_color[4];
   GLclampf alpha_ref;
   GLubyte stencil_ref[2];
};

static void
color_calc_state_populate_key(struct brw_context *brw,
			      struct gen6_color_calc_state_key *key)
{
   GLcontext *ctx = &brw->intel.ctx;

   memset(key, 0, sizeof(*key));

   /* _NEW_STENCIL */
   if (ctx->Stencil._Enabled) {
      const unsigned back = ctx->Stencil._BackFace;

      key->stencil_ref[0] = ctx->Stencil.Ref[0];
      if (ctx->Stencil._TestTwoSide)
	 key->stencil_ref[1] = ctx->Stencil.Ref[back];
   }

   /* _NEW_COLOR */
   if (ctx->Color.AlphaEnabled)
      key->alpha_ref = ctx->Color.AlphaRef;

   key->blend_constant_color[0] = ctx->Color.BlendColor[0];
   key->blend_constant_color[1] = ctx->Color.BlendColor[1];
   key->blend_constant_color[2] = ctx->Color.BlendColor[2];
   key->blend_constant_color[3] = ctx->Color.BlendColor[3];
}

/**
 * Creates the state cache entry for the given CC state key.
 */
static drm_intel_bo *
color_calc_state_create_from_key(struct brw_context *brw,
				 struct gen6_color_calc_state_key *key)
{
   struct gen6_color_calc_state cc;
   drm_intel_bo *bo;

   memset(&cc, 0, sizeof(cc));

   cc.cc0.alpha_test_format = BRW_ALPHATEST_FORMAT_UNORM8;
   UNCLAMPED_FLOAT_TO_UBYTE(cc.cc1.alpha_ref_fi.ui, key->alpha_ref);

   cc.cc0.stencil_ref = key->stencil_ref[0];
   cc.cc0.bf_stencil_ref = key->stencil_ref[1];

   cc.constant_r = key->blend_constant_color[0];
   cc.constant_g = key->blend_constant_color[1];
   cc.constant_b = key->blend_constant_color[2];
   cc.constant_a = key->blend_constant_color[3];

   bo = brw_upload_cache(&brw->cache, BRW_COLOR_CALC_STATE,
			 key, sizeof(*key),
			 NULL, 0,
			 &cc, sizeof(cc));

   return bo;
}

static void
prepare_color_calc_state(struct brw_context *brw)
{
   struct gen6_color_calc_state_key key;

   color_calc_state_populate_key(brw, &key);

   drm_intel_bo_unreference(brw->cc.state_bo);
   brw->cc.state_bo = brw_search_cache(&brw->cache, BRW_COLOR_CALC_STATE,
				       &key, sizeof(key),
				       NULL, 0,
				       NULL);

   if (brw->cc.state_bo == NULL)
      brw->cc.state_bo = color_calc_state_create_from_key(brw, &key);
}

const struct brw_tracked_state gen6_color_calc_state = {
   .dirty = {
      .mesa = _NEW_COLOR,
      .brw = 0,
      .cache = 0,
   },
   .prepare = prepare_color_calc_state,
};

static void upload_cc_state_pointers(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;

   BEGIN_BATCH(4);
   OUT_BATCH(CMD_3D_CC_STATE_POINTERS << 16 | (4 - 2));
   OUT_RELOC(brw->cc.state_bo, I915_GEM_DOMAIN_INSTRUCTION, 0, 1);
   OUT_RELOC(brw->cc.blend_state_bo, I915_GEM_DOMAIN_INSTRUCTION, 0, 1);
   OUT_RELOC(brw->cc.depth_stencil_state_bo, I915_GEM_DOMAIN_INSTRUCTION, 0, 1);
   ADVANCE_BATCH();

   intel_batchbuffer_emit_mi_flush(intel->batch);
}


static void prepare_cc_state_pointers(struct brw_context *brw)
{
   brw_add_validated_bo(brw, brw->cc.state_bo);
   brw_add_validated_bo(brw, brw->cc.blend_state_bo);
   brw_add_validated_bo(brw, brw->cc.depth_stencil_state_bo);
}

const struct brw_tracked_state gen6_cc_state_pointers = {
   .dirty = {
      .mesa = 0,
      .brw = BRW_NEW_BATCH,
      .cache = (CACHE_NEW_BLEND_STATE |
		CACHE_NEW_COLOR_CALC_STATE |
		CACHE_NEW_DEPTH_STENCIL_STATE)
   },
   .prepare = prepare_cc_state_pointers,
   .emit = upload_cc_state_pointers,
};
