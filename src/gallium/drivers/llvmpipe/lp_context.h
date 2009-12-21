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

#ifndef LP_CONTEXT_H
#define LP_CONTEXT_H

#include "pipe/p_context.h"

#include "draw/draw_vertex.h"

#include "lp_tex_sample.h"
#include "lp_jit.h"


struct llvmpipe_vbuf_render;
struct draw_context;
struct draw_stage;
struct llvmpipe_tile_cache;
struct llvmpipe_tex_tile_cache;
struct lp_fragment_shader;
struct lp_vertex_shader;
struct lp_blend_state;


struct llvmpipe_context {
   struct pipe_context pipe;  /**< base class */

   /** Constant state objects */
   const struct pipe_blend_state *blend;
   const struct pipe_sampler_state *sampler[PIPE_MAX_SAMPLERS];
   const struct pipe_depth_stencil_alpha_state *depth_stencil;
   const struct pipe_rasterizer_state *rasterizer;
   struct lp_fragment_shader *fs;
   const struct lp_vertex_shader *vs;

   /** Other rendering state */
   struct pipe_blend_color blend_color[4][16];
   struct pipe_clip_state clip;
   struct pipe_constant_buffer constants[PIPE_SHADER_TYPES];
   struct pipe_framebuffer_state framebuffer;
   struct pipe_poly_stipple poly_stipple;
   struct pipe_scissor_state scissor;
   struct pipe_texture *texture[PIPE_MAX_SAMPLERS];
   struct pipe_viewport_state viewport;
   struct pipe_vertex_buffer vertex_buffer[PIPE_MAX_ATTRIBS];
   struct pipe_vertex_element vertex_element[PIPE_MAX_ATTRIBS];

   unsigned num_samplers;
   unsigned num_textures;
   unsigned num_vertex_elements;
   unsigned num_vertex_buffers;

   unsigned dirty; /**< Mask of LP_NEW_x flags */

   /* Counter for occlusion queries.  Note this supports overlapping
    * queries.
    */
   uint64_t occlusion_count;
   unsigned active_query_count;

   /** Mapped vertex buffers */
   ubyte *mapped_vbuffer[PIPE_MAX_ATTRIBS];
   
   /** Vertex format */
   struct vertex_info vertex_info;
   struct vertex_info vertex_info_vbuf;

   /** Which vertex shader output slot contains point size */
   int psize_slot;

   /* The reduced version of the primitive supplied by the state
    * tracker.
    */
   unsigned reduced_api_prim;

   /* The reduced primitive after unfilled triangles, wide-line
    * decomposition, etc, are taken into account.  This is the
    * primitive actually rasterized.
    */
   unsigned reduced_prim;

   /** Derived from scissor and surface bounds: */
   struct pipe_scissor_state cliprect;

   unsigned line_stipple_counter;

   /** TGSI exec things */
   struct {
      struct lp_shader_sampler vert_samplers[PIPE_MAX_SAMPLERS];
      struct lp_shader_sampler *vert_samplers_list[PIPE_MAX_SAMPLERS];
      struct lp_shader_sampler frag_samplers[PIPE_MAX_SAMPLERS];
      struct lp_shader_sampler *frag_samplers_list[PIPE_MAX_SAMPLERS];
   } tgsi;

   /** The primitive drawing context */
   struct draw_context *draw;

   /** Draw module backend */
   struct vbuf_render *vbuf_backend;
   struct draw_stage *vbuf;

   boolean dirty_render_cache;
   
   struct llvmpipe_tile_cache *cbuf_cache[PIPE_MAX_COLOR_BUFS];
   
   /* TODO: we shouldn't be using external interfaces internally like this */
   struct pipe_transfer *zsbuf_transfer;
   uint8_t *zsbuf_map;

   unsigned tex_timestamp;
   struct llvmpipe_tex_tile_cache *tex_cache[PIPE_MAX_SAMPLERS];

   unsigned no_rast : 1;

   struct lp_jit_context jit_context;
};


static INLINE struct llvmpipe_context *
llvmpipe_context( struct pipe_context *pipe )
{
   return (struct llvmpipe_context *)pipe;
}

#endif /* LP_CONTEXT_H */

