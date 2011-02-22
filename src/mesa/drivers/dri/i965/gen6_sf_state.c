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
#include "main/macros.h"
#include "intel_batchbuffer.h"

static uint32_t
get_attr_override(struct brw_context *brw, int fs_attr, int two_side_color)
{
   int attr_index = 0, i, vs_attr;
   int bfc = 0;

   if (fs_attr <= FRAG_ATTRIB_TEX7)
      vs_attr = fs_attr;
   else if (fs_attr == FRAG_ATTRIB_FACE)
      vs_attr = 0; /* XXX */
   else if (fs_attr == FRAG_ATTRIB_PNTC)
      vs_attr = 0; /* XXX */
   else {
      assert(fs_attr >= FRAG_ATTRIB_VAR0);
      vs_attr = fs_attr - FRAG_ATTRIB_VAR0 + VERT_RESULT_VAR0;
   }

   /* Find the source index (0 = first attribute after the 4D position)
    * for this output attribute.  attr is currently a VERT_RESULT_* but should
    * be FRAG_ATTRIB_*.
    */
   for (i = 1; i < vs_attr; i++) {
      if (i == VERT_RESULT_PSIZ)
	 continue;
      if (brw->vs.prog_data->outputs_written & BITFIELD64_BIT(i))
	 attr_index++;
   }

   assert(attr_index < 32);

   if (two_side_color) {
       if ((brw->vs.prog_data->outputs_written & BITFIELD64_BIT(VERT_RESULT_COL1)) &&
           (brw->vs.prog_data->outputs_written & BITFIELD64_BIT(VERT_RESULT_BFC1))) {
           assert(brw->vs.prog_data->outputs_written & BITFIELD64_BIT(VERT_RESULT_COL0));
           assert(brw->vs.prog_data->outputs_written & BITFIELD64_BIT(VERT_RESULT_BFC0));
           bfc = 2;
       } else if ((brw->vs.prog_data->outputs_written & BITFIELD64_BIT(VERT_RESULT_COL0)) &&
                (brw->vs.prog_data->outputs_written & BITFIELD64_BIT(VERT_RESULT_BFC0)))
           bfc = 1;
   }

   if (bfc && (fs_attr <= FRAG_ATTRIB_TEX7 && fs_attr > FRAG_ATTRIB_WPOS)) {
       if (fs_attr == FRAG_ATTRIB_COL0)
           attr_index |= (ATTRIBUTE_SWIZZLE_INPUTATTR_FACING << ATTRIBUTE_SWIZZLE_SHIFT);
       else if (fs_attr == FRAG_ATTRIB_COL1 && bfc == 2) {
           attr_index++;
           attr_index |= (ATTRIBUTE_SWIZZLE_INPUTATTR_FACING << ATTRIBUTE_SWIZZLE_SHIFT);
       } else {
           attr_index += bfc;
       }
   }

   return attr_index;
}

static void
upload_sf_state(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;
   struct gl_context *ctx = &intel->ctx;
   /* CACHE_NEW_VS_PROG */
   uint32_t num_inputs = brw_count_bits(brw->vs.prog_data->outputs_written);
   /* BRW_NEW_FRAGMENT_PROGRAM */
   uint32_t num_outputs = brw_count_bits(brw->fragment_program->Base.InputsRead);
   uint32_t dw1, dw2, dw3, dw4, dw16, dw17;
   int i;
   /* _NEW_BUFFER */
   GLboolean render_to_fbo = brw->intel.ctx.DrawBuffer->Name != 0;
   int attr = 0;
   int urb_start;
   int two_side_color = (ctx->Light.Enabled && ctx->Light.Model.TwoSide);

   /* _NEW_TRANSFORM */
   if (ctx->Transform.ClipPlanesEnabled)
      urb_start = 2;
   else
      urb_start = 1;

   dw1 =
      GEN6_SF_SWIZZLE_ENABLE |
      num_outputs << GEN6_SF_NUM_OUTPUTS_SHIFT |
      (num_inputs + 1) / 2 << GEN6_SF_URB_ENTRY_READ_LENGTH_SHIFT |
      urb_start << GEN6_SF_URB_ENTRY_READ_OFFSET_SHIFT;
   dw2 = GEN6_SF_VIEWPORT_TRANSFORM_ENABLE |
      GEN6_SF_STATISTICS_ENABLE;
   dw3 = 0;
   dw4 = 0;
   dw16 = 0;
   dw17 = 0;

   /* _NEW_POLYGON */
   if ((ctx->Polygon.FrontFace == GL_CCW) ^ render_to_fbo)
      dw2 |= GEN6_SF_WINDING_CCW;

   if (ctx->Polygon.OffsetFill)
       dw2 |= GEN6_SF_GLOBAL_DEPTH_OFFSET_SOLID;

   if (ctx->Polygon.OffsetLine)
       dw2 |= GEN6_SF_GLOBAL_DEPTH_OFFSET_WIREFRAME;

   if (ctx->Polygon.OffsetPoint)
       dw2 |= GEN6_SF_GLOBAL_DEPTH_OFFSET_POINT;

   switch (ctx->Polygon.FrontMode) {
   case GL_FILL:
       dw2 |= GEN6_SF_FRONT_SOLID;
       break;

   case GL_LINE:
       dw2 |= GEN6_SF_FRONT_WIREFRAME;
       break;

   case GL_POINT:
       dw2 |= GEN6_SF_FRONT_POINT;
       break;

   default:
       assert(0);
       break;
   }

   switch (ctx->Polygon.BackMode) {
   case GL_FILL:
       dw2 |= GEN6_SF_BACK_SOLID;
       break;

   case GL_LINE:
       dw2 |= GEN6_SF_BACK_WIREFRAME;
       break;

   case GL_POINT:
       dw2 |= GEN6_SF_BACK_POINT;
       break;

   default:
       assert(0);
       break;
   }

   /* _NEW_SCISSOR */
   if (ctx->Scissor.Enabled)
      dw3 |= GEN6_SF_SCISSOR_ENABLE;

   /* _NEW_POLYGON */
   if (ctx->Polygon.CullFlag) {
      switch (ctx->Polygon.CullFaceMode) {
      case GL_FRONT:
	 dw3 |= GEN6_SF_CULL_FRONT;
	 break;
      case GL_BACK:
	 dw3 |= GEN6_SF_CULL_BACK;
	 break;
      case GL_FRONT_AND_BACK:
	 dw3 |= GEN6_SF_CULL_BOTH;
	 break;
      default:
	 assert(0);
	 break;
      }
   } else {
      dw3 |= GEN6_SF_CULL_NONE;
   }

   /* _NEW_LINE */
   dw3 |= U_FIXED(CLAMP(ctx->Line.Width, 0.0, 7.99), 7) <<
      GEN6_SF_LINE_WIDTH_SHIFT;
   if (ctx->Line.SmoothFlag) {
      dw3 |= GEN6_SF_LINE_AA_ENABLE;
      dw3 |= GEN6_SF_LINE_AA_MODE_TRUE;
      dw3 |= GEN6_SF_LINE_END_CAP_WIDTH_1_0;
   }

   /* _NEW_POINT */
   if (!(ctx->VertexProgram.PointSizeEnabled ||
	 ctx->Point._Attenuated))
      dw4 |= GEN6_SF_USE_STATE_POINT_WIDTH;

   dw4 |= U_FIXED(CLAMP(ctx->Point.Size, 0.125, 225.875), 3) <<
      GEN6_SF_POINT_WIDTH_SHIFT;
   if (ctx->Point.SpriteOrigin == GL_LOWER_LEFT)
      dw1 |= GEN6_SF_POINT_SPRITE_LOWERLEFT;

   /* _NEW_LIGHT */
   if (ctx->Light.ProvokingVertex != GL_FIRST_VERTEX_CONVENTION) {
      dw4 |=
	 (2 << GEN6_SF_TRI_PROVOKE_SHIFT) |
	 (2 << GEN6_SF_TRIFAN_PROVOKE_SHIFT) |
	 (1 << GEN6_SF_LINE_PROVOKE_SHIFT);
   } else {
      dw4 |=
	 (1 << GEN6_SF_TRIFAN_PROVOKE_SHIFT);
   }

   if (ctx->Point.PointSprite) {
       for (i = 0; i < 8; i++) { 
	   if (ctx->Point.CoordReplace[i])
	       dw16 |= (1 << i);
       }
   }

   /* flat shading */
   if (ctx->Light.ShadeModel == GL_FLAT) {
       dw17 |= ((brw->fragment_program->Base.InputsRead & (FRAG_BIT_COL0 | FRAG_BIT_COL1)) >>
                ((brw->fragment_program->Base.InputsRead & FRAG_BIT_WPOS) ? 0 : 1));
   }

   BEGIN_BATCH(20);
   OUT_BATCH(_3DSTATE_SF << 16 | (20 - 2));
   OUT_BATCH(dw1);
   OUT_BATCH(dw2);
   OUT_BATCH(dw3);
   OUT_BATCH(dw4);
   OUT_BATCH_F(ctx->Polygon.OffsetUnits * 2); /* constant.  copied from gen4 */
   OUT_BATCH_F(ctx->Polygon.OffsetFactor); /* scale */
   OUT_BATCH_F(0.0); /* XXX: global depth offset clamp */
   for (i = 0; i < 8; i++) {
      uint32_t attr_overrides = 0;

      for (; attr < 64; attr++) {
	 if (brw->fragment_program->Base.InputsRead & BITFIELD64_BIT(attr)) {
	    attr_overrides |= get_attr_override(brw, attr, two_side_color);
	    attr++;
	    break;
	 }
      }

      for (; attr < 64; attr++) {
	 if (brw->fragment_program->Base.InputsRead & BITFIELD64_BIT(attr)) {
	    attr_overrides |= get_attr_override(brw, attr, two_side_color) << 16;
	    attr++;
	    break;
	 }
      }
      OUT_BATCH(attr_overrides);
   }
   OUT_BATCH(dw16); /* point sprite texcoord bitmask */
   OUT_BATCH(dw17); /* constant interp bitmask */
   OUT_BATCH(0); /* wrapshortest enables 0-7 */
   OUT_BATCH(0); /* wrapshortest enables 8-15 */
   ADVANCE_BATCH();
}

const struct brw_tracked_state gen6_sf_state = {
   .dirty = {
      .mesa  = (_NEW_LIGHT |
		_NEW_POLYGON |
		_NEW_LINE |
		_NEW_SCISSOR |
		_NEW_BUFFERS |
		_NEW_POINT |
		_NEW_TRANSFORM),
      .brw   = (BRW_NEW_CONTEXT |
		BRW_NEW_FRAGMENT_PROGRAM),
      .cache = CACHE_NEW_VS_PROG
   },
   .emit = upload_sf_state,
};
