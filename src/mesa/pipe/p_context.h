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
#include "p_compiler.h"


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
   const unsigned *(*supported_formats)(struct pipe_context *pipe,
                                      unsigned *numFormats);

   void (*max_texture_size)(struct pipe_context *pipe,
                            unsigned textureType, /* PIPE_TEXTURE_x */
                            unsigned *maxWidth,
                            unsigned *maxHeight,
                            unsigned *maxDepth);

   const char *(*get_name)( struct pipe_context *pipe );

   const char *(*get_vendor)( struct pipe_context *pipe );

   

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

   /** occlusion counting (XXX this may be temporary - we should probably
    * have generic query objects with begin/end methods)
    */
   void (*reset_occlusion_counter)(struct pipe_context *pipe);
   unsigned (*get_occlusion_counter)(struct pipe_context *pipe);

   /*
    * State functions
    */
   void (*set_alpha_test_state)( struct pipe_context *,
                                 const struct pipe_alpha_test_state * );

   void (*set_blend_state)( struct pipe_context *,
                            const struct pipe_blend_state * );

   void (*set_blend_color)( struct pipe_context *,
                            const struct pipe_blend_color * );

   void (*set_clip_state)( struct pipe_context *,
			   const struct pipe_clip_state * );

   void (*set_clear_color_state)( struct pipe_context *,
                                  const struct pipe_clear_color_state * );

   void (*set_constant_buffer)( struct pipe_context *,
                                uint shader, uint index,
                                const struct pipe_constant_buffer *buf );
                              
   void (*set_depth_state)( struct pipe_context *,
                            const struct pipe_depth_state * );

   void (*set_feedback_state)( struct pipe_context *,
                               const struct pipe_feedback_state *);

   void (*set_framebuffer_state)( struct pipe_context *,
                                  const struct pipe_framebuffer_state * );

   void (*set_fs_state)( struct pipe_context *,
			 const struct pipe_shader_state * );

   void (*set_vs_state)( struct pipe_context *,
			 const struct pipe_shader_state * );

   void (*set_polygon_stipple)( struct pipe_context *,
				const struct pipe_poly_stipple * );

   void (*set_setup_state)( struct pipe_context *,
			    const struct pipe_setup_state * );

   void (*set_scissor_state)( struct pipe_context *,
                              const struct pipe_scissor_state * );

   void (*set_stencil_state)( struct pipe_context *,
                              const struct pipe_stencil_state * );

   void (*set_sampler_state)( struct pipe_context *,
                              unsigned unit,
                              const struct pipe_sampler_state * );

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

   /*
    * Surface functions
    * This might go away...
    */
   struct pipe_surface *(*surface_alloc)(struct pipe_context *pipe,
                                         unsigned format);

   struct pipe_surface *(*get_tex_surface)(struct pipe_context *pipe,
                                           struct pipe_mipmap_tree *texture,
                                           unsigned face, unsigned level,
                                           unsigned zslice);

   /*
    * Memory region functions
    * Some of these may go away...
    */
   struct pipe_region *(*region_alloc)(struct pipe_context *pipe,
                                       unsigned cpp, unsigned width, unsigned height,
                                       unsigned flags);

   void (*region_release)(struct pipe_context *pipe, struct pipe_region **r);

   void (*region_idle)(struct pipe_context *pipe, struct pipe_region *region);

   ubyte *(*region_map)(struct pipe_context *pipe, struct pipe_region *r);

   void (*region_unmap)(struct pipe_context *pipe, struct pipe_region *r);

   void (*region_data)(struct pipe_context *pipe,
                       struct pipe_region *dest,
                       unsigned dest_offset,
                       unsigned destx, unsigned desty,
                       const void *src, unsigned src_stride,
                       unsigned srcx, unsigned srcy, unsigned width, unsigned height);

   void (*region_copy)(struct pipe_context *pipe,
                       struct pipe_region *dest,
                       unsigned dest_offset,
                       unsigned destx, unsigned desty,
                       struct pipe_region *src,	/* don't make this const - 
						   need to map/unmap */
                       unsigned src_offset,
                       unsigned srcx, unsigned srcy, unsigned width, unsigned height);

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


/**
 * Set 'ptr' to point to 'region' and update reference counting.
 * The old thing pointed to, if any, will be unreferenced first.
 * 'region' may be NULL.
 */
static INLINE void
pipe_region_reference(struct pipe_region **ptr, struct pipe_region *region)
{
   assert(ptr);
   if (*ptr) {
      /* unreference the old thing */
      struct pipe_region *oldReg = *ptr;
      oldReg->refcount--;
      assert(oldReg->refcount >= 0);
      if (oldReg->refcount == 0) {
         /* free the old region */
         assert(oldReg->map_refcount == 0);
         /* XXX dereference the region->buffer */
         free(oldReg);
      }
      *ptr = NULL;
   }
   if (region) {
      /* reference the new thing */
      region->refcount++;
      *ptr = region;
   }
}


/**
 * \sa pipe_region_reference
 */
static INLINE void
pipe_surface_reference(struct pipe_surface **ptr, struct pipe_surface *surf)
{
   assert(ptr);
   if (*ptr) {
      /* unreference the old thing */
      struct pipe_surface *oldSurf = *ptr;
      oldSurf->refcount--;
      assert(oldSurf->refcount >= 0);
      if (oldSurf->refcount == 0) {
         /* free the old region */
         pipe_region_reference(&oldSurf->region, NULL);
         free(oldSurf);
      }
      *ptr = NULL;
   }
   if (surf) {
      /* reference the new thing */
      surf->refcount++;
      *ptr = surf;
   }
}


#endif /* PIPE_CONTEXT_H */
