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
#include "tgsi/tgsi_scan.h"
#include "util/u_keymap.h"


struct cell_vbuf_render;


/**
 * Cell vertex shader state, subclass of pipe_shader_state.
 */
struct cell_vertex_shader_state
{
   struct pipe_shader_state shader;
   struct tgsi_shader_info info;
   void *draw_data;
};


/**
 * Cell fragment shader state, subclass of pipe_shader_state.
 */
struct cell_fragment_shader_state
{
   struct pipe_shader_state shader;
   struct tgsi_shader_info info;
   struct spe_function code;
   void *data;
};


/**
 * Key for mapping per-fragment state to cached SPU machine code.
 *  keymap(cell_fragment_ops_key) => cell_command_fragment_ops
 */
struct cell_fragment_ops_key
{
   struct pipe_blend_state blend;
   struct pipe_blend_color blend_color;
   struct pipe_depth_stencil_alpha_state dsa;
   enum pipe_format color_format;
   enum pipe_format zs_format;
};


struct cell_buffer_node;

/**
 * Fenced buffer list.  List of buffers which can be unreferenced after
 * the fence has been executed/signalled.
 */
struct cell_buffer_list
{
   PIPE_ALIGN_VAR(16) struct cell_fence fence;
   struct cell_buffer_node *head;
};


/**
 * Per-context state, subclass of pipe_context.
 */
struct cell_context
{
   struct pipe_context pipe;

   struct cell_winsys *winsys;

   const struct pipe_blend_state *blend;
   const struct pipe_sampler_state *sampler[PIPE_MAX_SAMPLERS];
   uint num_samplers;
   const struct pipe_depth_stencil_alpha_state *depth_stencil;
   const struct pipe_rasterizer_state *rasterizer;
   const struct cell_vertex_shader_state *vs;
   const struct cell_fragment_shader_state *fs;

   struct spe_function logic_op;

   struct pipe_blend_color blend_color;
   struct pipe_stencil_ref stencil_ref;
   struct pipe_clip_state clip;
   struct pipe_buffer *constants[2];
   struct pipe_framebuffer_state framebuffer;
   struct pipe_poly_stipple poly_stipple;
   struct pipe_scissor_state scissor;
   struct cell_texture *texture[PIPE_MAX_SAMPLERS];
   uint num_textures;
   struct pipe_viewport_state viewport;
   struct pipe_vertex_buffer vertex_buffer[PIPE_MAX_ATTRIBS];
   uint num_vertex_buffers;
   struct pipe_vertex_element vertex_element[PIPE_MAX_ATTRIBS];
   uint num_vertex_elements;

   ubyte *cbuf_map[PIPE_MAX_COLOR_BUFS];
   ubyte *zsbuf_map;

   uint dirty;
   uint dirty_textures;  /* bitmask of texture units */
   uint dirty_samplers;  /* bitmask of sampler units */

   /** Cache of code generated for per-fragment ops */
   struct keymap *fragment_ops_cache;

   /** The primitive drawing context */
   struct draw_context *draw;
   struct draw_stage *render_stage;

   /** For post-transformed vertex buffering: */
   struct cell_vbuf_render *vbuf_render;
   struct draw_stage *vbuf;

   struct vertex_info vertex_info;

   /** Mapped constant buffers */
   void *mapped_constants[PIPE_SHADER_TYPES];

   PIPE_ALIGN_VAR(16) struct cell_spu_function_info spu_functions;

   uint num_cells, num_spus;

   /** Buffers for command batches, vertex/index data */
   uint buffer_size[CELL_NUM_BUFFERS];
   PIPE_ALIGN_VAR(16) ubyte buffer[CELL_NUM_BUFFERS][CELL_BUFFER_SIZE];

   int cur_batch;  /**< which buffer is being filled w/ commands */

   /** [4] to ensure 16-byte alignment for each status word */
   PIPE_ALIGN_VAR(16) uint buffer_status[CELL_MAX_SPUS][CELL_NUM_BUFFERS][4];


   /** Associated with each command/batch buffer is a list of pipe_buffers
    * that are fenced.  When the last command in a buffer is executed, the
    * fence will be signalled, indicating that any pipe_buffers preceeding
    * that fence can be unreferenced (and probably freed).
    */
   struct cell_buffer_list fenced_buffers[CELL_NUM_BUFFERS];


   struct spe_function attrib_fetch;
   unsigned attrib_fetch_offsets[PIPE_MAX_ATTRIBS];

   unsigned debug_flags;
};




static INLINE struct cell_context *
cell_context(struct pipe_context *pipe)
{
   return (struct cell_context *) pipe;
}


struct pipe_context *
cell_create_context(struct pipe_screen *screen,
                    void *priv );

extern void
cell_vertex_shader_queue_flush(struct draw_context *draw);


/* XXX find a better home for this */
extern void cell_update_vertex_fetch(struct draw_context *draw);


#endif /* CELL_CONTEXT_H */
