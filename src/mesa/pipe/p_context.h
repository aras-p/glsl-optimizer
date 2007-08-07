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

#include "main/mtypes.h"
#include "p_state.h"


/* Drawing currently kludged up via the existing tnl/ module.  
 */
struct vertex_buffer;


/**
 * Software pipeline rendering context.  Basically a collection of
 * state setting functions, plus VBO drawing entrypoint.
 */
struct pipe_context {

   void (*destroy)( struct pipe_context * );

   /*
    * Queries
    */
   const GLuint *(*supported_formats)(struct pipe_context *pipe,
                                      GLuint *numFormats);

   /*
    * Drawing
    */
   void (*draw_vb)( struct pipe_context *pipe,
		    struct vertex_buffer *VB );

   void (*draw_vertices)( struct pipe_context *pipe,
                          GLuint mode,
                          GLuint numVertex, const GLfloat *verts,
                          GLuint numAttribs, const GLuint attribs[]);

   /** Clear a surface to given value (no scissor; clear whole surface) */
   void (*clear)(struct pipe_context *pipe, struct pipe_surface *ps,
                 GLuint clearValue);

   /** occlusion counting (XXX this may be temporary - we should probably
    * have generic query objects with begin/end methods)
    */
   void (*reset_occlusion_counter)(struct pipe_context *pipe);
   GLuint (*get_occlusion_counter)(struct pipe_context *pipe);

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

   void (*set_depth_state)( struct pipe_context *,
                              const struct pipe_depth_state * );

   void (*set_framebuffer_state)( struct pipe_context *,
                                  const struct pipe_framebuffer_state * );

   void (*set_fs_state)( struct pipe_context *,
			 const struct pipe_fs_state * );

   void (*set_polygon_stipple)( struct pipe_context *,
				const struct pipe_poly_stipple * );

   void (*set_setup_state)( struct pipe_context *,
			    const struct pipe_setup_state * );

   void (*set_scissor_state)( struct pipe_context *,
                              const struct pipe_scissor_state * );

   void (*set_stencil_state)( struct pipe_context *,
                              const struct pipe_stencil_state * );

   void (*set_sampler_state)( struct pipe_context *,
                              GLuint unit,
                              const struct pipe_sampler_state * );

   void (*set_texture_state)( struct pipe_context *,
                              GLuint unit,
                              struct pipe_mipmap_tree * );

   void (*set_viewport_state)( struct pipe_context *,
                               const struct pipe_viewport_state * );


   /*
    * Surface functions
    * This might go away...
    */
   struct pipe_surface *(*surface_alloc)(struct pipe_context *pipe,
                                         GLuint format);


   /*
    * Memory region functions
    * Some of these may go away...
    */
   struct pipe_region *(*region_alloc)(struct pipe_context *pipe,
                                       GLuint cpp, GLuint pitch, GLuint height);

   void (*region_release)(struct pipe_context *pipe, struct pipe_region **r);

   void (*region_idle)(struct pipe_context *pipe, struct pipe_region *region);

   GLubyte *(*region_map)(struct pipe_context *pipe, struct pipe_region *r);

   void (*region_unmap)(struct pipe_context *pipe, struct pipe_region *r);

   void (*region_data)(struct pipe_context *pipe,
                       struct pipe_region *dest,
                       GLuint dest_offset,
                       GLuint destx, GLuint desty,
                       const void *src, GLuint src_stride,
                       GLuint srcx, GLuint srcy, GLuint width, GLuint height);

   void (*region_copy)(struct pipe_context *pipe,
                       struct pipe_region *dest,
                       GLuint dest_offset,
                       GLuint destx, GLuint desty,
                       struct pipe_region *src,	/* don't make this const - 
						   need to map/unmap */
                       GLuint src_offset,
                       GLuint srcx, GLuint srcy, GLuint width, GLuint height);

   void (*region_fill)(struct pipe_context *pipe,
                       struct pipe_region *dst,
                       GLuint dst_offset,
                       GLuint dstx, GLuint dsty,
                       GLuint width, GLuint height,
                       GLuint value);


   /* Buffer management functions need to be exposed as well.  A pipe
    * buffer may be used as a texture, render target or vertex/index
    * buffer, or some combination according to flags.  
    */

   struct pipe_buffer_handle *(*create_buffer)(struct pipe_context *pipe, 
					       unsigned alignment,
					       unsigned flags );

   void *(*buffer_map)( struct pipe_context *pipe, 
			struct pipe_buffer_handle *buf,
			unsigned flags );
   
   void (*buffer_unmap)( struct pipe_context *pipe, 
			 struct pipe_buffer_handle *buf );

   struct pipe_buffer_handle *(*buffer_reference)( struct pipe_context *pipe,
						   struct pipe_buffer_handle *buf );

   void (*buffer_unreference)( struct pipe_context *pipe, 
			       struct pipe_buffer_handle **buf );

   void (*buffer_data)(struct pipe_context *pipe, 
		       struct pipe_buffer_handle *buf,
		       unsigned size, const void *data );

   void (*buffer_subdata)(struct pipe_context *pipe, 
			  struct pipe_buffer_handle *buf,
			  unsigned long offset, 
			  unsigned long size, 
			  const void *data);

   void (*buffer_get_subdata)(struct pipe_context *pipe, 
			      struct pipe_buffer_handle *buf,
			      unsigned long offset, 
			      unsigned long size, 
			      void *data);

   /*
    * Texture functions
    */
   GLboolean (*mipmap_tree_layout)( struct pipe_context *pipe,
                                    struct pipe_mipmap_tree *mt );


};



static INLINE void
pipe_region_reference(struct pipe_region **dst, struct pipe_region *src)
{
   assert(*dst == NULL);
   if (src) {
      src->refcount++;
      *dst = src;
   }
}



#endif /* PIPE_CONTEXT_H */
