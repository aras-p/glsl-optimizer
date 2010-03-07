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

#ifndef SP_STATE_H
#define SP_STATE_H

#include "pipe/p_state.h"
#include "tgsi/tgsi_scan.h"


#define SP_NEW_VIEWPORT      0x1
#define SP_NEW_RASTERIZER    0x2
#define SP_NEW_FS            0x4
#define SP_NEW_BLEND         0x8
#define SP_NEW_CLIP          0x10
#define SP_NEW_SCISSOR       0x20
#define SP_NEW_STIPPLE       0x40
#define SP_NEW_FRAMEBUFFER   0x80
#define SP_NEW_DEPTH_STENCIL_ALPHA 0x100
#define SP_NEW_CONSTANTS     0x200
#define SP_NEW_SAMPLER       0x400
#define SP_NEW_TEXTURE       0x800
#define SP_NEW_VERTEX        0x1000
#define SP_NEW_VS            0x2000
#define SP_NEW_QUERY         0x4000
#define SP_NEW_GS            0x8000


struct tgsi_sampler;
struct tgsi_exec_machine;
struct vertex_info;


/**
 * Subclass of pipe_shader_state (though it doesn't really need to be).
 *
 * This is starting to look an awful lot like a quad pipeline stage...
 */
struct sp_fragment_shader {
   struct pipe_shader_state shader;

   struct tgsi_shader_info info;

   boolean origin_lower_left; /**< fragment shader uses lower left position origin? */
   boolean pixel_center_integer; /**< fragment shader uses integer pixel center? */

   void (*prepare)( const struct sp_fragment_shader *shader,
		    struct tgsi_exec_machine *machine,
		    struct tgsi_sampler **samplers);

   /* Run the shader - this interface will get cleaned up in the
    * future:
    */
   unsigned (*run)( const struct sp_fragment_shader *shader,
		    struct tgsi_exec_machine *machine,
		    struct quad_header *quad );


   void (*delete)( struct sp_fragment_shader * );
};


/** Subclass of pipe_shader_state */
struct sp_vertex_shader {
   struct pipe_shader_state shader;
   struct draw_vertex_shader *draw_data;
   int max_sampler;             /* -1 if no samplers */
};

/** Subclass of pipe_shader_state */
struct sp_geometry_shader {
   struct pipe_shader_state shader;
   struct draw_geometry_shader *draw_data;
};


void *
softpipe_create_blend_state(struct pipe_context *,
                            const struct pipe_blend_state *);
void softpipe_bind_blend_state(struct pipe_context *,
                               void *);
void softpipe_delete_blend_state(struct pipe_context *,
                                 void *);

void *
softpipe_create_sampler_state(struct pipe_context *,
                              const struct pipe_sampler_state *);
void softpipe_bind_sampler_states(struct pipe_context *, unsigned, void **);
void
softpipe_bind_vertex_sampler_states(struct pipe_context *,
                                    unsigned num_samplers,
                                    void **samplers);
void softpipe_delete_sampler_state(struct pipe_context *, void *);

void *
softpipe_create_depth_stencil_state(struct pipe_context *,
                                    const struct pipe_depth_stencil_alpha_state *);
void softpipe_bind_depth_stencil_state(struct pipe_context *, void *);
void softpipe_delete_depth_stencil_state(struct pipe_context *, void *);

void *
softpipe_create_rasterizer_state(struct pipe_context *,
                                 const struct pipe_rasterizer_state *);
void softpipe_bind_rasterizer_state(struct pipe_context *, void *);
void softpipe_delete_rasterizer_state(struct pipe_context *, void *);

void softpipe_set_framebuffer_state( struct pipe_context *,
                                     const struct pipe_framebuffer_state * );

void softpipe_set_blend_color( struct pipe_context *pipe,
                               const struct pipe_blend_color *blend_color );

void softpipe_set_stencil_ref( struct pipe_context *pipe,
                               const struct pipe_stencil_ref *stencil_ref );

void softpipe_set_clip_state( struct pipe_context *,
                              const struct pipe_clip_state * );

void softpipe_set_constant_buffer(struct pipe_context *,
                                  uint shader, uint index,
                                  struct pipe_buffer *buf);

void *softpipe_create_fs_state(struct pipe_context *,
                               const struct pipe_shader_state *);
void softpipe_bind_fs_state(struct pipe_context *, void *);
void softpipe_delete_fs_state(struct pipe_context *, void *);
void *softpipe_create_vs_state(struct pipe_context *,
                               const struct pipe_shader_state *);
void softpipe_bind_vs_state(struct pipe_context *, void *);
void softpipe_delete_vs_state(struct pipe_context *, void *);
void *softpipe_create_gs_state(struct pipe_context *,
                               const struct pipe_shader_state *);
void softpipe_bind_gs_state(struct pipe_context *, void *);
void softpipe_delete_gs_state(struct pipe_context *, void *);

void softpipe_set_polygon_stipple( struct pipe_context *,
				  const struct pipe_poly_stipple * );

void softpipe_set_scissor_state( struct pipe_context *,
                                 const struct pipe_scissor_state * );

void softpipe_set_sampler_textures( struct pipe_context *,
                                    unsigned num,
                                    struct pipe_texture ** );

void
softpipe_set_vertex_sampler_textures(struct pipe_context *,
                                     unsigned num_textures,
                                     struct pipe_texture **);

void softpipe_set_viewport_state( struct pipe_context *,
                                  const struct pipe_viewport_state * );

void softpipe_set_vertex_elements(struct pipe_context *,
                                  unsigned count,
                                  const struct pipe_vertex_element *);

void softpipe_set_vertex_buffers(struct pipe_context *,
                                 unsigned count,
                                 const struct pipe_vertex_buffer *);


void softpipe_update_derived( struct softpipe_context *softpipe );


void softpipe_draw_arrays(struct pipe_context *pipe, unsigned mode,
                          unsigned start, unsigned count);

void softpipe_draw_elements(struct pipe_context *pipe,
                            struct pipe_buffer *indexBuffer,
                            unsigned indexSize,
                            unsigned mode, unsigned start, unsigned count);
void
softpipe_draw_range_elements(struct pipe_context *pipe,
                             struct pipe_buffer *indexBuffer,
                             unsigned indexSize,
                             unsigned min_index,
                             unsigned max_index,
                             unsigned mode, unsigned start, unsigned count);

void
softpipe_draw_arrays_instanced(struct pipe_context *pipe,
                               unsigned mode,
                               unsigned start,
                               unsigned count,
                               unsigned startInstance,
                               unsigned instanceCount);

void
softpipe_draw_elements_instanced(struct pipe_context *pipe,
                                 struct pipe_buffer *indexBuffer,
                                 unsigned indexSize,
                                 unsigned mode,
                                 unsigned start,
                                 unsigned count,
                                 unsigned startInstance,
                                 unsigned instanceCount);

void
softpipe_map_transfers(struct softpipe_context *sp);

void
softpipe_unmap_transfers(struct softpipe_context *sp);

void
softpipe_map_texture_surfaces(struct softpipe_context *sp);

void
softpipe_unmap_texture_surfaces(struct softpipe_context *sp);


struct vertex_info *
softpipe_get_vertex_info(struct softpipe_context *softpipe);

struct vertex_info *
softpipe_get_vbuf_vertex_info(struct softpipe_context *softpipe);


#endif
