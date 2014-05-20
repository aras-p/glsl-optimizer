/*
 * Copyright © 2012 Intel Corporation
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
 */

#include "brw_context.h"
#include "brw_state.h"
#include "brw_defines.h"
#include "brw_util.h"
#include "brw_wm.h"
#include "intel_batchbuffer.h"
#include "main/macros.h"
#include "main/enums.h"
#include "main/glformats.h"

#define blend_factor(x) brw_translate_blend_factor(x)
#define blend_eqn(x) brw_translate_blend_equation(x)

static void
gen8_upload_blend_state(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;

   /* We need at least one BLEND_STATE written, because we might do
    * thread dispatch even if _NumColorDrawBuffers is 0 (for example
    * for computed depth or alpha test), which will do an FB write
    * with render target 0, which will reference BLEND_STATE[0] for
    * alpha test enable.
    */
   int nr_draw_buffers = ctx->DrawBuffer->_NumColorDrawBuffers;
   if (nr_draw_buffers == 0 && ctx->Color.AlphaEnabled)
      nr_draw_buffers = 1;

   int size = 4 + 8 * nr_draw_buffers;
   uint32_t *blend = brw_state_batch(brw, AUB_TRACE_BLEND_STATE,
                                     size, 64, &brw->cc.blend_state_offset);
   memset(blend, 0, size);

   /* OpenGL specification 3.3 (page 196), section 4.1.3 says:
    * "If drawbuffer zero is not NONE and the buffer it references has an
    * integer format, the SAMPLE_ALPHA_TO_COVERAGE and SAMPLE_ALPHA_TO_ONE
    * operations are skipped."
    */
   struct gl_renderbuffer *rb0 = ctx->DrawBuffer->_ColorDrawBuffers[0];
   GLenum rb_zero_type =
      rb0 ? _mesa_get_format_datatype(rb0->Format) : GL_UNSIGNED_NORMALIZED;

   if (rb_zero_type != GL_INT && rb_zero_type != GL_UNSIGNED_INT) {
      /* _NEW_MULTISAMPLE */
      if (ctx->Multisample._Enabled) {
         if (ctx->Multisample.SampleAlphaToCoverage) {
            blend[0] |= GEN8_BLEND_ALPHA_TO_COVERAGE_ENABLE;
            blend[0] |= GEN8_BLEND_ALPHA_TO_COVERAGE_DITHER_ENABLE;
         }
         if (ctx->Multisample.SampleAlphaToOne)
            blend[0] |= GEN8_BLEND_ALPHA_TO_ONE_ENABLE;
      }

      /* _NEW_COLOR */
      if (ctx->Color.AlphaEnabled) {
         blend[0] |=
            GEN8_BLEND_ALPHA_TEST_ENABLE |
            SET_FIELD(intel_translate_compare_func(ctx->Color.AlphaFunc),
                      GEN8_BLEND_ALPHA_TEST_FUNCTION);
      }

      if (ctx->Color.DitherFlag) {
         blend[0] |= GEN8_BLEND_COLOR_DITHER_ENABLE;
      }
   }

   for (int i = 0; i < nr_draw_buffers; i++) {
      /* _NEW_BUFFERS */
      struct gl_renderbuffer *rb = ctx->DrawBuffer->_ColorDrawBuffers[i];
      GLenum rb_type =
         rb ? _mesa_get_format_datatype(rb->Format) : GL_UNSIGNED_NORMALIZED;

      /* Used for implementing the following bit of GL_EXT_texture_integer:
       * "Per-fragment operations that require floating-point color
       *  components, including multisample alpha operations, alpha test,
       *  blending, and dithering, have no effect when the corresponding
       *  colors are written to an integer color buffer."
      */
      bool integer = rb_type == GL_INT || rb_type == GL_UNSIGNED_INT;

      /* _NEW_COLOR */
      if (ctx->Color.ColorLogicOpEnabled) {
         blend[1 + 2*i+1] |=
            GEN8_BLEND_LOGIC_OP_ENABLE |
            SET_FIELD(intel_translate_logic_op(ctx->Color.LogicOp),
                      GEN8_BLEND_LOGIC_OP_FUNCTION);
      } else if (ctx->Color.BlendEnabled & (1 << i) && !integer) {
         GLenum eqRGB = ctx->Color.Blend[i].EquationRGB;
         GLenum eqA = ctx->Color.Blend[i].EquationA;
         GLenum srcRGB = ctx->Color.Blend[i].SrcRGB;
         GLenum dstRGB = ctx->Color.Blend[i].DstRGB;
         GLenum srcA = ctx->Color.Blend[i].SrcA;
         GLenum dstA = ctx->Color.Blend[i].DstA;

         if (eqRGB == GL_MIN || eqRGB == GL_MAX)
            srcRGB = dstRGB = GL_ONE;

         if (eqA == GL_MIN || eqA == GL_MAX)
            srcA = dstA = GL_ONE;

         /* Due to hardware limitations, the destination may have information
          * in an alpha channel even when the format specifies no alpha
          * channel. In order to avoid getting any incorrect blending due to
          * that alpha channel, coerce the blend factors to values that will
          * not read the alpha channel, but will instead use the correct
          * implicit value for alpha.
          */
         if (rb && !_mesa_base_format_has_channel(rb->_BaseFormat, GL_TEXTURE_ALPHA_TYPE)) {
            srcRGB = brw_fix_xRGB_alpha(srcRGB);
            srcA = brw_fix_xRGB_alpha(srcA);
            dstRGB = brw_fix_xRGB_alpha(dstRGB);
            dstA = brw_fix_xRGB_alpha(dstA);
         }

         blend[1 + 2*i] |=
            GEN8_BLEND_COLOR_BUFFER_BLEND_ENABLE |
            SET_FIELD(blend_factor(dstRGB), GEN8_BLEND_DST_BLEND_FACTOR) |
            SET_FIELD(blend_factor(srcRGB), GEN8_BLEND_SRC_BLEND_FACTOR) |
            SET_FIELD(blend_factor(dstA), GEN8_BLEND_DST_ALPHA_BLEND_FACTOR) |
            SET_FIELD(blend_factor(srcA), GEN8_BLEND_SRC_ALPHA_BLEND_FACTOR) |
            SET_FIELD(blend_eqn(eqRGB), GEN8_BLEND_COLOR_BLEND_FUNCTION) |
            SET_FIELD(blend_eqn(eqA), GEN8_BLEND_ALPHA_BLEND_FUNCTION);

         if (srcA != srcRGB || dstA != dstRGB || eqA != eqRGB)
            blend[0] |= GEN8_BLEND_INDEPENDENT_ALPHA_BLEND_ENABLE;
      }

      /* See section 8.1.6 "Pre-Blend Color Clamping" of the
       * SandyBridge PRM Volume 2 Part 1 for HW requirements.
       *
       * We do our ARB_color_buffer_float CLAMP_FRAGMENT_COLOR
       * clamping in the fragment shader.  For its clamping of
       * blending, the spec says:
       *
       *     "RESOLVED: For fixed-point color buffers, the inputs and
       *      the result of the blending equation are clamped.  For
       *      floating-point color buffers, no clamping occurs."
       *
       * So, generally, we want clamping to the render target's range.
       * And, good news, the hardware tables for both pre- and
       * post-blend color clamping are either ignored, or any are
       * allowed, or clamping is required but RT range clamping is a
       * valid option.
       */
      blend[1 + 2*i+1] |=
         GEN8_BLEND_PRE_BLEND_COLOR_CLAMP_ENABLE |
         GEN8_BLEND_POST_BLEND_COLOR_CLAMP_ENABLE |
         GEN8_BLEND_COLOR_CLAMP_RANGE_RTFORMAT;

      if (!ctx->Color.ColorMask[i][0])
         blend[1 + 2*i] |= GEN8_BLEND_WRITE_DISABLE_RED;
      if (!ctx->Color.ColorMask[i][1])
         blend[1 + 2*i] |= GEN8_BLEND_WRITE_DISABLE_GREEN;
      if (!ctx->Color.ColorMask[i][2])
         blend[1 + 2*i] |= GEN8_BLEND_WRITE_DISABLE_BLUE;
      if (!ctx->Color.ColorMask[i][3])
         blend[1 + 2*i] |= GEN8_BLEND_WRITE_DISABLE_ALPHA;

     /* From the BLEND_STATE docs, DWord 0, Bit 29 (AlphaToOne Enable):
      * "If Dual Source Blending is enabled, this bit must be disabled."
      */
      WARN_ONCE(ctx->Color.Blend[i]._UsesDualSrc &&
                ctx->Multisample._Enabled &&
                ctx->Multisample.SampleAlphaToOne,
                "HW workaround: disabling alpha to one with dual src "
                "blending\n");
      if (ctx->Color.Blend[i]._UsesDualSrc)
         blend[0] &= ~GEN8_BLEND_ALPHA_TO_ONE_ENABLE;
   }

   BEGIN_BATCH(2);
   OUT_BATCH(_3DSTATE_BLEND_STATE_POINTERS << 16 | (2 - 2));
   OUT_BATCH(brw->cc.blend_state_offset | 1);
   ADVANCE_BATCH();
}

const struct brw_tracked_state gen8_blend_state = {
   .dirty = {
      .mesa = _NEW_COLOR | _NEW_BUFFERS | _NEW_MULTISAMPLE,
      .brw = BRW_NEW_BATCH | BRW_NEW_STATE_BASE_ADDRESS,
      .cache = 0,
   },
   .emit = gen8_upload_blend_state,
};

static void
gen8_upload_ps_blend(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;
   uint32_t dw1 = 0;

   /* _NEW_BUFFERS */
   struct gl_renderbuffer *rb = ctx->DrawBuffer->_ColorDrawBuffers[0];

   /* BRW_NEW_FRAGMENT_PROGRAM | _NEW_BUFFERS | _NEW_COLOR */
   if (brw_color_buffer_write_enabled(brw))
      dw1 |= GEN8_PS_BLEND_HAS_WRITEABLE_RT;

   /* _NEW_COLOR */
   if (ctx->Color.AlphaEnabled)
      dw1 |= GEN8_PS_BLEND_ALPHA_TEST_ENABLE;

   /* _NEW_MULTISAMPLE */
   if (ctx->Multisample._Enabled && ctx->Multisample.SampleAlphaToCoverage)
      dw1 |= GEN8_PS_BLEND_ALPHA_TO_COVERAGE_ENABLE;

   /* Used for implementing the following bit of GL_EXT_texture_integer:
    * "Per-fragment operations that require floating-point color
    *  components, including multisample alpha operations, alpha test,
    *  blending, and dithering, have no effect when the corresponding
    *  colors are written to an integer color buffer."
    *
    * The OpenGL specification 3.3 (page 196), section 4.1.3 says:
    * "If drawbuffer zero is not NONE and the buffer it references has an
    *  integer format, the SAMPLE_ALPHA_TO_COVERAGE and SAMPLE_ALPHA_TO_ONE
    *  operations are skipped."
    */
   GLenum rb_type =
      rb ? _mesa_get_format_datatype(rb->Format) : GL_UNSIGNED_NORMALIZED;

   if (rb && rb_type != GL_INT && rb_type != GL_UNSIGNED_INT &&
       (ctx->Color.BlendEnabled & 1)) {
      GLenum eqRGB = ctx->Color.Blend[0].EquationRGB;
      GLenum eqA = ctx->Color.Blend[0].EquationA;
      GLenum srcRGB = ctx->Color.Blend[0].SrcRGB;
      GLenum dstRGB = ctx->Color.Blend[0].DstRGB;
      GLenum srcA = ctx->Color.Blend[0].SrcA;
      GLenum dstA = ctx->Color.Blend[0].DstA;

      if (eqRGB == GL_MIN || eqRGB == GL_MAX)
         srcRGB = dstRGB = GL_ONE;

      if (eqA == GL_MIN || eqA == GL_MAX)
         srcA = dstA = GL_ONE;

      /* Due to hardware limitations, the destination may have information
       * in an alpha channel even when the format specifies no alpha
       * channel. In order to avoid getting any incorrect blending due to
       * that alpha channel, coerce the blend factors to values that will
       * not read the alpha channel, but will instead use the correct
       * implicit value for alpha.
       */
      if (!_mesa_base_format_has_channel(rb->_BaseFormat, GL_TEXTURE_ALPHA_TYPE)) {
         srcRGB = brw_fix_xRGB_alpha(srcRGB);
         srcA = brw_fix_xRGB_alpha(srcA);
         dstRGB = brw_fix_xRGB_alpha(dstRGB);
         dstA = brw_fix_xRGB_alpha(dstA);
      }

      dw1 |=
         GEN8_PS_BLEND_COLOR_BUFFER_BLEND_ENABLE |
         SET_FIELD(blend_factor(dstRGB), GEN8_PS_BLEND_DST_BLEND_FACTOR) |
         SET_FIELD(blend_factor(srcRGB), GEN8_PS_BLEND_SRC_BLEND_FACTOR) |
         SET_FIELD(blend_factor(dstA), GEN8_PS_BLEND_DST_ALPHA_BLEND_FACTOR) |
         SET_FIELD(blend_factor(srcA), GEN8_PS_BLEND_SRC_ALPHA_BLEND_FACTOR);

      if (srcA != srcRGB || dstA != dstRGB || eqA != eqRGB)
         dw1 |= GEN8_PS_BLEND_INDEPENDENT_ALPHA_BLEND_ENABLE;
   }

   BEGIN_BATCH(2);
   OUT_BATCH(_3DSTATE_PS_BLEND << 16 | (2 - 2));
   OUT_BATCH(dw1);
   ADVANCE_BATCH();
}

const struct brw_tracked_state gen8_ps_blend = {
   .dirty = {
      .mesa = _NEW_BUFFERS | _NEW_COLOR | _NEW_MULTISAMPLE,
      .brw = BRW_NEW_CONTEXT | BRW_NEW_FRAGMENT_PROGRAM,
      .cache = 0,
   },
   .emit = gen8_upload_ps_blend
};
