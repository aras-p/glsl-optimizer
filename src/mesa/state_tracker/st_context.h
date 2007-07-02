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

#ifndef ST_CONTEXT_H
#define ST_CONTEXT_H

#include "mtypes.h"
#include "pipe/p_state.h"


struct st_context;
struct st_region;
struct st_texture_object;
struct st_texture_image;
struct st_fragment_program;

#define ST_NEW_MESA                    0x1 /* Mesa state has changed */
#define ST_NEW_FRAGMENT_PROGRAM        0x2

struct st_state_flags {
   GLuint mesa;
   GLuint st;
};

struct st_tracked_state {
   struct st_state_flags dirty;
   void (*update)( struct st_context *st );
};




struct st_context
{
   GLcontext *ctx;

   struct pipe_context *pipe;

   /* Eventually will use a cache to feed the pipe with
    * create/bind/delete calls to constant state objects.  Not yet
    * though, we just shove random objects across the interface.  
    */
   struct {
      struct pipe_alpha_test_state  alpha_test;
      struct pipe_blend_state  blend;
      struct pipe_clear_color_state clear_color;
      struct pipe_clip_state clip;
      struct pipe_depth_state depth;
      struct pipe_framebuffer_state framebuffer;
      struct pipe_fs_state     fs;
      struct pipe_poly_stipple poly_stipple;
      struct pipe_sampler_state sampler[PIPE_MAX_SAMPLERS];
      struct pipe_scissor_state scissor;
      struct pipe_setup_state  setup;
      struct pipe_stencil_state stencil;
      struct pipe_viewport_state viewport;
   } state;

   struct {
      struct st_tracked_state tracked_state;
   } constants;

   struct {
      struct gl_fragment_program *fragment_program;
   } cb;

   /* State to be validated:
    */
   struct st_tracked_state **atoms;
   GLuint nr_atoms;

   struct st_state_flags dirty;

   /* Counter to track program string changes:
    */
   GLuint program_id;

   GLfloat polygon_offset_scale; /* ?? */
};


/* Need this so that we can implement Mesa callbacks in this module.
 */
static INLINE struct st_context *st_context(GLcontext *ctx)
{
   return ctx->st;
}


#endif
