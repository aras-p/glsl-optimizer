/**************************************************************************
 * 
 * Copyright 2009 Younes Manton.
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

#ifndef vl_compositor_h
#define vl_compositor_h

#include <pipe/p_compiler.h>
#include <pipe/p_state.h>
#include <pipe/p_video_state.h>

struct pipe_context;
struct pipe_texture;

struct vl_compositor
{
   struct pipe_context *pipe;

   struct pipe_framebuffer_state fb_state;
   void *sampler;
   void *vertex_shader;
   void *fragment_shader;
   struct pipe_viewport_state viewport;
   struct pipe_scissor_state scissor;
   struct pipe_vertex_buffer vertex_bufs[2];
   struct pipe_vertex_element vertex_elems[2];
   struct pipe_buffer *vs_const_buf, *fs_const_buf;
};

bool vl_compositor_init(struct vl_compositor *compositor, struct pipe_context *pipe);

void vl_compositor_cleanup(struct vl_compositor *compositor);

void vl_compositor_render(struct vl_compositor          *compositor,
                          /*struct pipe_texture         *backround,
                          struct pipe_video_rect        *backround_area,*/
                          struct pipe_texture           *src_surface,
                          enum pipe_mpeg12_picture_type picture_type,
                          /*unsigned                    num_past_surfaces,
                          struct pipe_texture           *past_surfaces,
                          unsigned                      num_future_surfaces,
                          struct pipe_texture           *future_surfaces,*/
                          struct pipe_video_rect        *src_area,
                          struct pipe_texture           *dst_surface,
                          struct pipe_video_rect        *dst_area,
                          /*unsigned                      num_layers,
                          struct pipe_texture           *layers,
                          struct pipe_video_rect        *layer_src_areas,
                          struct pipe_video_rect        *layer_dst_areas,*/
                          struct pipe_fence_handle      **fence);

void vl_compositor_set_csc_matrix(struct vl_compositor *compositor, const float *mat);

#endif /* vl_compositor_h */
