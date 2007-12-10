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


#ifndef CELL_CONTEXT_H
#define CELL_CONTEXT_H


#include "pipe/p_context.h"
#include "cell_winsys.h"


struct cell_context
{
   struct pipe_context pipe;

   struct cell_winsys *winsys;

   const struct pipe_alpha_test_state *alpha_test;
   const struct pipe_blend_state *blend;
   const struct pipe_sampler_state *sampler[PIPE_MAX_SAMPLERS];
   const struct pipe_depth_stencil_state   *depth_stencil;
   const struct pipe_rasterizer_state *rasterizer;

   struct pipe_blend_color blend_color;
   struct pipe_clear_color_state clear_color;
   struct pipe_clip_state clip;
   struct pipe_constant_buffer constants[2];
   struct pipe_feedback_state feedback;
   struct pipe_framebuffer_state framebuffer;
   struct pipe_poly_stipple poly_stipple;
   struct pipe_scissor_state scissor;
   struct softpipe_texture *texture[PIPE_MAX_SAMPLERS];
   struct pipe_viewport_state viewport;
   struct pipe_vertex_buffer vertex_buffer[PIPE_ATTRIB_MAX];
   struct pipe_vertex_element vertex_element[PIPE_ATTRIB_MAX];
   uint sampler_units[PIPE_MAX_SAMPLERS];
   uint dirty;

   /** The primitive drawing context */
   struct draw_context *draw;
   struct draw_stage *setup;
   struct draw_stage *vbuf;


   uint num_spus;
   

};



static INLINE struct cell_context *
cell_context(struct pipe_context *pipe)
{
   return (struct cell_context *) pipe;
}


extern struct pipe_context *
cell_create_context(struct pipe_winsys *ws, struct cell_winsys *cws);





#endif /* CELL_CONTEXT_H */
