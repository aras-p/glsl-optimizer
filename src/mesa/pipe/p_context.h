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


/* Kludge:
 */
extern struct pipe_context *softpipe_create( void );
				      
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
    * Drawing
    */
   void (*draw_vb)( struct pipe_context *pipe,
		    struct vertex_buffer *VB );

   /** Clear framebuffer */
   void (*clear)(struct pipe_context *pipe, GLboolean color, GLboolean depth,
                 GLboolean stencil, GLboolean accum);

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
                              struct pipe_texture_object * );

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

   struct pipe_region *(*region_create_static)(struct pipe_context *pipe,
                                               GLuint mem_type,
                                               GLuint offset,
                                               void *virtual,
                                               GLuint cpp, GLuint pitch,
                                               GLuint height);

   void (*region_update_static)(struct pipe_context *pipe,
                                struct pipe_region *region,
                                GLuint mem_type,
                                GLuint offset,
                                void *virtual,
                                GLuint cpp, GLuint pitch, GLuint height);

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
                       const struct pipe_region *src,
                       GLuint src_offset,
                       GLuint srcx, GLuint srcy, GLuint width, GLuint height);

   void (*region_fill)(struct pipe_context *pipe,
                       struct pipe_region *dst,
                       GLuint dst_offset,
                       GLuint dstx, GLuint dsty,
                       GLuint width, GLuint height, GLuint color);

   void (*region_cow)(struct pipe_context *pipe, struct pipe_region *region);

   void (*region_attach_pbo)(struct pipe_context *pipe,
                             struct pipe_region *region,
                             struct intel_buffer_object *pbo);

   void (*region_release_pbo)(struct pipe_context *pipe,
                              struct pipe_region *region);

   struct _DriBufferObject *(*region_buffer)(struct pipe_context *pipe,
                                             struct pipe_region *region,
                                             GLuint flag);

   void *screen;  /**< temporary */
   void *glctx;   /**< temporary */
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
