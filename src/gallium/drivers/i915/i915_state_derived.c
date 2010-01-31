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


#include "util/u_memory.h"
#include "pipe/p_shader_tokens.h"
#include "draw/draw_context.h"
#include "draw/draw_vertex.h"
#include "i915_context.h"
#include "i915_state.h"
#include "i915_reg.h"



/**
 * Determine the hardware vertex layout.
 * Depends on vertex/fragment shader state.
 */
static void calculate_vertex_layout( struct i915_context *i915 )
{
   const struct i915_fragment_shader *fs = i915->fs;
   const enum interp_mode colorInterp = i915->rasterizer->color_interp;
   struct vertex_info vinfo;
   boolean texCoords[8], colors[2], fog, needW;
   uint i;
   int src;

   memset(texCoords, 0, sizeof(texCoords));
   colors[0] = colors[1] = fog = needW = FALSE;
   memset(&vinfo, 0, sizeof(vinfo));

   /* Determine which fragment program inputs are needed.  Setup HW vertex
    * layout below, in the HW-specific attribute order.
    */
   for (i = 0; i < fs->info.num_inputs; i++) {
      switch (fs->info.input_semantic_name[i]) {
      case TGSI_SEMANTIC_POSITION:
         break;
      case TGSI_SEMANTIC_COLOR:
         assert(fs->info.input_semantic_index[i] < 2);
         colors[fs->info.input_semantic_index[i]] = TRUE;
         break;
      case TGSI_SEMANTIC_GENERIC:
         /* usually a texcoord */
         {
            const uint unit = fs->info.input_semantic_index[i];
            assert(unit < 8);
            texCoords[unit] = TRUE;
            needW = TRUE;
         }
         break;
      case TGSI_SEMANTIC_FOG:
         fog = TRUE;
         break;
      default:
         assert(0);
      }
   }

   
   /* pos */
   src = draw_find_shader_output(i915->draw, TGSI_SEMANTIC_POSITION, 0);
   if (needW) {
      draw_emit_vertex_attr(&vinfo, EMIT_4F, INTERP_LINEAR, src);
      vinfo.hwfmt[0] |= S4_VFMT_XYZW;
      vinfo.attrib[0].emit = EMIT_4F;
   }
   else {
      draw_emit_vertex_attr(&vinfo, EMIT_3F, INTERP_LINEAR, src);
      vinfo.hwfmt[0] |= S4_VFMT_XYZ;
      vinfo.attrib[0].emit = EMIT_3F;
   }

   /* hardware point size */
   /* XXX todo */

   /* primary color */
   if (colors[0]) {
      src = draw_find_shader_output(i915->draw, TGSI_SEMANTIC_COLOR, 0);
      draw_emit_vertex_attr(&vinfo, EMIT_4UB, colorInterp, src);
      vinfo.hwfmt[0] |= S4_VFMT_COLOR;
   }

   /* secondary color */
   if (colors[1]) {
      src = draw_find_shader_output(i915->draw, TGSI_SEMANTIC_COLOR, 1);
      draw_emit_vertex_attr(&vinfo, EMIT_4UB, colorInterp, src);
      vinfo.hwfmt[0] |= S4_VFMT_SPEC_FOG;
   }

   /* fog coord, not fog blend factor */
   if (fog) {
      src = draw_find_shader_output(i915->draw, TGSI_SEMANTIC_FOG, 0);
      draw_emit_vertex_attr(&vinfo, EMIT_1F, INTERP_PERSPECTIVE, src);
      vinfo.hwfmt[0] |= S4_VFMT_FOG_PARAM;
   }

   /* texcoords */
   for (i = 0; i < 8; i++) {
      uint hwtc;
      if (texCoords[i]) {
         hwtc = TEXCOORDFMT_4D;
         src = draw_find_shader_output(i915->draw, TGSI_SEMANTIC_GENERIC, i);
         draw_emit_vertex_attr(&vinfo, EMIT_4F, INTERP_PERSPECTIVE, src);
      }
      else {
         hwtc = TEXCOORDFMT_NOT_PRESENT;
      }
      vinfo.hwfmt[1] |= hwtc << (i * 4);
   }

   draw_compute_vertex_size(&vinfo);

   if (memcmp(&i915->current.vertex_info, &vinfo, sizeof(vinfo))) {
      /* Need to set this flag so that the LIS2/4 registers get set.
       * It also means the i915_update_immediate() function must be called
       * after this one, in i915_update_derived().
       */
      i915->dirty |= I915_NEW_VERTEX_FORMAT;

      memcpy(&i915->current.vertex_info, &vinfo, sizeof(vinfo));
   }
}




/* Hopefully this will remain quite simple, otherwise need to pull in
 * something like the state tracker mechanism.
 */
void i915_update_derived( struct i915_context *i915 )
{
   if (i915->dirty & (I915_NEW_RASTERIZER | I915_NEW_FS | I915_NEW_VS))
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
      i915->hardware_dirty |= I915_HW_PROGRAM; /* XXX right? */
   }

   /* HW emit currently references framebuffer state directly:
    */
   if (i915->dirty & I915_NEW_FRAMEBUFFER)
      i915->hardware_dirty |= I915_HW_STATIC;

   i915->dirty = 0;
}
