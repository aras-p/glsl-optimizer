/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/


#include "pipe/p_util.h"
#include "pipe/draw/draw_context.h"
#include "pipe/draw/draw_vertex.h"
#include "i915_context.h"
#include "i915_state.h"
#include "i915_reg.h"
#include "i915_fpc.h"
#include "pipe/tgsi/exec/tgsi_token.h"


/**
 * Determine which post-transform / pre-rasterization vertex attributes
 * we need.
 * Derived from:  fs, setup states.
 */
static void calculate_vertex_layout( struct i915_context *i915 )
{
   const struct pipe_shader_state *fs = i915->fs;
   const interp_mode colorInterp = i915->rasterizer->color_interp;
   struct vertex_info *vinfo = &i915->current.vertex_info;
   uint front0 = 0, back0 = 0, front1 = 0, back1 = 0;
   boolean needW = 0;
   uint i;
   boolean texCoords[8];

   memset(texCoords, 0, sizeof(texCoords));
   memset(vinfo, 0, sizeof(*vinfo));

   /* pos */
   draw_emit_vertex_attr(vinfo, FORMAT_3F, INTERP_LINEAR);
   /* Note: we'll set the S4_VFMT_XYZ[W] bits below */

   for (i = 0; i < fs->num_inputs; i++) {
      switch (fs->input_semantics[i]) {
      case TGSI_SEMANTIC_POSITION:
         break;
      case TGSI_SEMANTIC_COLOR0:
         front0 = draw_emit_vertex_attr(vinfo, FORMAT_4UB, colorInterp);
         vinfo->hwfmt[0] |= S4_VFMT_COLOR;
         break;
      case TGSI_SEMANTIC_COLOR1:
         assert(0); /* untested */
         front1 = draw_emit_vertex_attr(vinfo, FORMAT_4UB, colorInterp);
         vinfo->hwfmt[0] |= S4_VFMT_SPEC_FOG;
         break;
      case TGSI_SEMANTIC_TEX0:
      case TGSI_SEMANTIC_TEX1:
      case TGSI_SEMANTIC_TEX2:
      case TGSI_SEMANTIC_TEX3:
      case TGSI_SEMANTIC_TEX4:
      case TGSI_SEMANTIC_TEX5:
      case TGSI_SEMANTIC_TEX6:
      case TGSI_SEMANTIC_TEX7:
         {
            const uint unit = fs->input_semantics[i] - TGSI_SEMANTIC_TEX0;
            uint hwtc;
            texCoords[unit] = TRUE;
            draw_emit_vertex_attr(vinfo, FORMAT_4F, INTERP_PERSPECTIVE);
            hwtc = TEXCOORDFMT_4D;
            needW = TRUE;
            vinfo->hwfmt[1] |= hwtc << (unit * 4);
         }
         break;
      default:
         assert(0);
      }

   }

   /* finish up texcoord fields */
   for (i = 0; i < 8; i++) {
      if (!texCoords[i]) {
         const uint hwtc = TEXCOORDFMT_NOT_PRESENT;
         vinfo->hwfmt[1] |= hwtc << (i* 4);
      }
   }

#if 0
   /* color0 */
   if (inputsRead & (1 << TGSI_ATTRIB_COLOR0)) {
      front0 = draw_emit_vertex_attr(vinfo, FORMAT_4UB, colorInterp);
      vinfo->hwfmt[0] |= S4_VFMT_COLOR;
   }

   /* color 1 */
   if (inputsRead & (1 << TGSI_ATTRIB_COLOR1)) {
      assert(0); /* untested */
      front1 = draw_emit_vertex_attr(vinfo, FORMAT_4UB, colorInterp);
      vinfo->hwfmt[0] |= S4_VFMT_SPEC_FOG;
   }

   /* XXX fog? */

   /* texcoords */
   {
      uint i;
      for (i = TGSI_ATTRIB_TEX0; i <= TGSI_ATTRIB_TEX7; i++) {
         uint hwtc;
         if (inputsRead & (1 << i)) {
            draw_emit_vertex_attr(vinfo, FORMAT_4F, INTERP_PERSPECTIVE);
            hwtc = TEXCOORDFMT_4D;
            needW = TRUE;
         }
         else {
            hwtc = TEXCOORDFMT_NOT_PRESENT;
         }
         vinfo->hwfmt[1] |= hwtc << ((i - TGSI_ATTRIB_TEX0) * 4);
      }
   }
#endif

   /* go back and fill in the vertex position info now that we have needW */
   if (needW) {
      vinfo->hwfmt[0] |= S4_VFMT_XYZW;
      vinfo->format[0] = FORMAT_4F;
   }
   else {
      vinfo->hwfmt[0] |= S4_VFMT_XYZ;
      vinfo->format[0] = FORMAT_3F;
   }

   /* Additional attributes required for setup: Just twosided
    * lighting.  Edgeflag is dealt with specially by setting bits in
    * the vertex header.
    */
   if (i915->rasterizer->light_twoside) {
      if (front0) {
         back0 = draw_emit_vertex_attr(vinfo, FORMAT_OMIT, colorInterp);
      }
      if (back0) {
         back1 = draw_emit_vertex_attr(vinfo, FORMAT_OMIT, colorInterp);
      }
   }

   draw_compute_vertex_size(vinfo);

   /* If the attributes have changed, tell the draw module about the new
    * vertex layout.  We'll also update the hardware vertex format info.
    */
   draw_set_vertex_attributes( i915->draw,
                               NULL,/*vinfo->slot_to_attrib,*/
                               vinfo->interp_mode,
			       vinfo->num_attribs);

   draw_set_twoside_attributes(i915->draw,
                               front0, back0, front1, back1);

   /* Need to set this flag so that the LIS2/4 registers get set.
    * It also means the i915_update_immediate() function must be called
    * after this one, in i915_update_derived().
    */
   i915->dirty |= I915_NEW_VERTEX_FORMAT;
}




/* Hopefully this will remain quite simple, otherwise need to pull in
 * something like the state tracker mechanism.
 */
void i915_update_derived( struct i915_context *i915 )
{
   if (i915->dirty & (I915_NEW_RASTERIZER | I915_NEW_FS))
      calculate_vertex_layout( i915 );

   if (i915->dirty & (I915_NEW_SAMPLER | I915_NEW_TEXTURE))
      i915_update_samplers(i915);

   if (i915->dirty & I915_NEW_TEXTURE)
      i915_update_textures(i915);

   if (i915->dirty)
      i915_update_immediate( i915 );

   if (i915->dirty)
      i915_update_dynamic( i915 );

   if (i915->dirty & I915_NEW_FS) {
      i915_translate_fragment_program(i915);
      i915->hardware_dirty |= I915_HW_PROGRAM; /* XXX right? */
   }

   /* HW emit currently references framebuffer state directly:
    */
   if (i915->dirty & I915_NEW_FRAMEBUFFER)
      i915->hardware_dirty |= I915_HW_STATIC;

   i915->dirty = 0;
}
