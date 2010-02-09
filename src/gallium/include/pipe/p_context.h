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
struct pipe_fence_handle;
struct pipe_state_cache;
struct pipe_query;
struct pipe_winsys;

/**
 * Gallium rendering context.  Basically:
 *  - state setting functions
 *  - VBO drawing functions
 *  - surface functions
 */
struct pipe_context {
   struct pipe_winsys *winsys;
   struct pipe_screen *screen;

   void *priv;  /**< context private data (for DRI for example) */
   void *draw;  /**< private, for draw module (temporary?) */

   void (*destroy)( struct pipe_context * );

   /**
    * VBO drawing (return false on fallbacks (temporary??))
    */
   /*@{*/
   void (*draw_arrays)( struct pipe_context *pipe,
                        unsigned mode, unsigned start, unsigned count);

   void (*draw_elements)( struct pipe_context *pipe,
                          struct pipe_buffer *indexBuffer,
                          unsigned indexSize,
                          unsigned mode, unsigned start, unsigned count);

   void (*draw_arrays_instanced)(struct pipe_context *pipe,
                                 unsigned mode,
                                 unsigned start,
                                 unsigned count,
                                 unsigned startInstance,
                                 unsigned instanceCount);

   void (*draw_elements_instanced)(struct pipe_context *pipe,
                                   struct pipe_buffer *indexBuffer,
                                   unsigned indexSize,
                                   unsigned mode,
                                   unsigned start,
                                   unsigned count,
                                   unsigned startInstance,
                                   unsigned instanceCount);

   /* XXX: this is (probably) a temporary entrypoint, as the range
    * information should be available from the vertex_buffer state.
    * Using this to quickly evaluate a specialized path in the draw
    * module.
    */
   void (*draw_range_elements)( struct pipe_context *pipe,
                                struct pipe_buffer *indexBuffer,
                                unsigned indexSize,
                                unsigned minIndex,
                                unsigned maxIndex,
                                unsigned mode, 
                                unsigned start, 
                                unsigned count);
   /*@}*/

   /**
    * Predicate subsequent rendering on occlusion query result
    * \param query  the query predicate, or NULL if no predicate
    * \param mode  one of PIPE_RENDER_COND_x
    */
   void (*render_condition)( struct pipe_context *pipe,
                             struct pipe_query *query,
                             uint mode );

   /**
    * Query objects
    */
   /*@{*/
   struct pipe_query *(*create_query)( struct pipe_context *pipe,
                                       unsigned query_type );

   void (*destroy_query)(struct pipe_context *pipe,
                         struct pipe_query *q);

   void (*begin_query)(struct pipe_context *pipe, struct pipe_query *q);
   void (*end_query)(struct pipe_context *pipe, struct pipe_query *q);

   /**
    * Get results of a query.
    * \param wait  if true, this query will block until the result is ready
    * \return TRUE if results are ready, FALSE otherwise
    */
   boolean (*get_query_result)(struct pipe_context *pipe, 
                               struct pipe_query *q,
                               boolean wait,
                               uint64_t *result);
   /*@}*/

   /**
    * State functions (create/bind/destroy state objects)
    */
   /*@{*/
   void * (*create_blend_state)(struct pipe_context *,
                                const struct pipe_blend_state *);
   void   (*bind_blend_state)(struct pipe_context *, void *);
   void   (*delete_blend_state)(struct pipe_context *, void  *);

   void * (*create_sampler_state)(struct pipe_context *,
                                  const struct pipe_sampler_state *);
   void   (*bind_fragment_sampler_states)(struct pipe_context *,
                                          unsigned num_samplers,
                                          void **samplers);
   void   (*bind_vertex_sampler_states)(struct pipe_context *,
                                        unsigned num_samplers,
                                        void **samplers);
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

   void * (*create_gs_state)(struct pipe_context *,
                             const struct pipe_shader_state *);
   void   (*bind_gs_state)(struct pipe_context *, void *);
   void   (*delete_gs_state)(struct pipe_context *, void *);

   /*@}*/

   /**
    * Parameter-like state (or properties)
    */
   /*@{*/
   void (*set_blend_color)( struct pipe_context *,
                            const struct pipe_blend_color * );

   void (*set_stencil_ref)( struct pipe_context *,
                            const struct pipe_stencil_ref * );

   void (*set_clip_state)( struct pipe_context *,
                            const struct pipe_clip_state * );

   void (*set_constant_buffer)( struct pipe_context *,
                                uint shader, uint index,
                                struct pipe_buffer *buf );

   void (*set_framebuffer_state)( struct pipe_context *,
                                  const struct pipe_framebuffer_state * );

   void (*set_polygon_stipple)( struct pipe_context *,
				const struct pipe_poly_stipple * );

   void (*set_scissor_state)( struct pipe_context *,
                              const struct pipe_scissor_state * );

   void (*set_viewport_state)( struct pipe_context *,
                               const struct pipe_viewport_state * );

   void (*set_fragment_sampler_textures)(struct pipe_context *,
                                         unsigned num_textures,
                                         struct pipe_texture **);

   void (*set_vertex_sampler_textures)(struct pipe_context *,
                                       unsigned num_textures,
                                       struct pipe_texture **);

   void (*set_vertex_buffers)( struct pipe_context *,
                               unsigned num_buffers,
                               const struct pipe_vertex_buffer * );

   void (*set_vertex_elements)( struct pipe_context *,
                                unsigned num_elements,
                                const struct pipe_vertex_element * );
   /*@}*/


   /**
    * Surface functions
    *
    * The pipe driver is allowed to set these functions to NULL, and in that
    * case, they will not be available.
    */
   /*@{*/

   /**
    * Copy a block of pixels from one surface to another.
    * The surfaces must be of the same format.
    */
   void (*surface_copy)(struct pipe_context *pipe,
			struct pipe_surface *dest,
			unsigned destx, unsigned desty,
			struct pipe_surface *src,
			unsigned srcx, unsigned srcy,
			unsigned width, unsigned height);

   /**
    * Fill a region of a surface with a constant value.
    */
   void (*surface_fill)(struct pipe_context *pipe,
			struct pipe_surface *dst,
			unsigned dstx, unsigned dsty,
			unsigned width, unsigned height,
			unsigned value);
   /*@}*/

   /**
    * Clear the specified set of currently bound buffers to specified values.
    * The entire buffers are cleared (no scissor, no colormask, etc).
    *
    * \param buffers  bitfield of PIPE_CLEAR_* values.
    * \param rgba  pointer to an array of one float for each of r, g, b, a.
    * \param depth  depth clear value in [0,1].
    * \param stencil  stencil clear value
    */
   void (*clear)(struct pipe_context *pipe,
                 unsigned buffers,
		 const float *rgba,
                 double depth,
		 unsigned stencil);

   /** Flush rendering
    * \param flags  bitmask of PIPE_FLUSH_x tokens)
    */
   void (*flush)( struct pipe_context *pipe,
                  unsigned flags,
                  struct pipe_fence_handle **fence );

   /**
    * Check whether a texture is referenced by an unflushed hw command.
    * The state-tracker uses this function to avoid unnecessary flushes.
    * It is safe (but wasteful) to always return
    * PIPE_REFERENCED_FOR_READ | PIPE_REFERENCED_FOR_WRITE.
    * \param pipe  context whose unflushed hw commands will be checked.
    * \param texture  texture to check.
    * \param face  cubemap face. Use 0 for non-cubemap texture.
    * \param level  mipmap level.
    * \return mask of PIPE_REFERENCED_FOR_READ/WRITE or PIPE_UNREFERENCED
    */
   unsigned int (*is_texture_referenced)(struct pipe_context *pipe,
					 struct pipe_texture *texture,
					 unsigned face, unsigned level);

   /**
    * Check whether a buffer is referenced by an unflushed hw command.
    * The state-tracker uses this function to avoid unnecessary flushes.
    * It is safe (but wasteful) to always return
    * PIPE_REFERENCED_FOR_READ | PIPE_REFERENCED_FOR_WRITE.
    * \param pipe  context whose unflushed hw commands will be checked.
    * \param buf  buffer to check.
    * \return mask of PIPE_REFERENCED_FOR_READ/WRITE or PIPE_UNREFERENCED
    */
   unsigned int (*is_buffer_referenced)(struct pipe_context *pipe,
					struct pipe_buffer *buf);
};


#ifdef __cplusplus
}
#endif

#endif /* PIPE_CONTEXT_H */
