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
#include "pipe/p_defines.h"
#include "draw/draw_vertex.h"
#include "draw/draw_vbuf.h"
#include "cell_winsys.h"
#include "cell/common.h"
#include "rtasm/rtasm_ppc_spe.h"
#include "tgsi/util/tgsi_scan.h"

struct cell_vbuf_render;

struct cell_vertex_shader_state
{
   struct pipe_shader_state shader;
   struct tgsi_shader_info info;
   void *draw_data;
};


struct cell_fragment_shader_state
{
   struct pipe_shader_state shader;
   struct tgsi_shader_info info;
   void *data;
};


struct cell_blend_state {
   struct pipe_blend_state base;

   /**
    * Generated code to perform alpha blending
    */
   struct spe_function code;
};


struct cell_depth_stencil_alpha_state {
   struct pipe_depth_stencil_alpha_state   base;

   /**
    * Generated code to perform alpha, stencil, and depth testing on the SPE
    */
   struct spe_function code;

};


struct cell_context
{
   struct pipe_context pipe;

   struct cell_winsys *winsys;

   const struct cell_blend_state *blend;
   const struct pipe_sampler_state *sampler[PIPE_MAX_SAMPLERS];
   uint num_samplers;
   const struct cell_depth_stencil_alpha_state   *depth_stencil;
   const struct pipe_rasterizer_state *rasterizer;
   const struct cell_vertex_shader_state *vs;
   const struct cell_fragment_shader_state *fs;

   struct pipe_blend_color blend_color;
   struct pipe_clip_state clip;
   struct pipe_constant_buffer constants[2];
   struct pipe_framebuffer_state framebuffer;
   struct pipe_poly_stipple poly_stipple;
   struct pipe_scissor_state scissor;
   struct cell_texture *texture[PIPE_MAX_SAMPLERS];
   uint num_textures;
   struct pipe_viewport_state viewport;
   struct pipe_vertex_buffer vertex_buffer[PIPE_ATTRIB_MAX];
   struct pipe_vertex_element vertex_element[PIPE_ATTRIB_MAX];

   ubyte *cbuf_map[PIPE_MAX_COLOR_BUFS];
   ubyte *zsbuf_map;

   struct pipe_surface *tex_surf;
   uint *tex_map;

   uint dirty;

   /** The primitive drawing context */
   struct draw_context *draw;
   struct draw_stage *render_stage;

   /** For post-transformed vertex buffering: */
   struct cell_vbuf_render *vbuf_render;
   struct draw_stage *vbuf;

   struct vertex_info vertex_info;

   /** Mapped constant buffers */
   void *mapped_constants[PIPE_SHADER_TYPES];


   uint num_spus;

   /** Buffers for command batches, vertex/index data */
   uint buffer_size[CELL_NUM_BUFFERS];
   ubyte buffer[CELL_NUM_BUFFERS][CELL_BUFFER_SIZE] ALIGN16_ATTRIB;

   int cur_batch;  /**< which buffer is being filled w/ commands */

   /** [4] to ensure 16-byte alignment for each status word */
   uint buffer_status[CELL_MAX_SPUS][CELL_NUM_BUFFERS][4] ALIGN16_ATTRIB;


   struct spe_function attrib_fetch;
   unsigned attrib_fetch_offsets[PIPE_ATTRIB_MAX];
};




static INLINE struct cell_context *
cell_context(struct pipe_context *pipe)
{
   return (struct cell_context *) pipe;
}


extern struct pipe_context *
cell_create_context(struct pipe_screen *screen, struct cell_winsys *cws);

extern void
cell_vertex_shader_queue_flush(struct draw_context *draw);


/* XXX find a better home for this */
extern void cell_update_vertex_fetch(struct draw_context *draw);


#endif /* CELL_CONTEXT_H */
