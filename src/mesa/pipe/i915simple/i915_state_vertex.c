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

#include "main/glheader.h"
#include "main/macros.h"
#include "main/enums.h"

#include "vf/vf.h"
#include "pipe/draw/draw_context.h"
#include "i915_context.h"
#include "i915_state.h"




/***********************************************************************
 * 
 */

#define SZ_TO_HW(sz)  ((sz-2)&0x3)
#define EMIT_SZ(sz)   (EMIT_1F + (sz) - 1)
#define EMIT_ATTR( ATTR, STYLE, S4, SZ )			\
do {								\
   i915->vertex_attrs[i915->vertex_attr_count].attrib = (ATTR);	\
   i915->vertex_attrs[i915->vertex_attr_count].format = (STYLE);	\
   i915->vertex_attr_count++;					\
   s4 |= S4;							\
   offset += (SZ);						\
} while (0)

#define EMIT_PAD( N )						\
do {								\
   i915->vertex_attrs[i915->vertex_attr_count].attrib = 0;		\
   i915->vertex_attrs[i915->vertex_attr_count].format = EMIT_PAD;	\
   i915->vertex_attrs[i915->vertex_attr_count].offset = (N);		\
   i915->vertex_attr_count++;					\
   offset += (N);						\
} while (0)



/**
 * Determine which post-transform / pre-rasterization vertex attributes
 * we need.
 * Derived from:  fs, setup states.
 */
static void calculate_vertex_layout( struct i915_context *i915 )
{
   const GLbitfield inputsRead = i915->fs.inputs_read;
   GLuint slot_to_vf_attr[VF_ATTRIB_MAX];
   GLuint nr_attrs = 0;
   GLuint i;



   memset(slot_to_vf_attr, 0, sizeof(slot_to_vf_attr));

   EMIT_ATTR(VF_ATTRIB_POS, FRAG_ATTRIB_WPOS, INTERP_LINEAR);
   EMIT_ATTR(VF_ATTRIB_COLOR0, FRAG_ATTRIB_COL0, INTERP_LINEAR);

   i915->hw_vertex_size = i915->nr_attrs;

   /* Additional attributes required for setup: Just twosided
    * lighting.  Edgeflag is dealt with specially by setting bits in
    * the vertex header.
    */
   if (i915->setup.light_twoside) {
      EMIT_ATTR(VF_ATTRIB_BFC0, FRAG_ATTRIB_MAX, 0); /* XXX: mark as discarded after setup */
   }

   /* If the attributes have changed, tell the draw module (which in turn
    * tells the vf module) about the new vertex layout.
    */
   draw_set_vertex_attributes( i915->draw,
			       slot_to_vf_attr,
			       i915->nr_attrs );
}



/***********************************************************************
 * 
 */
static inline GLuint attr_size(GLuint sizes, GLuint attr)
{
   return ((sizes >> (attr*2)) & 0x3) + 1;
}

static void i915_calculate_vertex_format( struct intel_context *intel )
{
   struct i915_context *i915 = i915_context( &intel->ctx );

   GLuint s2 = S2_TEXCOORD_NONE;
   GLuint s4 = 0;
   GLuint offset = 0;
   GLuint i;
   i915->vertex_attr_count = 0;

   EMIT_ATTR(VF_ATTRIB_POS, EMIT_4F_VIEWPORT, S4_VFMT_XYZW, 16);
   EMIT_ATTR(VF_ATTRIB_COLOR0, EMIT_4UB_4F_BGRA, S4_VFMT_COLOR, 4);



   if (s2 != i915->vertex_format.LIS2 || 
       s4 != i915->vertex_format.LIS4) {

      clip_set_hw_vertex_format( intel->clip, 
				 i915->vertex_attrs, 
				 i915->vertex_attr_count,
				 offset );

      /* Needed?  This does raise the INTEL_NEW_VERTEX_SIZE flag:
       */
      intel_vb_set_vertex_size( intel->vb, offset );
      
      i915->vertex_format.LIS2 = s2;
      i915->vertex_format.LIS4 = s4;
      intel->state.dirty.intel |= I915_NEW_VERTEX_FORMAT;
   }
}

