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

#ifndef FO_CONTEXT_H
#define FO_CONTEXT_H

#include "pipe/p_state.h"
#include "pipe/p_context.h"



#define FO_NEW_VIEWPORT        0x1
#define FO_NEW_RASTERIZER      0x2
#define FO_NEW_FRAGMENT_SHADER 0x4
#define FO_NEW_BLEND           0x8
#define FO_NEW_CLIP            0x10
#define FO_NEW_SCISSOR         0x20
#define FO_NEW_STIPPLE         0x40
#define FO_NEW_FRAMEBUFFER     0x80
#define FO_NEW_ALPHA_TEST      0x100
#define FO_NEW_DEPTH_STENCIL   0x200
#define FO_NEW_SAMPLER         0x400
#define FO_NEW_TEXTURE         0x800
#define FO_NEW_VERTEX          0x2000
#define FO_NEW_VERTEX_SHADER   0x4000
#define FO_NEW_BLEND_COLOR     0x8000
#define FO_NEW_STENCIL_REF     0x10000
#define FO_NEW_CLEAR_COLOR     0x20000
#define FO_NEW_VERTEX_BUFFER   0x40000
#define FO_NEW_VERTEX_ELEMENT  0x80000



#define FO_HW 0
#define FO_SW 1

struct fo_state {
   void *sw_state;
   void *hw_state;
};
struct failover_context {
   struct pipe_context pipe;  /**< base class */


   /* The most recent drawing state as set by the driver:
    */
   const struct fo_state     *blend;
   const struct fo_state     *sampler[PIPE_MAX_SAMPLERS];
   const struct fo_state     *vertex_samplers[PIPE_MAX_VERTEX_SAMPLERS];
   const struct fo_state     *depth_stencil;
   const struct fo_state     *rasterizer;
   const struct fo_state     *fragment_shader;
   const struct fo_state     *vertex_shader;

   struct pipe_blend_color blend_color;
   struct pipe_stencil_ref stencil_ref;
   struct pipe_clip_state clip;
   struct pipe_framebuffer_state framebuffer;
   struct pipe_poly_stipple poly_stipple;
   struct pipe_scissor_state scissor;
   struct pipe_texture *texture[PIPE_MAX_SAMPLERS];
   struct pipe_texture *vertex_textures[PIPE_MAX_VERTEX_SAMPLERS];
   struct pipe_viewport_state viewport;
   struct pipe_vertex_buffer vertex_buffers[PIPE_MAX_ATTRIBS];
   struct pipe_vertex_element vertex_elements[PIPE_MAX_ATTRIBS];

   uint num_vertex_buffers;
   uint num_vertex_elements;

   void *sw_sampler_state[PIPE_MAX_SAMPLERS];
   void *hw_sampler_state[PIPE_MAX_SAMPLERS];
   void *sw_vertex_sampler_state[PIPE_MAX_VERTEX_SAMPLERS];
   void *hw_vertex_sampler_state[PIPE_MAX_VERTEX_SAMPLERS];

   unsigned dirty;

   unsigned num_samplers;
   unsigned num_vertex_samplers;
   unsigned num_textures;
   unsigned num_vertex_textures;

   unsigned mode;
   struct pipe_context *hw;
   struct pipe_context *sw;
};



void failover_init_state_functions( struct failover_context *failover );
void failover_state_emit( struct failover_context *failover );

static INLINE struct failover_context *
failover_context( struct pipe_context *pipe )
{
   return (struct failover_context *)pipe;
}

/* Internal functions
 */
void
failover_set_constant_buffer(struct pipe_context *pipe,
                             uint shader, uint index,
                             struct pipe_buffer *buf);


#endif /* FO_CONTEXT_H */
