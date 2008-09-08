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

#include "pipe/p_context.h"
#include "pipe/p_defines.h"

#include "draw/draw_vertex.h"

#include "sp_quad.h"


/**
 * This is a temporary variable for testing draw-stage polygon stipple.
 * If zero, do stipple in sp_quad_stipple.c
 */
#define USE_DRAW_STAGE_PSTIPPLE 1

/* Number of threads working on individual quads.
 * Setting to 1 disables this feature.
 */
#define SP_NUM_QUAD_THREADS 1

struct softpipe_winsys;
struct softpipe_vbuf_render;
struct draw_context;
struct draw_stage;
struct softpipe_tile_cache;
struct sp_fragment_shader;
struct sp_vertex_shader;


struct softpipe_context {
   struct pipe_context pipe;  /**< base class */

   /* The most recent drawing state as set by the driver:
    */
   const struct pipe_blend_state   *blend;
   const struct pipe_sampler_state *sampler[PIPE_MAX_SAMPLERS];
   const struct pipe_depth_stencil_alpha_state   *depth_stencil;
   const struct pipe_rasterizer_state *rasterizer;
   const struct sp_fragment_shader *fs;
   const struct sp_vertex_shader *vs;

   struct pipe_blend_color blend_color;
   struct pipe_clip_state clip;
   struct pipe_constant_buffer constants[PIPE_SHADER_TYPES];
   struct pipe_framebuffer_state framebuffer;
   struct pipe_poly_stipple poly_stipple;
   struct pipe_scissor_state scissor;
   struct pipe_texture *texture[PIPE_MAX_SAMPLERS];
   struct pipe_viewport_state viewport;
   struct pipe_vertex_buffer vertex_buffer[PIPE_MAX_ATTRIBS];
   struct pipe_vertex_element vertex_element[PIPE_MAX_ATTRIBS];
   unsigned dirty;

   unsigned num_samplers;
   unsigned num_textures;
   unsigned num_vertex_elements;
   unsigned num_vertex_buffers;

   boolean no_rast;

   /* Counter for occlusion queries.  Note this supports overlapping
    * queries.
    */
   uint64 occlusion_count;

   /*
    * Mapped vertex buffers
    */
   ubyte *mapped_vbuffer[PIPE_MAX_ATTRIBS];
   
   /** Mapped constant buffers */
   void *mapped_constants[PIPE_SHADER_TYPES];

   /** Vertex format */
   struct vertex_info vertex_info;
   struct vertex_info vertex_info_vbuf;

   int psize_slot;

   unsigned reduced_api_prim;  /**< PIPE_PRIM_POINTS, _LINES or _TRIANGLES */

#if 0
   /* Stipple derived state:
    */
   ubyte stipple_masks[16][16];
#endif

   /** Derived from scissor and surface bounds: */
   struct pipe_scissor_state cliprect;

   unsigned line_stipple_counter;

   /** Software quad rendering pipeline */
   struct {
      struct quad_stage *polygon_stipple;
      struct quad_stage *earlyz;
      struct quad_stage *shade;
      struct quad_stage *alpha_test;
      struct quad_stage *stencil_test;
      struct quad_stage *depth_test;
      struct quad_stage *occlusion;
      struct quad_stage *coverage;
      struct quad_stage *blend;
      struct quad_stage *colormask;
      struct quad_stage *output;

      struct quad_stage *first; /**< points to one of the above stages */
   } quad[SP_NUM_QUAD_THREADS];

   /** The primitive drawing context */
   struct draw_context *draw;
   struct draw_stage *setup;
   struct draw_stage *vbuf;
   struct softpipe_vbuf_render *vbuf_render;

   struct softpipe_tile_cache *cbuf_cache[PIPE_MAX_COLOR_BUFS];
   struct softpipe_tile_cache *zsbuf_cache;

   struct softpipe_tile_cache *tex_cache[PIPE_MAX_SAMPLERS];

   int use_sse : 1;
   int dump_fs : 1;
};


static INLINE struct softpipe_context *
softpipe_context( struct pipe_context *pipe )
{
   return (struct softpipe_context *)pipe;
}

#endif /* SP_CONTEXT_H */

