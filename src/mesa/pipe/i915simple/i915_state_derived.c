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


static INLINE void
emit_vertex_attr(struct vertex_info *vinfo, uint vfAttr, uint format, uint interp)
{
   const uint n = vinfo->num_attribs;
   vinfo->attr_mask |= (1 << vfAttr);
   vinfo->slot_to_attrib[n] = vfAttr;
   vinfo->format[n] = format;
   vinfo->interp_mode[n] = interp;
   vinfo->num_attribs++;
}


/**
 * Recompute the vinfo->size field.
 */
static void
compute_vertex_size(struct vertex_info *vinfo)
{
   uint i;

   vinfo->size = 0;
   for (i = 0; i < vinfo->num_attribs; i++) {
      switch (vinfo->format[i]) {
      case FORMAT_OMIT:
         break;
      case FORMAT_4UB:
         /* fall-through */
      case FORMAT_1F:
         vinfo->size += 1;
         break;
      case FORMAT_2F:
         vinfo->size += 2;
         break;
      case FORMAT_3F:
         vinfo->size += 3;
         break;
      case FORMAT_4F:
         vinfo->size += 4;
         break;
      default:
         assert(0);
      }
   }
}



/**
 * Determine which post-transform / pre-rasterization vertex attributes
 * we need.
 * Derived from:  fs, setup states.
 */
static void calculate_vertex_layout( struct i915_context *i915 )
{
   const unsigned inputsRead = i915->fs.inputs_read;
   const uint colorInterp
      = i915->setup.flatshade ? INTERP_CONSTANT : INTERP_LINEAR;
   struct vertex_info *vinfo = &i915->current.vertex_info;
   boolean needW = 0;

   memset(vinfo, 0, sizeof(*vinfo));

   /* pos */
   emit_vertex_attr(vinfo, TGSI_ATTRIB_POS, FORMAT_3F, INTERP_LINEAR);
   /* Note: we'll set the S4_VFMT_XYZ[W] bits below */

   /* color0 */
   if (inputsRead & (1 << TGSI_ATTRIB_COLOR0)) {
      emit_vertex_attr(vinfo, TGSI_ATTRIB_COLOR0, FORMAT_4UB, colorInterp);
      vinfo->hwfmt[0] |= S4_VFMT_COLOR;
   }

   /* color 1 */
   if (inputsRead & (1 << TGSI_ATTRIB_COLOR1)) {
      assert(0); /* untested */
      emit_vertex_attr(vinfo, TGSI_ATTRIB_COLOR1, FORMAT_4UB, colorInterp);
      vinfo->hwfmt[0] |= S4_VFMT_SPEC_FOG;
   }

   /* XXX fog? */

   /* texcoords */
   {
      uint i;
      for (i = TGSI_ATTRIB_TEX0; i <= TGSI_ATTRIB_TEX7; i++) {
         uint hwtc;
         if (inputsRead & (1 << i)) {
            emit_vertex_attr(vinfo, i, FORMAT_4F, INTERP_PERSPECTIVE);
            hwtc = TEXCOORDFMT_4D;
            needW = TRUE;
         }
         else {
            hwtc = TEXCOORDFMT_NOT_PRESENT;
         }
         vinfo->hwfmt[1] |= hwtc << ((i - TGSI_ATTRIB_TEX0) * 4);
      }
   }

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
   if (i915->setup.light_twoside) {
      if (inputsRead & (1 << TGSI_ATTRIB_COLOR0)) {
         emit_vertex_attr(vinfo, TGSI_ATTRIB_BFC0, FORMAT_OMIT, colorInterp);
      }	    
      if (inputsRead & (1 << TGSI_ATTRIB_COLOR1)) {
         emit_vertex_attr(vinfo, TGSI_ATTRIB_BFC1, FORMAT_OMIT, colorInterp);
      }
   }

   compute_vertex_size(vinfo);

   /* If the attributes have changed, tell the draw module about the new
    * vertex layout.  We'll also update the hardware vertex format info.
    */
   draw_set_vertex_attributes( i915->draw,
                               vinfo->slot_to_attrib,
                               vinfo->interp_mode,
			       vinfo->num_attribs);

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
   if (i915->dirty & (I915_NEW_SETUP | I915_NEW_FS))
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
