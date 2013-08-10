/*
 * Copyright 2011-2013 Maarten Lankhorst
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "pipe/p_defines.h"
#include "vl/vl_video_buffer.h"

struct nouveau_vp3_video_buffer {
   struct pipe_video_buffer base;
   unsigned num_planes, valid_ref;
   struct pipe_resource *resources[VL_NUM_COMPONENTS];
   struct pipe_sampler_view *sampler_view_planes[VL_NUM_COMPONENTS];
   struct pipe_sampler_view *sampler_view_components[VL_NUM_COMPONENTS];
   struct pipe_surface *surfaces[VL_NUM_COMPONENTS * 2];
};

struct pipe_video_buffer *
nouveau_vp3_video_buffer_create(struct pipe_context *pipe,
                                const struct pipe_video_buffer *templat,
                                int flags);
