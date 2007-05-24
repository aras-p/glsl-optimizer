/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
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

/* Authors:  Keith Whitwell <keith@tungstengraphics.com>
 */

#ifndef G_CONTEXT_H
#define G_CONTEXT_H

#include "glheader.h"

#include "softpipe/sp_state.h"
#include "softpipe/sp_context.h"



struct generic_surface;
struct draw_context;
struct prim_stage;


enum interp_mode {
   INTERP_CONSTANT, 
   INTERP_LINEAR, 
   INTERP_PERSPECTIVE
};


#define G_NEW_VIEWPORT  0x1
#define G_NEW_SETUP     0x2
#define G_NEW_FS        0x4
#define G_NEW_BLEND     0x8
#define G_NEW_CBUF      0x10
#define G_NEW_CLIP      0x20
#define G_NEW_SCISSOR   0x40
#define G_NEW_STIPPLE   0x80



struct generic_context {     
   struct softpipe_context softpipe;


   /* The most recent drawing state as set by the driver:
    */
   struct softpipe_viewport     viewport;
   struct softpipe_setup_state  setup;
   struct softpipe_fs_state     fs;
   struct softpipe_blend_state  blend;
   struct softpipe_surface      cbuf;
   struct softpipe_clip_state   clip;
   struct softpipe_scissor_rect scissor;
   struct softpipe_poly_stipple poly_stipple;
   GLuint dirty;


   /* Cbuf derived state??? 
    */
   struct generic_surface *cbuf_surface;

   /* Clip derived state:
    */
   GLfloat plane[12][4];
   GLuint nr_planes;

   /* Setup derived state.  TODO: this should be passed in the program
    * tokens as parameters to DECL instructions.
    * 
    * For now we just set colors to CONST on flatshade, textures to
    * perspective always and everything else to linear.
    */
   enum interp_mode interp[VF_ATTRIB_MAX];


   /* FS + setup derived state:
    */
   GLuint fp_attr_to_slot[VF_ATTRIB_MAX];
   GLuint vf_attr_to_slot[VF_ATTRIB_MAX];
   GLuint nr_attrs;
   GLuint nr_frag_attrs;
   GLuint attr_mask;

   GLboolean need_z;
   GLboolean need_w;

   /* Stipple derived state:
    */
   GLubyte stipple_masks[16][16];


   /* The software clipper/setup engine.  
    */
   struct {
      struct prim_stage *setup;
      struct prim_stage *unfilled;
      struct prim_stage *twoside;
      struct prim_stage *clip;
      struct prim_stage *flatshade;
      struct prim_stage *offset;
      struct prim_stage *cull;

      struct prim_stage *first;

      GLenum prim;
      GLuint vertex_size;
   } prim;

   /* Temp kludge:
    */
   struct draw_context *draw;
};




static INLINE struct generic_context *
generic_context( struct softpipe_context *softpipe )
{
   return (struct generic_context *)softpipe;
}




#endif
