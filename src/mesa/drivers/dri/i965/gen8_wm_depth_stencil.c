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

#include "intel_batchbuffer.h"
#include "intel_fbo.h"
#include "brw_context.h"
#include "brw_defines.h"
#include "brw_state.h"

static void
gen8_upload_wm_depth_stencil(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;
   uint32_t dw1 = 0, dw2 = 0;

   /* _NEW_BUFFERS */
   struct intel_renderbuffer *depth_irb =
      intel_get_renderbuffer(ctx->DrawBuffer, BUFFER_DEPTH);

   struct gl_stencil_attrib *stencil = &ctx->Stencil;

   /* _NEW_STENCIL | _NEW_BUFFERS */
   if (stencil->_Enabled) {
      #define FUNC intel_translate_compare_func
      #define OP intel_translate_stencil_op

      dw1 |=
         GEN8_WM_DS_STENCIL_TEST_ENABLE |
         FUNC(stencil->Function[0]) << GEN8_WM_DS_STENCIL_FUNC_SHIFT |
         OP(stencil->FailFunc[0])  << GEN8_WM_DS_STENCIL_FAIL_OP_SHIFT |
         OP(stencil->ZFailFunc[0]) << GEN8_WM_DS_Z_FAIL_OP_SHIFT |
         OP(stencil->ZPassFunc[0]) << GEN8_WM_DS_Z_PASS_OP_SHIFT;

      if (stencil->_WriteEnabled)
         dw1 |= GEN8_WM_DS_STENCIL_BUFFER_WRITE_ENABLE;

      dw2 |=
         SET_FIELD(stencil->WriteMask[0] & 0xff, GEN8_WM_DS_STENCIL_WRITE_MASK) |
         SET_FIELD(stencil->ValueMask[0] & 0xff, GEN8_WM_DS_STENCIL_TEST_MASK);

      if (stencil->_TestTwoSide) {
         const int b = stencil->_BackFace;

         dw1 |=
            GEN8_WM_DS_DOUBLE_SIDED_STENCIL_ENABLE |
            FUNC(stencil->Function[b]) << GEN8_WM_DS_BF_STENCIL_FUNC_SHIFT |
            OP(stencil->FailFunc[b]) << GEN8_WM_DS_BF_STENCIL_FAIL_OP_SHIFT |
            OP(stencil->ZFailFunc[b]) << GEN8_WM_DS_BF_Z_FAIL_OP_SHIFT |
            OP(stencil->ZPassFunc[b]) << GEN8_WM_DS_BF_Z_PASS_OP_SHIFT;

         dw2 |= SET_FIELD(stencil->WriteMask[b] & 0xff,
                          GEN8_WM_DS_BF_STENCIL_WRITE_MASK) |
                SET_FIELD(stencil->ValueMask[b] & 0xff,
                          GEN8_WM_DS_BF_STENCIL_TEST_MASK);
      }
   }

   /* _NEW_DEPTH */
   if (ctx->Depth.Test && depth_irb) {
      dw1 |=
         GEN8_WM_DS_DEPTH_TEST_ENABLE |
         FUNC(ctx->Depth.Func) << GEN8_WM_DS_DEPTH_FUNC_SHIFT;

      if (ctx->Depth.Mask)
         dw1 |= GEN8_WM_DS_DEPTH_BUFFER_WRITE_ENABLE;
   }

   BEGIN_BATCH(3);
   OUT_BATCH(_3DSTATE_WM_DEPTH_STENCIL << 16 | (3 - 2));
   OUT_BATCH(dw1);
   OUT_BATCH(dw2);
   ADVANCE_BATCH();
}

const struct brw_tracked_state gen8_wm_depth_stencil = {
   .dirty = {
      .mesa = _NEW_BUFFERS | _NEW_DEPTH | _NEW_STENCIL,
      .brw  = BRW_NEW_CONTEXT,
      .cache = 0,
   },
   .emit = gen8_upload_wm_depth_stencil,
};
