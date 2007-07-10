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
#include "shader/program.h"

#include "vf/vf.h"
#include "pipe/draw/draw_context.h"
#include "sp_context.h"
#include "sp_state.h"


#define EMIT_ATTR( VF_ATTR, FRAG_ATTR, INTERP )			\
do {								\
   slot_to_vf_attr[softpipe->nr_attrs] = VF_ATTR;		\
   softpipe->vf_attr_to_slot[VF_ATTR] = softpipe->nr_attrs;	\
   softpipe->fp_attr_to_slot[FRAG_ATTR] = softpipe->nr_attrs;	\
   softpipe->interp[softpipe->nr_attrs] = INTERP;		\
   softpipe->nr_attrs++;					\
   attr_mask |= (1 << (VF_ATTR));				\
} while (0)


static const GLuint frag_to_vf[FRAG_ATTRIB_MAX] = 
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
static void calculate_vertex_layout( struct softpipe_context *softpipe )
{
   struct gl_fragment_program *fp = softpipe->fs.fp;
   const GLuint inputsRead = fp->Base.InputsRead;
   GLuint slot_to_vf_attr[VF_ATTRIB_MAX];
   GLbitfield attr_mask = 0x0;
   GLuint i;

   softpipe->nr_attrs = 0;
   memset(slot_to_vf_attr, 0, sizeof(slot_to_vf_attr));

   memset(softpipe->fp_attr_to_slot, 0, sizeof(softpipe->fp_attr_to_slot));
   memset(softpipe->vf_attr_to_slot, 0, sizeof(softpipe->vf_attr_to_slot));

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

   softpipe->nr_frag_attrs = softpipe->nr_attrs;

   /* Additional attributes required for setup: Just twosided
    * lighting.  Edgeflag is dealt with specially by setting bits in
    * the vertex header.
    */
   if (softpipe->setup.light_twoside) {
      if (inputsRead & FRAG_BIT_COL0) {
	 EMIT_ATTR(VF_ATTRIB_BFC0, FRAG_ATTRIB_MAX, 0); /* XXX: mark as discarded after setup */
      }
	    
      if (inputsRead & FRAG_BIT_COL1) {
	 EMIT_ATTR(VF_ATTRIB_BFC1, FRAG_ATTRIB_MAX, 0); /* XXX: discard after setup */
      }
   }

   /* If the attributes have changed, tell the draw module (which in turn
    * tells the vf module) about the new vertex layout.
    */
   if (attr_mask != softpipe->attr_mask) {
      softpipe->attr_mask = attr_mask;

      draw_set_vertex_attributes( softpipe->draw,
				  slot_to_vf_attr,
				  softpipe->nr_attrs );
   }
}


/* Hopefully this will remain quite simple, otherwise need to pull in
 * something like the state tracker mechanism.
 */
void softpipe_update_derived( struct softpipe_context *softpipe )
{
   if (softpipe->dirty & (SP_NEW_SETUP | SP_NEW_FS))
      calculate_vertex_layout( softpipe );

   if (softpipe->dirty & (SP_NEW_BLEND | SP_NEW_DEPTH_TEST | SP_NEW_ALPHA_TEST | SP_NEW_FS))
      sp_build_quad_pipeline(softpipe);

   softpipe->dirty = 0;
}
