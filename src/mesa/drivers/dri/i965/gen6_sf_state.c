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
#include "main/fbobject.h"
#include "intel_batchbuffer.h"

/**
 * Determine the appropriate attribute override value to store into the
 * 3DSTATE_SF structure for a given fragment shader attribute.  The attribute
 * override value contains two pieces of information: the location of the
 * attribute in the VUE (relative to urb_entry_read_offset, see below), and a
 * flag indicating whether to "swizzle" the attribute based on the direction
 * the triangle is facing.
 *
 * If an attribute is "swizzled", then the given VUE location is used for
 * front-facing triangles, and the VUE location that immediately follows is
 * used for back-facing triangles.  We use this to implement the mapping from
 * gl_FrontColor/gl_BackColor to gl_Color.
 *
 * urb_entry_read_offset is the offset into the VUE at which the SF unit is
 * being instructed to begin reading attribute data.  It can be set to a
 * nonzero value to prevent the SF unit from wasting time reading elements of
 * the VUE that are not needed by the fragment shader.  It is measured in
 * 256-bit increments.
 */
static uint32_t
get_attr_override(const struct brw_vue_map *vue_map, int urb_entry_read_offset,
                  int fs_attr, bool two_side_color, uint32_t *max_source_attr)
{
   /* Find the VUE slot for this attribute. */
   int slot = vue_map->varying_to_slot[fs_attr];

   /* If there was only a back color written but not front, use back
    * as the color instead of undefined
    */
   if (slot == -1 && fs_attr == VARYING_SLOT_COL0)
      slot = vue_map->varying_to_slot[VARYING_SLOT_BFC0];
   if (slot == -1 && fs_attr == VARYING_SLOT_COL1)
      slot = vue_map->varying_to_slot[VARYING_SLOT_BFC1];

   if (slot == -1) {
      /* This attribute does not exist in the VUE--that means that the vertex
       * shader did not write to it.  This means that either:
       *
       * (a) This attribute is a texture coordinate, and it is going to be
       * replaced with point coordinates (as a consequence of a call to
       * glTexEnvi(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE)), so the
       * hardware will ignore whatever attribute override we supply.
       *
       * (b) This attribute is read by the fragment shader but not written by
       * the vertex shader, so its value is undefined.  Therefore the
       * attribute override we supply doesn't matter.
       *
       * (c) This attribute is gl_PrimitiveID, and it wasn't written by the
       * previous shader stage.
       *
       * Note that we don't have to worry about the cases where the attribute
       * is gl_PointCoord or is undergoing point sprite coordinate
       * replacement, because in those cases, this function isn't called.
       *
       * In case (c), we need to program the attribute overrides so that the
       * primitive ID will be stored in this slot.  In every other case, the
       * attribute override we supply doesn't matter.  So just go ahead and
       * program primitive ID in every case.
       */
      return (ATTRIBUTE_0_OVERRIDE_W |
              ATTRIBUTE_0_OVERRIDE_Z |
              ATTRIBUTE_0_OVERRIDE_Y |
              ATTRIBUTE_0_OVERRIDE_X |
              (ATTRIBUTE_CONST_PRIM_ID << ATTRIBUTE_0_CONST_SOURCE_SHIFT));
   }

   /* Compute the location of the attribute relative to urb_entry_read_offset.
    * Each increment of urb_entry_read_offset represents a 256-bit value, so
    * it counts for two 128-bit VUE slots.
    */
   int source_attr = slot - 2 * urb_entry_read_offset;
   assert(source_attr >= 0 && source_attr < 32);

   /* If we are doing two-sided color, and the VUE slot following this one
    * represents a back-facing color, then we need to instruct the SF unit to
    * do back-facing swizzling.
    */
   bool swizzling = two_side_color &&
      ((vue_map->slot_to_varying[slot] == VARYING_SLOT_COL0 &&
        vue_map->slot_to_varying[slot+1] == VARYING_SLOT_BFC0) ||
       (vue_map->slot_to_varying[slot] == VARYING_SLOT_COL1 &&
        vue_map->slot_to_varying[slot+1] == VARYING_SLOT_BFC1));

   /* Update max_source_attr.  If swizzling, the SF will read this slot + 1. */
   if (*max_source_attr < source_attr + swizzling)
      *max_source_attr = source_attr + swizzling;

   if (swizzling) {
      return source_attr |
         (ATTRIBUTE_SWIZZLE_INPUTATTR_FACING << ATTRIBUTE_SWIZZLE_SHIFT);
   }

   return source_attr;
}


/**
 * Create the mapping from the FS inputs we produce to the previous pipeline
 * stage (GS or VS) outputs they source from.
 */
void
calculate_attr_overrides(const struct brw_context *brw,
                         uint16_t *attr_overrides,
                         uint32_t *point_sprite_enables,
                         uint32_t *flat_enables,
                         uint32_t *urb_entry_read_length)
{
   const int urb_entry_read_offset = BRW_SF_URB_ENTRY_READ_OFFSET;
   uint32_t max_source_attr = 0;

   *point_sprite_enables = 0;
   *flat_enables = 0;

   /* _NEW_LIGHT */
   bool shade_model_flat = brw->ctx.Light.ShadeModel == GL_FLAT;

   /* Initialize all the attr_overrides to 0.  In the loop below we'll modify
    * just the ones that correspond to inputs used by the fs.
    */
   memset(attr_overrides, 0, 16*sizeof(*attr_overrides));

   for (int attr = 0; attr < VARYING_SLOT_MAX; attr++) {
      enum glsl_interp_qualifier interp_qualifier =
         brw->fragment_program->InterpQualifier[attr];
      bool is_gl_Color = attr == VARYING_SLOT_COL0 || attr == VARYING_SLOT_COL1;
      /* CACHE_NEW_WM_PROG */
      int input_index = brw->wm.prog_data->urb_setup[attr];

      if (input_index < 0)
	 continue;

      /* _NEW_POINT */
      bool point_sprite = false;
      if (brw->ctx.Point.PointSprite &&
	  (attr >= VARYING_SLOT_TEX0 && attr <= VARYING_SLOT_TEX7) &&
	  brw->ctx.Point.CoordReplace[attr - VARYING_SLOT_TEX0]) {
         point_sprite = true;
      }

      if (attr == VARYING_SLOT_PNTC)
         point_sprite = true;

      if (point_sprite)
	 *point_sprite_enables |= (1 << input_index);

      /* flat shading */
      if (interp_qualifier == INTERP_QUALIFIER_FLAT ||
          (shade_model_flat && is_gl_Color &&
           interp_qualifier == INTERP_QUALIFIER_NONE))
         *flat_enables |= (1 << input_index);

      /* BRW_NEW_VUE_MAP_GEOM_OUT | _NEW_LIGHT | _NEW_PROGRAM */
      uint16_t attr_override = point_sprite ? 0 :
         get_attr_override(&brw->vue_map_geom_out,
			   urb_entry_read_offset, attr,
                           brw->ctx.VertexProgram._TwoSideEnabled,
                           &max_source_attr);

      /* The hardware can only do the overrides on 16 overrides at a
       * time, and the other up to 16 have to be lined up so that the
       * input index = the output index.  We'll need to do some
       * tweaking to make sure that's the case.
       */
      if (input_index < 16)
         attr_overrides[input_index] = attr_override;
      else
         assert(attr_override == input_index);
   }

   /* From the Sandy Bridge PRM, Volume 2, Part 1, documentation for
    * 3DSTATE_SF DWord 1 bits 15:11, "Vertex URB Entry Read Length":
    *
    * "This field should be set to the minimum length required to read the
    *  maximum source attribute.  The maximum source attribute is indicated
    *  by the maximum value of the enabled Attribute # Source Attribute if
    *  Attribute Swizzle Enable is set, Number of Output Attributes-1 if
    *  enable is not set.
    *  read_length = ceiling((max_source_attr + 1) / 2)
    *
    *  [errata] Corruption/Hang possible if length programmed larger than
    *  recommended"
    *
    * Similar text exists for Ivy Bridge.
    */
   *urb_entry_read_length = ALIGN(max_source_attr + 1, 2) / 2;
}


static void
upload_sf_state(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;
   /* CACHE_NEW_WM_PROG */
   uint32_t num_outputs = brw->wm.prog_data->num_varying_inputs;
   uint32_t dw1, dw2, dw3, dw4;
   uint32_t point_sprite_enables;
   uint32_t flat_enables;
   int i;
   /* _NEW_BUFFER */
   bool render_to_fbo = _mesa_is_user_fbo(ctx->DrawBuffer);
   bool multisampled_fbo = ctx->DrawBuffer->Visual.samples > 1;

   const int urb_entry_read_offset = BRW_SF_URB_ENTRY_READ_OFFSET;
   float point_size;
   uint16_t attr_overrides[16];
   uint32_t point_sprite_origin;

   dw1 = GEN6_SF_SWIZZLE_ENABLE | num_outputs << GEN6_SF_NUM_OUTPUTS_SHIFT;

   dw2 = GEN6_SF_STATISTICS_ENABLE |
         GEN6_SF_VIEWPORT_TRANSFORM_ENABLE;

   dw3 = 0;
   dw4 = 0;

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
   if (ctx->Scissor.EnableFlags)
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
   {
      uint32_t line_width_u3_7 = U_FIXED(CLAMP(ctx->Line.Width, 0.0, 7.99), 7);
      /* TODO: line width of 0 is not allowed when MSAA enabled */
      if (line_width_u3_7 == 0)
         line_width_u3_7 = 1;
      dw3 |= line_width_u3_7 << GEN6_SF_LINE_WIDTH_SHIFT;
   }
   if (ctx->Line.SmoothFlag) {
      dw3 |= GEN6_SF_LINE_AA_ENABLE;
      dw3 |= GEN6_SF_LINE_AA_MODE_TRUE;
      dw3 |= GEN6_SF_LINE_END_CAP_WIDTH_1_0;
   }
   /* _NEW_MULTISAMPLE */
   if (multisampled_fbo && ctx->Multisample.Enabled)
      dw3 |= GEN6_SF_MSRAST_ON_PATTERN;

   /* _NEW_PROGRAM | _NEW_POINT */
   if (!(ctx->VertexProgram.PointSizeEnabled ||
	 ctx->Point._Attenuated))
      dw4 |= GEN6_SF_USE_STATE_POINT_WIDTH;

   /* Clamp to ARB_point_parameters user limits */
   point_size = CLAMP(ctx->Point.Size, ctx->Point.MinSize, ctx->Point.MaxSize);

   /* Clamp to the hardware limits and convert to fixed point */
   dw4 |= U_FIXED(CLAMP(point_size, 0.125, 255.875), 3);

   /*
    * Window coordinates in an FBO are inverted, which means point
    * sprite origin must be inverted, too.
    */
   if ((ctx->Point.SpriteOrigin == GL_LOWER_LEFT) != render_to_fbo) {
      point_sprite_origin = GEN6_SF_POINT_SPRITE_LOWERLEFT;
   } else {
      point_sprite_origin = GEN6_SF_POINT_SPRITE_UPPERLEFT;
   }
   dw1 |= point_sprite_origin;

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

   /* BRW_NEW_VUE_MAP_GEOM_OUT | _NEW_POINT | _NEW_LIGHT | _NEW_PROGRAM |
    * CACHE_NEW_WM_PROG
    */
   uint32_t urb_entry_read_length;
   calculate_attr_overrides(brw, attr_overrides, &point_sprite_enables,
                            &flat_enables, &urb_entry_read_length);
   dw1 |= (urb_entry_read_length << GEN6_SF_URB_ENTRY_READ_LENGTH_SHIFT |
           urb_entry_read_offset << GEN6_SF_URB_ENTRY_READ_OFFSET_SHIFT);

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
      OUT_BATCH(attr_overrides[i * 2] | attr_overrides[i * 2 + 1] << 16);
   }
   OUT_BATCH(point_sprite_enables); /* dw16 */
   OUT_BATCH(flat_enables);
   OUT_BATCH(0); /* wrapshortest enables 0-7 */
   OUT_BATCH(0); /* wrapshortest enables 8-15 */
   ADVANCE_BATCH();
}

const struct brw_tracked_state gen6_sf_state = {
   .dirty = {
      .mesa  = (_NEW_LIGHT |
		_NEW_PROGRAM |
		_NEW_POLYGON |
		_NEW_LINE |
		_NEW_SCISSOR |
		_NEW_BUFFERS |
		_NEW_POINT |
                _NEW_MULTISAMPLE),
      .brw   = (BRW_NEW_CONTEXT |
		BRW_NEW_FRAGMENT_PROGRAM |
                BRW_NEW_VUE_MAP_GEOM_OUT),
      .cache = CACHE_NEW_WM_PROG
   },
   .emit = upload_sf_state,
};
