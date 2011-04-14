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

#ifndef vl_mpeg12_mc_renderer_h
#define vl_mpeg12_mc_renderer_h

#include <pipe/p_state.h>
#include <pipe/p_video_state.h>

#include "vl_types.h"

struct pipe_context;

struct vl_mpeg12_mc_renderer
{
   struct pipe_context *pipe;
   unsigned buffer_width;
   unsigned buffer_height;

   struct pipe_viewport_state viewport;
   struct pipe_framebuffer_state fb_state;

   void *rs_state;

   void *blend_clear, *blend_add;
   void *vs, *fs_ref, *fs_ycbcr;
   void *sampler_ref, *sampler_ycbcr;

   struct pipe_sampler_view *dummy;
};

struct vl_mpeg12_mc_buffer
{
   struct vl_mpeg12_mc_renderer *renderer;
   struct pipe_sampler_view *source;
};

bool vl_mc_init(struct vl_mpeg12_mc_renderer *renderer, struct pipe_context *pipe,
                unsigned picture_width, unsigned picture_height, float scale);

void vl_mc_cleanup(struct vl_mpeg12_mc_renderer *renderer);

bool vl_mc_init_buffer(struct vl_mpeg12_mc_renderer *renderer, struct vl_mpeg12_mc_buffer *buffer,
                       struct pipe_sampler_view *source);

void vl_mc_cleanup_buffer(struct vl_mpeg12_mc_buffer *buffer);

void vl_mc_set_surface(struct vl_mpeg12_mc_renderer *renderer, struct pipe_surface *surface);

void vl_mc_render_ref(struct vl_mpeg12_mc_buffer *buffer,
                      struct pipe_sampler_view *ref, bool first,
                      unsigned not_empty_start_instance, unsigned not_empty_num_instances,
                      unsigned empty_start_instance, unsigned empty_num_instances);

void vl_mc_render_ycbcr(struct vl_mpeg12_mc_buffer *buffer, bool first,
                        unsigned not_empty_start_instance, unsigned not_empty_num_instances);

#endif /* vl_mpeg12_mc_renderer_h */
