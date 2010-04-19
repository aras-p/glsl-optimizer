
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

/**
 * \brief  Public interface into the drawing module.
 */

/* Authors:  Keith Whitwell <keith@tungstengraphics.com>
 */


#ifndef DRAW_CONTEXT_H
#define DRAW_CONTEXT_H


#include "pipe/p_state.h"


struct pipe_context;
struct draw_context;
struct draw_stage;
struct draw_vertex_shader;
struct draw_geometry_shader;
struct tgsi_sampler;


struct draw_context *draw_create( struct pipe_context *pipe );

void draw_destroy( struct draw_context *draw );

void draw_set_viewport_state( struct draw_context *draw,
                              const struct pipe_viewport_state *viewport );

void draw_set_clip_state( struct draw_context *pipe,
                          const struct pipe_clip_state *clip );

void draw_set_rasterizer_state( struct draw_context *draw,
                                const struct pipe_rasterizer_state *raster,
                                void *rast_handle );

void draw_set_rasterize_stage( struct draw_context *draw,
                               struct draw_stage *stage );

void draw_wide_point_threshold(struct draw_context *draw, float threshold);

void draw_wide_line_threshold(struct draw_context *draw, float threshold);

void draw_enable_line_stipple(struct draw_context *draw, boolean enable);

void draw_enable_point_sprites(struct draw_context *draw, boolean enable);

void draw_set_mrd(struct draw_context *draw, double mrd);

boolean
draw_install_aaline_stage(struct draw_context *draw, struct pipe_context *pipe);

boolean
draw_install_aapoint_stage(struct draw_context *draw, struct pipe_context *pipe);

boolean
draw_install_pstipple_stage(struct draw_context *draw, struct pipe_context *pipe);


int
draw_find_shader_output(const struct draw_context *draw,
                        uint semantic_name, uint semantic_index);

uint
draw_num_shader_outputs(const struct draw_context *draw);


void
draw_texture_samplers(struct draw_context *draw,
                      uint num_samplers,
                      struct tgsi_sampler **samplers);



/*
 * Vertex shader functions
 */

struct draw_vertex_shader *
draw_create_vertex_shader(struct draw_context *draw,
                          const struct pipe_shader_state *shader);
void draw_bind_vertex_shader(struct draw_context *draw,
                             struct draw_vertex_shader *dvs);
void draw_delete_vertex_shader(struct draw_context *draw,
                               struct draw_vertex_shader *dvs);


/*
 * Geometry shader functions
 */
struct draw_geometry_shader *
draw_create_geometry_shader(struct draw_context *draw,
                            const struct pipe_shader_state *shader);
void draw_bind_geometry_shader(struct draw_context *draw,
                               struct draw_geometry_shader *dvs);
void draw_delete_geometry_shader(struct draw_context *draw,
                                 struct draw_geometry_shader *dvs);


/*
 * Vertex data functions
 */

void draw_set_vertex_buffers(struct draw_context *draw,
                             unsigned count,
                             const struct pipe_vertex_buffer *buffers);

void draw_set_vertex_elements(struct draw_context *draw,
			      unsigned count,
                              const struct pipe_vertex_element *elements);

void
draw_set_mapped_element_buffer_range( struct draw_context *draw,
                                      unsigned eltSize,
                                      unsigned min_index,
                                      unsigned max_index,
                                      const void *elements );

void draw_set_mapped_element_buffer( struct draw_context *draw,
                                     unsigned eltSize, 
                                     const void *elements );

void draw_set_mapped_vertex_buffer(struct draw_context *draw,
                                   unsigned attr, const void *buffer);

void
draw_set_mapped_constant_buffer(struct draw_context *draw,
                                unsigned shader_type,
                                unsigned slot,
                                const void *buffer,
                                unsigned size);


/***********************************************************************
 * draw_prim.c 
 */

void draw_arrays(struct draw_context *draw, unsigned prim,
		 unsigned start, unsigned count);

void
draw_arrays_instanced(struct draw_context *draw,
                      unsigned mode,
                      unsigned start,
                      unsigned count,
                      unsigned startInstance,
                      unsigned instanceCount);

void draw_flush(struct draw_context *draw);


/*******************************************************************************
 * Driver backend interface 
 */
struct vbuf_render;
void draw_set_render( struct draw_context *draw, 
		      struct vbuf_render *render );

void draw_set_driver_clipping( struct draw_context *draw,
                               boolean bypass_clipping );

void draw_set_force_passthrough( struct draw_context *draw, 
                                 boolean enable );

/*******************************************************************************
 * Draw pipeline 
 */
boolean draw_need_pipeline(const struct draw_context *draw,
                           const struct pipe_rasterizer_state *rasterizer,
                           unsigned prim );



#endif /* DRAW_CONTEXT_H */
