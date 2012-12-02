/*
 * Copyright 2011 Maarten Lankhorst
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
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "nvc0_context.h"
#include "nvc0_screen.h"

#include "vl/vl_decoder.h"
#include "vl/vl_video_buffer.h"
#include "vl/vl_types.h"

#include "util/u_video.h"

struct nvc0_video_buffer {
   struct pipe_video_buffer base;
   unsigned num_planes, valid_ref;
   struct pipe_resource *resources[VL_NUM_COMPONENTS];
   struct pipe_sampler_view *sampler_view_planes[VL_NUM_COMPONENTS];
   struct pipe_sampler_view *sampler_view_components[VL_NUM_COMPONENTS];
   struct pipe_surface *surfaces[VL_NUM_COMPONENTS * 2];
};

static INLINE uint32_t nvc0_video_align(uint32_t h)
{
   return ((h+0x3f)&~0x3f);
};

static INLINE uint32_t mb(uint32_t coord)
{
   return (coord + 0xf)>>4;
}

static INLINE uint32_t mb_half(uint32_t coord)
{
   return (coord + 0x1f)>>5;
}
