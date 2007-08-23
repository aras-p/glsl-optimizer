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

/* XXX should include i915_fpc.h but that causes some trouble atm */
extern void i915_translate_fragment_program( struct i915_context *i915 );



static const unsigned frag_to_vf[FRAG_ATTRIB_MAX] = 
{
   VF_ATTRIB_POS,
   VF_ATTRIB_COLOR0,
   VF_ATTRIB_COLOR1,
   VF_ATTRIB_FOG,
   VF_ATTRIB_TEX0,
   VF_ATTRIB_TEX1,
   VF_ATTRIB_TEX2,
   VF_ATTRIB_TEX3,
   VF_ATTRIB_TEX4,
   VF_ATTRIB_TEX5,
   VF_ATTRIB_TEX6,
   VF_ATTRIB_TEX7,
   VF_ATTRIB_VAR0,
   VF_ATTRIB_VAR1,
   VF_ATTRIB_VAR2,
   VF_ATTRIB_VAR3,
   VF_ATTRIB_VAR4,
   VF_ATTRIB_VAR5,
   VF_ATTRIB_VAR6,
   VF_ATTRIB_VAR7,
};


static INLINE void
emit_vertex_attr(struct vertex_info *vinfo, uint vfAttr, uint format)
{
   const uint n = vinfo->num_attribs;
   vinfo->attr_mask |= (1 << vfAttr);
   vinfo->slot_to_attrib[n] = vfAttr;
   printf("Vertex slot %d = vfattrib %d\n", n, vfAttr);
   /*vinfo->interp_mode[n] = interpMode;*/
   vinfo->format[n] = format;
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
//   const unsigned inputsRead = (FRAG_BIT_WPOS | FRAG_BIT_COL0);
   unsigned i;
   struct vertex_info *vinfo = &i915->current.vertex_info;

   memset(vinfo, 0, sizeof(*vinfo));

   /* TODO - Figure out if we need to do perspective divide, etc.
    */
   emit_vertex_attr(vinfo, VF_ATTRIB_POS, FORMAT_3F);
   vinfo->hwfmt[0] |= S4_VFMT_XYZ;
      
   /* Pull in the rest of the attributes.  They are all in float4
    * format.  Future optimizations could be to keep some attributes
    * as fixed point or ubyte format.
    */
   for (i = 1; i < FRAG_ATTRIB_TEX0; i++) {
      if (inputsRead & (1 << i)) {
         assert(i < Elements(frag_to_vf));
         if (i915->setup.flatshade
             && (i == FRAG_ATTRIB_COL0 || i == FRAG_ATTRIB_COL1)) {
            emit_vertex_attr(vinfo, frag_to_vf[i], FORMAT_4UB);
         }   
         else {
            emit_vertex_attr(vinfo, frag_to_vf[i], FORMAT_4UB);
         }
         vinfo->hwfmt[0] |= S4_VFMT_COLOR;
      }
   }

   for (i = FRAG_ATTRIB_TEX0; i <= FRAG_ATTRIB_TEX7/*MAX*/; i++) {
      uint hwtc;
      if (inputsRead & (1 << i)) {
         hwtc = TEXCOORDFMT_4D;
         assert(i < sizeof(frag_to_vf) / sizeof(frag_to_vf[0]));
         emit_vertex_attr(vinfo, frag_to_vf[i], FORMAT_4F);
      }
      else {
         hwtc = TEXCOORDFMT_NOT_PRESENT;
      }
      vinfo->hwfmt[1] |= hwtc << ((i - FRAG_ATTRIB_TEX0) * 4);
   }

   /* Additional attributes required for setup: Just twosided
    * lighting.  Edgeflag is dealt with specially by setting bits in
    * the vertex header.
    */
   if (i915->setup.light_twoside) {
      if (inputsRead & FRAG_BIT_COL0) {
         /* XXX: mark as discarded after setup */
         emit_vertex_attr(vinfo, VF_ATTRIB_BFC0, FORMAT_OMIT);
      }
	    
      if (inputsRead & FRAG_BIT_COL1) {
         /* XXX: discard after setup */
         emit_vertex_attr(vinfo, VF_ATTRIB_BFC1, FORMAT_OMIT);
      }
   }

   compute_vertex_size(vinfo);

   /* If the attributes have changed, tell the draw module about the new
    * vertex layout.  We'll also update the hardware vertex format info.
    */
   draw_set_vertex_attributes( i915->draw,
                               vinfo->slot_to_attrib,
			       vinfo->num_attribs);
#if 0
   printf("VERTEX_FORMAT LIS2: 0x%x  LIS4: 0x%x\n",
          vinfo->hwfmt[1], vinfo->hwfmt[0]);
#endif
}




/* Hopefully this will remain quite simple, otherwise need to pull in
 * something like the state tracker mechanism.
 */
void i915_update_derived( struct i915_context *i915 )
{
   if (i915->dirty & (I915_NEW_SETUP | I915_NEW_FS))
      calculate_vertex_layout( i915 );

   if (i915->dirty & I915_NEW_SAMPLER)
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
