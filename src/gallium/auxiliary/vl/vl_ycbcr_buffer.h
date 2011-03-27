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

/**
 * implementation of a planar ycbcr buffer
 */

/* resources of a buffer */
struct vl_ycbcr_resources
{
   struct pipe_resource *y, *cb, *cr;
};

/* sampler views of a buffer */
struct vl_ycbcr_sampler_views
{
   struct pipe_sampler_view *y, *cb, *cr;
};

/* surfaces of a buffer */
struct vl_ycbcr_surfaces
{
   struct pipe_surface *y, *cb, *cr;
};

/* planar buffer for vl data upload and manipulation */
struct vl_ycbcr_buffer
{
   struct pipe_context           *pipe;
   struct vl_ycbcr_resources     resources;
   struct vl_ycbcr_sampler_views sampler_views;
   struct vl_ycbcr_surfaces      surfaces;
};

/**
 * initialize a buffer, creating its resources
 */
bool vl_ycbcr_buffer_init(struct vl_ycbcr_buffer *buffer,
                          struct pipe_context *pipe,
                          unsigned width, unsigned height,
                          enum pipe_video_chroma_format chroma_format,
                          enum pipe_format resource_format,
                          unsigned usage);

/**
 * create default sampler views for the buffer on demand
 */
struct vl_ycbcr_sampler_views *vl_ycbcr_get_sampler_views(struct vl_ycbcr_buffer *buffer);

/**
 * create default surfaces for the buffer on demand
 */
struct vl_ycbcr_surfaces *vl_ycbcr_get_surfaces(struct vl_ycbcr_buffer *buffer);

/**
 * cleanup the buffer destroying all its resources
 */
void vl_ycbcr_buffer_cleanup(struct vl_ycbcr_buffer *buffer);

#endif
