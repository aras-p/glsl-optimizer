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

#include <pipe/p_context.h>
#include <pipe/p_video_context.h>

#include "vl_defines.h"

/**
 * implementation of a planar ycbcr buffer
 */

/* planar buffer for vl data upload and manipulation */
struct vl_video_buffer
{
   struct pipe_video_buffer base;
   struct pipe_context      *pipe;
   unsigned                 num_planes;
   struct pipe_resource     *resources[VL_MAX_PLANES];
   struct pipe_sampler_view *sampler_view_planes[VL_MAX_PLANES];
   struct pipe_sampler_view *sampler_view_components[VL_MAX_PLANES];
   struct pipe_surface      *surfaces[VL_MAX_PLANES];
};

/**
 * initialize a buffer, creating its resources
 */
struct pipe_video_buffer *
vl_video_buffer_init(struct pipe_video_context *context,
                     struct pipe_context *pipe,
                     unsigned width, unsigned height, unsigned depth,
                     enum pipe_video_chroma_format chroma_format,
                     const enum pipe_format resource_formats[VL_MAX_PLANES],
                     unsigned usage);
#endif /* vl_ycbcr_buffer_h */
