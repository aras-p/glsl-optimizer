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


#ifdef __cplusplus
extern "C" {
#endif

   
struct pipe_screen;

struct pipe_state_cache;

/* Opaque driver handles:
 */
struct pipe_query;

/**
 * Gallium rendering context.  Basically:
 *  - state setting functions
 *  - VBO drawing functions
 *  - surface functions
 *  - device queries
 */
struct pipe_context {
   struct pipe_winsys *winsys;
   struct pipe_screen *screen;

   void *priv;  /** context private data (for DRI for example) */
   void *draw;  /** private, for draw module (temporary? */

   void (*destroy)( struct pipe_context * );

   /*
    * Drawing.  
    * Return false on fallbacks (temporary??)
    */
   boolean (*draw_arrays)( struct pipe_context *pipe,
			   unsigned mode, unsigned start, unsigned count);

   boolean (*draw_elements)( struct pipe_context *pipe,
			     struct pipe_buffer *indexBuffer,
			     unsigned indexSize,
			     unsigned mode, unsigned start, unsigned count);


   /**
    * Query objects
    */
   struct pipe_query *(*create_query)( struct pipe_context *pipe,
                                              unsigned query_type );

   void (*destroy_query)(struct pipe_context *pipe,
                         struct pipe_query *q);

   void (*begin_query)(struct pipe_context *pipe, struct pipe_query *q);
   void (*end_query)(struct pipe_context *pipe, struct pipe_query *q);

   boolean (*get_query_result)(struct pipe_context *pipe, 
                               struct pipe_query *q,
                               boolean wait,
                               uint64 *result);

   /*
    * State functions
    */
   void * (*create_blend_state)(struct pipe_context *,
                                const struct pipe_blend_state *);
   void   (*bind_blend_state)(struct pipe_context *, void *);
   void   (*delete_blend_state)(struct pipe_context *, void  *);

   void * (*create_sampler_state)(struct pipe_context *,
                                  const struct pipe_sampler_state *);
   void   (*bind_sampler_states)(struct pipe_context *, unsigned num, void **);
   void   (*delete_sampler_state)(struct pipe_context *, void *);

   void * (*create_rasterizer_state)(struct pipe_context *,
                                     const struct pipe_rasterizer_state *);
   void   (*bind_rasterizer_state)(struct pipe_context *, void *);
   void   (*delete_rasterizer_state)(struct pipe_context *, void *);

   void * (*create_depth_stencil_alpha_state)(struct pipe_context *,
                                        const struct pipe_depth_stencil_alpha_state *);
   void   (*bind_depth_stencil_alpha_state)(struct pipe_context *, void *);
   void   (*delete_depth_stencil_alpha_state)(struct pipe_context *, void *);

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

   void (*set_constant_buffer)( struct pipe_context *,
                                uint shader, uint index,
                                const struct pipe_constant_buffer *buf );

   void (*set_framebuffer_state)( struct pipe_context *,
                                  const struct pipe_framebuffer_state * );

   void (*set_polygon_stipple)( struct pipe_context *,
				const struct pipe_poly_stipple * );

   void (*set_scissor_state)( struct pipe_context *,
                              const struct pipe_scissor_state * );


   /* Currently a sampler is constrained to sample from a single texture:
    */
   void (*set_sampler_textures)( struct pipe_context *,
                                 unsigned num,
                                 struct pipe_texture ** );

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
    * Surface functions
    */

   void (*surface_copy)(struct pipe_context *pipe,
                        unsigned do_flip,	/*<< flip surface contents vertically */
			struct pipe_surface *dest,
			unsigned destx, unsigned desty,
			struct pipe_surface *src, /* don't make this const - 
						     need to map/unmap */
			unsigned srcx, unsigned srcy,
			unsigned width, unsigned height);

   void (*surface_fill)(struct pipe_context *pipe,
			struct pipe_surface *dst,
			unsigned dstx, unsigned dsty,
			unsigned width, unsigned height,
			unsigned value);

   void (*clear)(struct pipe_context *pipe, 
		 struct pipe_surface *ps,
		 unsigned clearValue);

   /**
    * Called when texture data is changed.
    */
   void (*texture_update)(struct pipe_context *pipe,
                          struct pipe_texture *texture,
                          uint face, uint dirtyLevelsMask);



   /* Flush rendering:
    */
   void (*flush)( struct pipe_context *pipe,
		  unsigned flags );
};


#ifdef __cplusplus
}
#endif

#endif /* PIPE_CONTEXT_H */
