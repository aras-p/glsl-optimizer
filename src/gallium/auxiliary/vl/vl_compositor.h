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

#include <pipe/p_state.h>
#include <pipe/p_video_context.h>
#include <pipe/p_video_state.h>

struct pipe_context;

#define VL_COMPOSITOR_MAX_LAYERS 16

struct vl_compositor_layer
{
   void *fs;
   struct pipe_sampler_view *sampler_views[3];
   struct pipe_video_rect src_rect;
   struct pipe_video_rect dst_rect;
};

struct vl_compositor
{
   struct pipe_video_compositor base;
   struct pipe_context *pipe;

   struct pipe_framebuffer_state fb_state;
   struct pipe_viewport_state viewport;
   struct pipe_vertex_buffer vertex_buf;
   struct pipe_resource *csc_matrix;

   void *sampler;
   void *blend;
   void *rast;
   void *vertex_elems_state;

   void *vs;
   void *fs_video_buffer;
   void *fs_palette;
   void *fs_rgba;

   unsigned used_layers:VL_COMPOSITOR_MAX_LAYERS;
   struct vl_compositor_layer layers[VL_COMPOSITOR_MAX_LAYERS];
};

struct pipe_video_compositor *vl_compositor_init(struct pipe_video_context *vpipe,
                                                 struct pipe_context *pipe);

#endif /* vl_compositor_h */
