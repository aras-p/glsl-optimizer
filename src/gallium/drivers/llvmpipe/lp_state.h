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

#ifndef LP_STATE_H
#define LP_STATE_H

#include <llvm-c/Core.h>

#include "pipe/p_state.h"
#include "tgsi/tgsi_scan.h"
#include "lp_jit.h"
#include "gallivm/lp_bld_sample.h" /* for struct lp_sampler_static_state */


#define LP_NEW_VIEWPORT      0x1
#define LP_NEW_RASTERIZER    0x2
#define LP_NEW_FS            0x4
#define LP_NEW_BLEND         0x8
#define LP_NEW_CLIP          0x10
#define LP_NEW_SCISSOR       0x20
#define LP_NEW_STIPPLE       0x40
#define LP_NEW_FRAMEBUFFER   0x80
#define LP_NEW_DEPTH_STENCIL_ALPHA 0x100
#define LP_NEW_CONSTANTS     0x200
#define LP_NEW_SAMPLER       0x400
#define LP_NEW_TEXTURE       0x800
#define LP_NEW_VERTEX        0x1000
#define LP_NEW_VS            0x2000
#define LP_NEW_QUERY         0x4000
#define LP_NEW_BLEND_COLOR   0x8000


struct vertex_info;
struct pipe_context;
struct llvmpipe_context;

struct lp_fragment_shader;


struct lp_fragment_shader_variant_key
{
   struct pipe_depth_state depth;
   struct pipe_alpha_state alpha;
   struct pipe_blend_state blend;
   enum pipe_format zsbuf_format;
   unsigned nr_cbufs:8;
   unsigned flatshade:1;
   unsigned scissor:1;

   struct {
      ubyte colormask;
   } cbuf_blend[PIPE_MAX_COLOR_BUFS];
   
   struct lp_sampler_static_state sampler[PIPE_MAX_SAMPLERS];
};


struct lp_fragment_shader_variant
{
   struct lp_fragment_shader *shader;

   struct lp_fragment_shader_variant_key key;

   LLVMValueRef function[2];

   lp_jit_frag_func jit_function[2];

   struct lp_fragment_shader_variant *next;
};


/**
 * Subclass of pipe_shader_state (though it doesn't really need to be).
 *
 * This is starting to look an awful lot like a quad pipeline stage...
 */
struct lp_fragment_shader
{
   struct pipe_shader_state base;

   struct tgsi_shader_info info;

   struct lp_fragment_shader_variant *variants;

   struct lp_fragment_shader_variant *current;
};


/** Subclass of pipe_shader_state */
struct lp_vertex_shader {
   struct pipe_shader_state shader;
   struct draw_vertex_shader *draw_data;
};



void *
llvmpipe_create_blend_state(struct pipe_context *,
                            const struct pipe_blend_state *);
void llvmpipe_bind_blend_state(struct pipe_context *,
                               void *);
void llvmpipe_delete_blend_state(struct pipe_context *,
                                 void *);

void *
llvmpipe_create_sampler_state(struct pipe_context *,
                              const struct pipe_sampler_state *);
void llvmpipe_bind_sampler_states(struct pipe_context *, unsigned, void **);
void
llvmpipe_bind_vertex_sampler_states(struct pipe_context *,
                                    unsigned num_samplers,
                                    void **samplers);
void llvmpipe_delete_sampler_state(struct pipe_context *, void *);

void *
llvmpipe_create_depth_stencil_state(struct pipe_context *,
                                    const struct pipe_depth_stencil_alpha_state *);
void llvmpipe_bind_depth_stencil_state(struct pipe_context *, void *);
void llvmpipe_delete_depth_stencil_state(struct pipe_context *, void *);

void *
llvmpipe_create_rasterizer_state(struct pipe_context *,
                                 const struct pipe_rasterizer_state *);
void llvmpipe_bind_rasterizer_state(struct pipe_context *, void *);
void llvmpipe_delete_rasterizer_state(struct pipe_context *, void *);

void llvmpipe_set_framebuffer_state( struct pipe_context *,
                                     const struct pipe_framebuffer_state * );

void llvmpipe_set_blend_color( struct pipe_context *pipe,
                               const struct pipe_blend_color *blend_color );

void llvmpipe_set_stencil_ref( struct pipe_context *pipe,
                               const struct pipe_stencil_ref *stencil_ref );

void llvmpipe_set_clip_state( struct pipe_context *,
                              const struct pipe_clip_state * );

void llvmpipe_set_constant_buffer(struct pipe_context *,
                                  uint shader, uint index,
                                  struct pipe_buffer *buf);

void *llvmpipe_create_fs_state(struct pipe_context *,
                               const struct pipe_shader_state *);
void llvmpipe_bind_fs_state(struct pipe_context *, void *);
void llvmpipe_delete_fs_state(struct pipe_context *, void *);
void *llvmpipe_create_vs_state(struct pipe_context *,
                               const struct pipe_shader_state *);
void llvmpipe_bind_vs_state(struct pipe_context *, void *);
void llvmpipe_delete_vs_state(struct pipe_context *, void *);

void llvmpipe_set_polygon_stipple( struct pipe_context *,
				  const struct pipe_poly_stipple * );

void llvmpipe_set_scissor_state( struct pipe_context *,
                                 const struct pipe_scissor_state * );

void llvmpipe_set_sampler_textures( struct pipe_context *,
                                    unsigned num,
                                    struct pipe_texture ** );

void
llvmpipe_set_vertex_sampler_textures(struct pipe_context *,
                                     unsigned num_textures,
                                     struct pipe_texture **);

void llvmpipe_set_viewport_state( struct pipe_context *,
                                  const struct pipe_viewport_state * );

void llvmpipe_set_vertex_elements(struct pipe_context *,
                                  unsigned count,
                                  const struct pipe_vertex_element *);

void llvmpipe_set_vertex_buffers(struct pipe_context *,
                                 unsigned count,
                                 const struct pipe_vertex_buffer *);

void llvmpipe_update_fs(struct llvmpipe_context *lp);

void llvmpipe_update_derived( struct llvmpipe_context *llvmpipe );


void llvmpipe_draw_arrays(struct pipe_context *pipe, unsigned mode,
			     unsigned start, unsigned count);

void llvmpipe_draw_elements(struct pipe_context *pipe,
			       struct pipe_buffer *indexBuffer,
			       unsigned indexSize,
			       unsigned mode, unsigned start, unsigned count);
void
llvmpipe_draw_range_elements(struct pipe_context *pipe,
                             struct pipe_buffer *indexBuffer,
                             unsigned indexSize,
                             unsigned min_index,
                             unsigned max_index,
                             unsigned mode, unsigned start, unsigned count);

void
llvmpipe_map_texture_surfaces(struct llvmpipe_context *lp);

void
llvmpipe_unmap_texture_surfaces(struct llvmpipe_context *lp);


#endif
