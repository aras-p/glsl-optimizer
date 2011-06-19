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

static void
prepare_blend_state(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->intel.ctx;
   struct gen6_blend_state *blend;
   int b;
   int nr_draw_buffers = ctx->DrawBuffer->_NumColorDrawBuffers;
   int size;

   /* We need at least one BLEND_STATE written, because we might do
    * thread dispatch even if _NumColorDrawBuffers is 0 (for example
    * for computed depth or alpha test), which will do an FB write
    * with render target 0, which will reference BLEND_STATE[0] for
    * alpha test enable.
    */
   if (nr_draw_buffers == 0 && ctx->Color.AlphaEnabled)
      nr_draw_buffers = 1;

   size = sizeof(*blend) * nr_draw_buffers;
   blend = brw_state_batch(brw, size, 64, &brw->cc.blend_state_offset);

   memset(blend, 0, size);

   for (b = 0; b < nr_draw_buffers; b++) {
      /* _NEW_COLOR */
      if (ctx->Color._LogicOpEnabled) {
	 struct gl_renderbuffer *rb = ctx->DrawBuffer->_ColorDrawBuffers[b];
	 /* _NEW_BUFFERS */
	 /* Floating point RTs should have no effect from LogicOp,
	  * except for disabling of blending
	  */
	 if (_mesa_get_format_datatype(rb->Format) != GL_FLOAT) {
	    blend[b].blend1.logic_op_enable = 1;
	    blend[b].blend1.logic_op_func =
	       intel_translate_logic_op(ctx->Color.LogicOp);
	 }
      } else if (ctx->Color.BlendEnabled & (1 << b)) {
	 GLenum eqRGB = ctx->Color.Blend[0].EquationRGB;
	 GLenum eqA = ctx->Color.Blend[0].EquationA;
	 GLenum srcRGB = ctx->Color.Blend[0].SrcRGB;
	 GLenum dstRGB = ctx->Color.Blend[0].DstRGB;
	 GLenum srcA = ctx->Color.Blend[0].SrcA;
	 GLenum dstA = ctx->Color.Blend[0].DstA;

	 if (eqRGB == GL_MIN || eqRGB == GL_MAX) {
	    srcRGB = dstRGB = GL_ONE;
	 }

	 if (eqA == GL_MIN || eqA == GL_MAX) {
	    srcA = dstA = GL_ONE;
	 }

	 blend[b].blend0.dest_blend_factor = brw_translate_blend_factor(dstRGB);
	 blend[b].blend0.source_blend_factor = brw_translate_blend_factor(srcRGB);
	 blend[b].blend0.blend_func = brw_translate_blend_equation(eqRGB);

	 blend[b].blend0.ia_dest_blend_factor = brw_translate_blend_factor(dstA);
	 blend[b].blend0.ia_source_blend_factor = brw_translate_blend_factor(srcA);
	 blend[b].blend0.ia_blend_func = brw_translate_blend_equation(eqA);

	 blend[b].blend0.blend_enable = 1;
	 blend[b].blend0.ia_blend_enable = (srcA != srcRGB ||
					 dstA != dstRGB ||
					 eqA != eqRGB);
      }


      /* _NEW_COLOR */
      if (ctx->Color.AlphaEnabled) {
	 blend[b].blend1.alpha_test_enable = 1;
	 blend[b].blend1.alpha_test_func =
	    intel_translate_compare_func(ctx->Color.AlphaFunc);

      }

      /* _NEW_COLOR */
      if (ctx->Color.DitherFlag) {
	 blend[b].blend1.dither_enable = 1;
	 blend[b].blend1.y_dither_offset = 0;
	 blend[b].blend1.x_dither_offset = 0;
      }

      blend[b].blend1.write_disable_r = !ctx->Color.ColorMask[b][0];
      blend[b].blend1.write_disable_g = !ctx->Color.ColorMask[b][1];
      blend[b].blend1.write_disable_b = !ctx->Color.ColorMask[b][2];
      blend[b].blend1.write_disable_a = !ctx->Color.ColorMask[b][3];
   }

   brw->state.dirty.cache |= CACHE_NEW_BLEND_STATE;
}

const struct brw_tracked_state gen6_blend_state = {
   .dirty = {
      .mesa = (_NEW_COLOR |
	       _NEW_BUFFERS),
      .brw = BRW_NEW_BATCH,
      .cache = 0,
   },
   .prepare = prepare_blend_state,
};

static void
gen6_prepare_color_calc_state(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->intel.ctx;
   struct gen6_color_calc_state *cc;

   cc = brw_state_batch(brw, sizeof(*cc), 64, &brw->cc.state_offset);
   memset(cc, 0, sizeof(*cc));

   /* _NEW_COLOR */
   cc->cc0.alpha_test_format = BRW_ALPHATEST_FORMAT_UNORM8;
   UNCLAMPED_FLOAT_TO_UBYTE(cc->cc1.alpha_ref_fi.ui, ctx->Color.AlphaRef);

   /* _NEW_STENCIL */
   cc->cc0.stencil_ref = ctx->Stencil.Ref[0];
   cc->cc0.bf_stencil_ref = ctx->Stencil.Ref[ctx->Stencil._BackFace];

   /* _NEW_COLOR */
   cc->constant_r = ctx->Color.BlendColorUnclamped[0];
   cc->constant_g = ctx->Color.BlendColorUnclamped[1];
   cc->constant_b = ctx->Color.BlendColorUnclamped[2];
   cc->constant_a = ctx->Color.BlendColorUnclamped[3];

   brw->state.dirty.cache |= CACHE_NEW_COLOR_CALC_STATE;
}

const struct brw_tracked_state gen6_color_calc_state = {
   .dirty = {
      .mesa = _NEW_COLOR | _NEW_STENCIL,
      .brw = BRW_NEW_BATCH,
      .cache = 0,
   },
   .prepare = gen6_prepare_color_calc_state,
};

static void upload_cc_state_pointers(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;

   BEGIN_BATCH(4);
   OUT_BATCH(_3DSTATE_CC_STATE_POINTERS << 16 | (4 - 2));
   OUT_BATCH(brw->cc.blend_state_offset | 1);
   OUT_BATCH(brw->cc.depth_stencil_state_offset | 1);
   OUT_BATCH(brw->cc.state_offset | 1);
   ADVANCE_BATCH();
}

const struct brw_tracked_state gen6_cc_state_pointers = {
   .dirty = {
      .mesa = 0,
      .brw = (BRW_NEW_BATCH |
	      BRW_NEW_STATE_BASE_ADDRESS),
      .cache = (CACHE_NEW_BLEND_STATE |
		CACHE_NEW_COLOR_CALC_STATE |
		CACHE_NEW_DEPTH_STENCIL_STATE)
   },
   .emit = upload_cc_state_pointers,
};
