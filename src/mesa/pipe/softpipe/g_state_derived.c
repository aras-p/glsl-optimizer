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

#include "glheader.h"
#include "macros.h"
#include "enums.h"
#include "program.h"

#include "g_context.h"
#include "g_draw.h"
#include "g_state.h"

#define EMIT_ATTR( ATTR, FRAG_ATTR, INTERP )			\
do {							\
   slot_to_vf_attr[generic->nr_attrs] = ATTR;	\
   generic->vf_attr_to_slot[ATTR] = generic->nr_attrs;	\
   generic->fp_attr_to_slot[FRAG_ATTR] = generic->nr_attrs;	\
   generic->interp[generic->nr_attrs] = INTERP;	\
   generic->nr_attrs++;	\
   attr_mask |= (1<<ATTR);	\
} while (0)


static GLuint frag_to_vf[FRAG_ATTRIB_MAX] = 
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
};


/* Derived from:  fs, setup states.
 */
static void calculate_vertex_layout( struct generic_context *generic )
{
   struct gl_fragment_program *fp = generic->fs.fp;
   const GLuint inputsRead = fp->Base.InputsRead;
   GLuint slot_to_vf_attr[VF_ATTRIB_MAX];
   GLuint attr_mask = 0;
   GLuint i;

   generic->nr_attrs = 0;
   memset(slot_to_vf_attr, 0, sizeof(slot_to_vf_attr));

   memset(generic->fp_attr_to_slot, 0, sizeof(generic->vf_attr_to_slot));
   memset(generic->vf_attr_to_slot, 0, sizeof(generic->fp_attr_to_slot));

   /* TODO - Figure out if we need to do perspective divide, etc.
    */
   EMIT_ATTR(VF_ATTRIB_POS, FRAG_ATTRIB_WPOS, INTERP_LINEAR);
      
   /* Pull in the rest of the attributes.  They are all in float4
    * format.  Future optimizations could be to keep some attributes
    * as fixed point or ubyte format.
    */
   for (i = 1; i < FRAG_ATTRIB_TEX0; i++) {
      if (inputsRead & (i << i)) {
	 EMIT_ATTR(frag_to_vf[i], i, INTERP_LINEAR);
      }
   }

   for (i = FRAG_ATTRIB_TEX0; i < FRAG_ATTRIB_MAX; i++) {
      if (inputsRead & (i << i)) {
	 EMIT_ATTR(frag_to_vf[i], i, INTERP_PERSPECTIVE);
      }
   }

   generic->nr_frag_attrs = generic->nr_attrs;

   /* Additional attributes required for setup: Just twosided
    * lighting.  Edgeflag is dealt with specially by setting bits in
    * the vertex header.
    */
   if (generic->setup.light_twoside) {
      if (inputsRead & FRAG_BIT_COL0) {
	 EMIT_ATTR(VF_ATTRIB_BFC0, FRAG_ATTRIB_MAX, 0); /* XXX: mark as discarded after setup */
      }
	    
      if (inputsRead & FRAG_BIT_COL1) {
	 EMIT_ATTR(VF_ATTRIB_BFC1, FRAG_ATTRIB_MAX, 0); /* XXX: discard after setup */
      }
   }

   if (attr_mask != generic->attr_mask) {
      generic->attr_mask = attr_mask;

      draw_set_vertex_attributes( generic->draw,
				  slot_to_vf_attr,
				  generic->nr_attrs );
   }
}


/* Hopefully this will remain quite simple, otherwise need to pull in
 * something like the state tracker mechanism.
 */
void generic_update_derived( struct generic_context *generic )
{
   if (generic->dirty & (G_NEW_SETUP | G_NEW_FS))
      calculate_vertex_layout( generic );

   generic->dirty = 0;
}
