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

#ifndef I915_CONTEXT_H
#define I915_CONTEXT_H


#include "pipe/p_context.h"
#include "pipe/p_state.h"

struct i915_context
{
   struct pipe_context pipe; 
   struct i915_winsys *winsys;
   struct draw_context *draw;

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
   struct pipe_mipmap_tree *texture[PIPE_MAX_SAMPLERS];
   struct pipe_viewport_state viewport;
   GLuint dirty;
   GLuint hw_dirty;

   GLuint *batch_start;

   struct pipe_scissor_state cliprect;
};

#define I915_NEW_VIEWPORT      0x1
#define I915_NEW_SETUP         0x2
#define I915_NEW_FS            0x4
#define I915_NEW_BLEND         0x8
#define I915_NEW_CLIP         0x10
#define I915_NEW_SCISSOR      0x20
#define I915_NEW_STIPPLE      0x40
#define I915_NEW_FRAMEBUFFER  0x80
#define I915_NEW_ALPHA_TEST  0x100
#define I915_NEW_DEPTH_TEST  0x200
#define I915_NEW_SAMPLER     0x400
#define I915_NEW_TEXTURE     0x800
#define I915_NEW_STENCIL    0x1000


/***********************************************************************
 * i915_prim_emit.c: 
 */
struct draw_stage *i915_draw_render_stage( struct i915_context *i915 );


/***********************************************************************
 * i915_state_emit.c: 
 */
void i915_emit_hardware_state(struct i915_context *i915 );
unsigned *i915_passthrough_program( unsigned *dwords );



/***********************************************************************
 * i915_clear.c: 
 */
void i915_clear(struct pipe_context *pipe, struct pipe_surface *ps,
		GLuint clearValue);


/***********************************************************************
 * i915_buffer.c: 
 */
void i915_init_buffer_functions( struct i915_context *i915 );
void i915_init_region_functions( struct i915_context *i915 );
void i915_init_surface_functions( struct i915_context *i915 );
void i915_init_state_functions( struct i915_context *i915 );
void i915_init_flush_functions( struct i915_context *i915 );



/***********************************************************************
 * Inline conversion functions.  These are better-typed than the
 * macros used previously:
 */
static INLINE struct i915_context *
i915_context( struct pipe_context *pipe )
{
   return (struct i915_context *)pipe;
}



#endif
