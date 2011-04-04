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

#include "vl_video_buffer.h"
#include <util/u_format.h>
#include <util/u_inlines.h>
#include <util/u_sampler.h>
#include <pipe/p_screen.h>
#include <pipe/p_context.h>
#include <assert.h>

bool vl_video_buffer_init(struct vl_video_buffer *buffer,
                          struct pipe_context *pipe,
                          unsigned width, unsigned height, unsigned depth,
                          enum pipe_video_chroma_format chroma_format,
                          unsigned num_planes,
                          const enum pipe_format resource_format[VL_MAX_PLANES],
                          unsigned usage)
{
   struct pipe_resource templ;
   unsigned i;

   assert(buffer && pipe);
   assert(num_planes > 0 && num_planes <= VL_MAX_PLANES);

   memset(buffer, 0, sizeof(struct vl_video_buffer));
   buffer->pipe = pipe;
   buffer->num_planes = num_planes;

   memset(&templ, 0, sizeof(templ));
   templ.target = PIPE_TEXTURE_2D;
   templ.format = resource_format[0];
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
      return true;
   }

   templ.format = resource_format[1];
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
      return true;

   templ.format = resource_format[2];
   buffer->resources[2] = pipe->screen->resource_create(pipe->screen, &templ);
   if (!buffer->resources[2])
      goto error;

   return true;

error:
   for (i = 0; i < VL_MAX_PLANES; ++i)
      pipe_resource_reference(&buffer->resources[i], NULL);

   return false;
}

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

vl_sampler_views *vl_video_buffer_sampler_views(struct vl_video_buffer *buffer)
{
   struct pipe_sampler_view sv_templ;
   struct pipe_context *pipe;
   unsigned i;

   assert(buffer);

   pipe = buffer->pipe;

   for (i = 0; i < buffer->num_planes; ++i ) {
      if (!buffer->sampler_views[i]) {
         memset(&sv_templ, 0, sizeof(sv_templ));
         u_sampler_view_default_template(&sv_templ, buffer->resources[i], buffer->resources[i]->format);
         adjust_swizzle(&sv_templ);
         buffer->sampler_views[i] = pipe->create_sampler_view(pipe, buffer->resources[i], &sv_templ);
         if (!buffer->sampler_views[i])
            goto error;
      }
   }

   return &buffer->sampler_views;

error:
   for (i = 0; i < buffer->num_planes; ++i )
      pipe_sampler_view_reference(&buffer->sampler_views[i], NULL);

   return NULL;
}

vl_surfaces *vl_video_buffer_surfaces(struct vl_video_buffer *buffer)
{
   struct pipe_surface surf_templ;
   struct pipe_context *pipe;
   unsigned i;

   assert(buffer);

   pipe = buffer->pipe;

   for (i = 0; i < buffer->num_planes; ++i ) {
      if (!buffer->surfaces[i]) {
         memset(&surf_templ, 0, sizeof(surf_templ));
         surf_templ.format = buffer->resources[i]->format;
         surf_templ.usage = PIPE_BIND_SAMPLER_VIEW | PIPE_BIND_RENDER_TARGET;
         buffer->surfaces[i] = pipe->create_surface(pipe, buffer->resources[i], &surf_templ);
         if (!buffer->surfaces[i])
            goto error;
      }
   }

   return &buffer->surfaces;

error:
   for (i = 0; i < buffer->num_planes; ++i )
      pipe_surface_reference(&buffer->surfaces[i], NULL);

   return NULL;
}

void vl_video_buffer_cleanup(struct vl_video_buffer *buffer)
{
   unsigned i;

   assert(buffer);

   for (i = 0; i < VL_MAX_PLANES; ++i) {
      pipe_surface_reference(&buffer->surfaces[i], NULL);
      pipe_sampler_view_reference(&buffer->sampler_views[i], NULL);
      pipe_resource_reference(&buffer->resources[i], NULL);
   }
}
