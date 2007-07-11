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

#ifndef SP_CONTEXT_H
#define SP_CONTEXT_H

#include "glheader.h"

#include "pipe/p_state.h"
#include "pipe/p_context.h"

#include "sp_quad.h"


struct softpipe_surface;
struct draw_context;
struct prim_stage;


enum interp_mode {
   INTERP_CONSTANT, 
   INTERP_LINEAR, 
   INTERP_PERSPECTIVE
};


#define SP_NEW_VIEWPORT      0x1
#define SP_NEW_SETUP         0x2
#define SP_NEW_FS            0x4
#define SP_NEW_BLEND         0x8
#define SP_NEW_CLIP         0x10
#define SP_NEW_SCISSOR      0x20
#define SP_NEW_STIPPLE      0x40
#define SP_NEW_FRAMEBUFFER  0x80
#define SP_NEW_ALPHA_TEST  0x100
#define SP_NEW_DEPTH_TEST  0x200
#define SP_NEW_SAMPLER     0x400
#define SP_NEW_TEXTURE     0x800
#define SP_NEW_STENCIL    0x1000


struct softpipe_context {     
   struct pipe_context pipe;  /**< base class */

   /* The most recent drawing state as set by the driver:
    */
   struct pipe_alpha_test_state alpha_test;
   struct pipe_blend_state blend;
   struct pipe_blend_color blend_color;
   struct pipe_clear_color_state clear_color;
   struct pipe_clip_state clip;
   struct pipe_depth_state depth_test;
   struct pipe_framebuffer_state framebuffer;
   struct pipe_fs_state fs;
   struct pipe_poly_stipple poly_stipple;
   struct pipe_scissor_state scissor;
   struct pipe_sampler_state sampler[PIPE_MAX_SAMPLERS];
   struct pipe_setup_state setup;
   struct pipe_stencil_state stencil;
   struct pipe_texture_object *texture[PIPE_MAX_SAMPLERS];
   struct pipe_viewport_state viewport;
   GLuint dirty;

   /* Setup derived state.  TODO: this should be passed in the program
    * tokens as parameters to DECL instructions.
    * 
    * For now we just set colors to CONST on flatshade, textures to
    * perspective always and everything else to linear.
    */
   enum interp_mode interp[PIPE_ATTRIB_MAX];


   /* FS + setup derived state:
    */

   /** Map fragment program attribute to quad/coef array slot */
   GLuint fp_attr_to_slot[PIPE_ATTRIB_MAX];
   /** Map vertex format attribute to a vertex attribute slot */
   GLuint vf_attr_to_slot[PIPE_ATTRIB_MAX];
   GLuint nr_attrs;
   GLuint nr_frag_attrs;  /**< number of active fragment attribs */
   GLbitfield attr_mask;  /**< bitfield of VF_ATTRIB_ indexes/bits */

   GLboolean need_z;  /**< produce quad/fragment Z values? */
   GLboolean need_w;  /**< produce quad/fragment W values? */

   /* Stipple derived state:
    */
   GLubyte stipple_masks[16][16];

   /** Software quad rendering pipeline */
   struct {
      struct quad_stage *polygon_stipple;
      struct quad_stage *shade;
      struct quad_stage *alpha_test;
      struct quad_stage *stencil_test;
      struct quad_stage *depth_test;
      struct quad_stage *blend;
      struct quad_stage *output;

      struct quad_stage *first; /**< points to one of the above stages */
   } quad;

   /** The primitive drawing context */
   struct draw_context *draw;
};




static INLINE struct softpipe_context *
softpipe_context( struct pipe_context *pipe )
{
   return (struct softpipe_context *)pipe;
}


#endif /* SP_CONTEXT_H */
