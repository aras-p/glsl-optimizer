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

static void
gen8_upload_ps_blend(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;
   uint32_t dw1 = 0;

   /* _NEW_BUFFERS */
   struct gl_renderbuffer *rb = ctx->DrawBuffer->_ColorDrawBuffers[0];

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
      .brw = BRW_NEW_CONTEXT,
      .cache = 0,
   },
   .emit = gen8_upload_ps_blend
};
