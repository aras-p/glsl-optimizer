/**************************************************************************
 *
 * Copyright 2011 Christian König.
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

#ifndef vl_ycbcr_buffer_h
#define vl_ycbcr_buffer_h

#include <pipe/p_state.h>

#define VL_MAX_PLANES 3

/**
 * implementation of a planar ycbcr buffer
 */

/* resources of a buffer */
typedef struct pipe_resource *vl_resources[VL_MAX_PLANES];

/* sampler views of a buffer */
typedef struct pipe_sampler_view *vl_sampler_views[VL_MAX_PLANES];

/* surfaces of a buffer */
typedef struct pipe_surface *vl_surfaces[VL_MAX_PLANES];

/* planar buffer for vl data upload and manipulation */
struct vl_video_buffer
{
   struct pipe_context *pipe;
   unsigned            num_planes;
   vl_resources        resources;
   vl_sampler_views    sampler_views;
   vl_surfaces         surfaces;
};

/**
 * initialize a buffer, creating its resources
 */
bool vl_video_buffer_init(struct vl_video_buffer *buffer,
                          struct pipe_context *pipe,
                          unsigned width, unsigned height, unsigned depth,
                          enum pipe_video_chroma_format chroma_format,
                          unsigned num_planes,
                          const enum pipe_format resource_formats[VL_MAX_PLANES],
                          unsigned usage);

/**
 * create default sampler views for the buffer on demand
 */
vl_sampler_views *vl_video_buffer_sampler_views(struct vl_video_buffer *buffer);

/**
 * create default surfaces for the buffer on demand
 */
vl_surfaces *vl_video_buffer_surfaces(struct vl_video_buffer *buffer);

/**
 * cleanup the buffer destroying all its resources
 */
void vl_video_buffer_cleanup(struct vl_video_buffer *buffer);

#endif
