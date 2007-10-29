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

#ifndef PIPE_CONTEXT_H
#define PIPE_CONTEXT_H

#include "p_state.h"

struct pipe_state_cache;
/**
 * Software pipeline rendering context.  Basically a collection of
 * state setting functions, plus VBO drawing entrypoint.
 */
struct pipe_context {
   struct pipe_winsys *winsys;

   void (*destroy)( struct pipe_context * );

   /*
    * Queries
    */
   boolean (*is_format_supported)( struct pipe_context *pipe,
                                   uint format );

   void (*max_texture_size)(struct pipe_context *pipe,
                            unsigned textureType, /* PIPE_TEXTURE_x */
                            unsigned *maxWidth,
                            unsigned *maxHeight,
                            unsigned *maxDepth);

   const char *(*get_name)( struct pipe_context *pipe );

   const char *(*get_vendor)( struct pipe_context *pipe );

   int (*get_param)( struct pipe_context *pipe, int param );


   /*
    * Drawing.  
    * Return false on fallbacks (temporary??)
    */
   boolean (*draw_arrays)( struct pipe_context *pipe,
			   unsigned mode, unsigned start, unsigned count);

   boolean (*draw_elements)( struct pipe_context *pipe,
			     struct pipe_buffer_handle *indexBuffer,
			     unsigned indexSize,
			     unsigned mode, unsigned start, unsigned count);

   /** Clear a surface to given value (no scissor; clear whole surface) */
   void (*clear)(struct pipe_context *pipe, struct pipe_surface *ps,
                 unsigned clearValue);

   /**
    * Query objects
    */
   void (*begin_query)(struct pipe_context *pipe, struct pipe_query_object *q);
   void (*end_query)(struct pipe_context *pipe, struct pipe_query_object *q);
   void (*wait_query)(struct pipe_context *pipe, struct pipe_query_object *q);

   /*
    * State functions
    */
   void * (*create_alpha_test_state)(struct pipe_context *,
                                     const struct pipe_alpha_test_state *);
   void   (*bind_alpha_test_state)(struct pipe_context *, void *);
   void   (*delete_alpha_test_state)(struct pipe_context *, void *);

   void * (*create_blend_state)(struct pipe_context *,
                                const struct pipe_blend_state *);
   void   (*bind_blend_state)(struct pipe_context *, void *);
   void   (*delete_blend_state)(struct pipe_context *, void  *);

   void * (*create_sampler_state)(struct pipe_context *,
                                  const struct pipe_sampler_state *);
   void   (*bind_sampler_state)(struct pipe_context *, unsigned unit, void *);
   void   (*delete_sampler_state)(struct pipe_context *, void *);

   void * (*create_rasterizer_state)(struct pipe_context *,
                                     const struct pipe_rasterizer_state *);
   void   (*bind_rasterizer_state)(struct pipe_context *, void *);
   void   (*delete_rasterizer_state)(struct pipe_context *, void *);

   void * (*create_depth_stencil_state)(struct pipe_context *,
                                        const struct pipe_depth_stencil_state *);
   void   (*bind_depth_stencil_state)(struct pipe_context *, void *);
   void   (*delete_depth_stencil_state)(struct pipe_context *, void *);

   void * (*create_fs_state)(struct pipe_context *,
                             const struct pipe_shader_state *);
   void   (*bind_fs_state)(struct pipe_context *, void *);
   void   (*delete_fs_state)(struct pipe_context *, void *);

   void * (*create_vs_state)(struct pipe_context *,
                             const struct pipe_shader_state *);
   void   (*bind_vs_state)(struct pipe_context *, void *);
   void   (*delete_vs_state)(struct pipe_context *, void *);

   /* The following look more properties than states.
    * maybe combine a few of them into states or pass them
    * in the bind calls to the state */
   void (*set_blend_color)( struct pipe_context *,
                            const struct pipe_blend_color * );

   void (*set_clip_state)( struct pipe_context *,
			   const struct pipe_clip_state * );

   void (*set_clear_color_state)( struct pipe_context *,
                                  const struct pipe_clear_color_state * );

   void (*set_constant_buffer)( struct pipe_context *,
                                uint shader, uint index,
                                const struct pipe_constant_buffer *buf );

   void (*set_feedback_state)( struct pipe_context *,
                               const struct pipe_feedback_state *);

   void (*set_framebuffer_state)( struct pipe_context *,
                                  const struct pipe_framebuffer_state * );

   void (*set_polygon_stipple)( struct pipe_context *,
				const struct pipe_poly_stipple * );

   void (*set_sampler_units)( struct pipe_context *,
                              uint num_samplers, const uint *units );

   void (*set_scissor_state)( struct pipe_context *,
                              const struct pipe_scissor_state * );

   void (*set_texture_state)( struct pipe_context *,
                              unsigned unit,
                              struct pipe_mipmap_tree * );

   void (*set_viewport_state)( struct pipe_context *,
                               const struct pipe_viewport_state * );

   /*
    * Vertex arrays
    */
   void (*set_vertex_buffer)( struct pipe_context *,
                              unsigned index,
                              const struct pipe_vertex_buffer * );

   void (*set_vertex_element)( struct pipe_context *,
			       unsigned index,
			       const struct pipe_vertex_element * );

   /*
    * Vertex feedback
    */
   void (*set_feedback_buffer)(struct pipe_context *,
                               unsigned index,
                               const struct pipe_feedback_buffer *);

   /** Get a surface which is a "view" into a texture */
   struct pipe_surface *(*get_tex_surface)(struct pipe_context *pipe,
                                           struct pipe_mipmap_tree *texture,
                                           unsigned face, unsigned level,
                                           unsigned zslice);

   /** Get a block of raw pixel data from a surface */
   void (*get_tile)(struct pipe_context *pipe,
                    struct pipe_surface *ps,
                    uint x, uint y, uint w, uint h,
                    void *p, int dst_stride);
   /** Put a block of raw pixel data into a surface */
   void (*put_tile)(struct pipe_context *pipe,
                    struct pipe_surface *ps,
                    uint x, uint y, uint w, uint h,
                    const void *p, int src_stride);
   /* XXX temporary here, move these to softpipe */
   void (*get_tile_rgba)(struct pipe_context *pipe, struct pipe_surface *ps,
                         uint x, uint y, uint w, uint h, float *p);
   void (*put_tile_rgba)(struct pipe_context *pipe, struct pipe_surface *ps,
                         uint x, uint y, uint w, uint h, const float *p);


   /*
    * Memory region functions
    */
   ubyte *(*region_map)(struct pipe_context *pipe, struct pipe_region *r);

   void (*region_unmap)(struct pipe_context *pipe, struct pipe_region *r);

   void (*region_data)(struct pipe_context *pipe,
                       struct pipe_region *dest,
                       unsigned dest_offset,
                       unsigned destx, unsigned desty,
                       const void *src, unsigned src_stride,
                       unsigned srcx, unsigned srcy,
                       unsigned width, unsigned height);

   void (*region_copy)(struct pipe_context *pipe,
                       struct pipe_region *dest,
                       unsigned dest_offset,
                       unsigned destx, unsigned desty,
                       struct pipe_region *src,	/* don't make this const - 
						   need to map/unmap */
                       unsigned src_offset,
                       unsigned srcx, unsigned srcy,
                       unsigned width, unsigned height);

   void (*region_fill)(struct pipe_context *pipe,
                       struct pipe_region *dst,
                       unsigned dst_offset,
                       unsigned dstx, unsigned dsty,
                       unsigned width, unsigned height,
                       unsigned value);


   /*
    * Texture functions
    */
   boolean (*mipmap_tree_layout)( struct pipe_context *pipe,
				  struct pipe_mipmap_tree *mt );


   /* Flush rendering:
    */
   void (*flush)( struct pipe_context *pipe,
		  unsigned flags );
};

#endif /* PIPE_CONTEXT_H */
