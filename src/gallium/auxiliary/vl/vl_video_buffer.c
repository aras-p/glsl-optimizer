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

#include <assert.h>

#include <pipe/p_screen.h>
#include <pipe/p_context.h>
#include <pipe/p_state.h>

#include <util/u_format.h>
#include <util/u_inlines.h>
#include <util/u_sampler.h>
#include <util/u_memory.h>

#include "vl_video_buffer.h"

static inline void
adjust_swizzle(struct pipe_sampler_view *sv_templ)
{
   if (util_format_get_nr_components(sv_templ->format) == 1) {
      sv_templ->swizzle_r = PIPE_SWIZZLE_RED;
      sv_templ->swizzle_g = PIPE_SWIZZLE_RED;
      sv_templ->swizzle_b = PIPE_SWIZZLE_RED;
      sv_templ->swizzle_a = PIPE_SWIZZLE_RED;
   }
}

static void
vl_video_buffer_destroy(struct pipe_video_buffer *buffer)
{
   struct vl_video_buffer *buf = (struct vl_video_buffer *)buffer;
   unsigned i;

   assert(buf);

   for (i = 0; i < VL_MAX_PLANES; ++i) {
      pipe_surface_reference(&buf->surfaces[i], NULL);
      pipe_sampler_view_reference(&buf->sampler_views[i], NULL);
      pipe_resource_reference(&buf->resources[i], NULL);
   }
}

static struct pipe_sampler_view **
vl_video_buffer_sampler_views(struct pipe_video_buffer *buffer)
{
   struct vl_video_buffer *buf = (struct vl_video_buffer *)buffer;
   struct pipe_sampler_view sv_templ;
   struct pipe_context *pipe;
   unsigned i;

   assert(buf);

   pipe = buf->pipe;

   for (i = 0; i < buf->num_planes; ++i ) {
      if (!buf->sampler_views[i]) {
         memset(&sv_templ, 0, sizeof(sv_templ));
         u_sampler_view_default_template(&sv_templ, buf->resources[i], buf->resources[i]->format);
         adjust_swizzle(&sv_templ);
         buf->sampler_views[i] = pipe->create_sampler_view(pipe, buf->resources[i], &sv_templ);
         if (!buf->sampler_views[i])
            goto error;
      }
   }

   return buf->sampler_views;

error:
   for (i = 0; i < buf->num_planes; ++i )
      pipe_sampler_view_reference(&buf->sampler_views[i], NULL);

   return NULL;
}

static struct pipe_surface **
vl_video_buffer_surfaces(struct pipe_video_buffer *buffer)
{
   struct vl_video_buffer *buf = (struct vl_video_buffer *)buffer;
   struct pipe_surface surf_templ;
   struct pipe_context *pipe;
   unsigned i;

   assert(buf);

   pipe = buf->pipe;

   for (i = 0; i < buf->num_planes; ++i ) {
      if (!buf->surfaces[i]) {
         memset(&surf_templ, 0, sizeof(surf_templ));
         surf_templ.format = buf->resources[i]->format;
         surf_templ.usage = PIPE_BIND_SAMPLER_VIEW | PIPE_BIND_RENDER_TARGET;
         buf->surfaces[i] = pipe->create_surface(pipe, buf->resources[i], &surf_templ);
         if (!buf->surfaces[i])
            goto error;
      }
   }

   return buf->surfaces;

error:
   for (i = 0; i < buf->num_planes; ++i )
      pipe_surface_reference(&buf->surfaces[i], NULL);

   return NULL;
}

struct pipe_video_buffer *
vl_video_buffer_init(struct pipe_video_context *context,
                     struct pipe_context *pipe,
                     unsigned width, unsigned height, unsigned depth,
                     enum pipe_video_chroma_format chroma_format,
                     unsigned num_planes,
                     const enum pipe_format resource_formats[VL_MAX_PLANES],
                     unsigned usage)
{
   struct vl_video_buffer *buffer;
   struct pipe_resource templ;
   unsigned i;

   assert(context && pipe);
   assert(num_planes > 0 && num_planes <= VL_MAX_PLANES);

   buffer = CALLOC_STRUCT(vl_video_buffer);

   buffer->base.destroy = vl_video_buffer_destroy;
   buffer->base.get_sampler_views = vl_video_buffer_sampler_views;
   buffer->base.get_surfaces = vl_video_buffer_surfaces;
   buffer->pipe = pipe;
   buffer->num_planes = num_planes;

   memset(&templ, 0, sizeof(templ));
   templ.target = PIPE_TEXTURE_2D;
   templ.format = resource_formats[0];
   templ.width0 = width;
   templ.height0 = height;
   templ.depth0 = depth;
   templ.array_size = 1;
   templ.bind = PIPE_BIND_SAMPLER_VIEW | PIPE_BIND_RENDER_TARGET;
   templ.usage = usage;

   buffer->resources[0] = pipe->screen->resource_create(pipe->screen, &templ);
   if (!buffer->resources[0])
      goto error;

   if (num_planes == 1) {
      assert(chroma_format == PIPE_VIDEO_CHROMA_FORMAT_444);
      return &buffer->base;
   }

   templ.format = resource_formats[1];
   if (chroma_format == PIPE_VIDEO_CHROMA_FORMAT_420) {
      if (depth > 1)
         templ.depth0 /= 2;
      else
         templ.width0 /= 2;
      templ.height0 /= 2;
   } else if (chroma_format == PIPE_VIDEO_CHROMA_FORMAT_422) {
      if (depth > 1)
         templ.depth0 /= 2;
      else
         templ.height0 /= 2;
   }

   buffer->resources[1] = pipe->screen->resource_create(pipe->screen, &templ);
   if (!buffer->resources[1])
      goto error;

   if (num_planes == 2)
      return &buffer->base;

   templ.format = resource_formats[2];
   buffer->resources[2] = pipe->screen->resource_create(pipe->screen, &templ);
   if (!buffer->resources[2])
      goto error;

   return &buffer->base;

error:
   for (i = 0; i < VL_MAX_PLANES; ++i)
      pipe_resource_reference(&buffer->resources[i], NULL);
   FREE(buffer);

   return NULL;
}
