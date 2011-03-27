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

#include "vl_ycbcr_buffer.h"
#include <util/u_format.h>
#include <util/u_inlines.h>
#include <util/u_sampler.h>
#include <pipe/p_screen.h>
#include <pipe/p_context.h>
#include <assert.h>

bool vl_ycbcr_buffer_init(struct vl_ycbcr_buffer *buffer,
                          struct pipe_context *pipe,
                          unsigned width, unsigned height,
                          enum pipe_video_chroma_format chroma_format,
                          enum pipe_format resource_format,
                          unsigned usage)
{
   struct pipe_resource templ;

   assert(buffer && pipe);

   memset(buffer, 0, sizeof(struct vl_ycbcr_buffer));
   buffer->pipe = pipe;

   memset(&templ, 0, sizeof(templ));
   templ.target = PIPE_TEXTURE_2D;
   templ.format = resource_format;
   templ.width0 = width / util_format_get_nr_components(resource_format);
   templ.height0 = height;
   templ.depth0 = 1;
   templ.array_size = 1;
   templ.bind = PIPE_BIND_SAMPLER_VIEW | PIPE_BIND_RENDER_TARGET;
   templ.usage = usage;

   buffer->resources.y = pipe->screen->resource_create(pipe->screen, &templ);
   if (!buffer->resources.y)
      goto error_resource_y;

   if (chroma_format == PIPE_VIDEO_CHROMA_FORMAT_420) {
      templ.width0 /= 2;
      templ.height0 /= 2;
   } else if (chroma_format == PIPE_VIDEO_CHROMA_FORMAT_422) {
      templ.height0 /= 2;
   }

   buffer->resources.cb = pipe->screen->resource_create(pipe->screen, &templ);
   if (!buffer->resources.cb)
      goto error_resource_cb;

   buffer->resources.cr = pipe->screen->resource_create(pipe->screen, &templ);
   if (!buffer->resources.cr)
      goto error_resource_cr;

   return true;

error_resource_cr:
   pipe_resource_reference(&buffer->resources.cb, NULL);

error_resource_cb:
   pipe_resource_reference(&buffer->resources.y, NULL);

error_resource_y:
   return false;
}

struct vl_ycbcr_sampler_views *vl_ycbcr_get_sampler_views(struct vl_ycbcr_buffer *buffer)
{
   struct pipe_sampler_view sv_templ;
   struct pipe_context *pipe;

   assert(buffer);

   pipe = buffer->pipe;

   if (!buffer->sampler_views.y) {
      memset(&sv_templ, 0, sizeof(sv_templ));
      u_sampler_view_default_template(&sv_templ, buffer->resources.y, buffer->resources.y->format);
      buffer->sampler_views.y = pipe->create_sampler_view(pipe, buffer->resources.y, &sv_templ);
      if (!buffer->sampler_views.y)
         goto error;
   }

   if (!buffer->sampler_views.cb) {
      memset(&sv_templ, 0, sizeof(sv_templ));
      u_sampler_view_default_template(&sv_templ, buffer->resources.cb, buffer->resources.cb->format);
      buffer->sampler_views.cb = pipe->create_sampler_view(pipe, buffer->resources.cb, &sv_templ);
      if (!buffer->sampler_views.cb)
         goto error;
   }

   if (!buffer->sampler_views.cr) {
      memset(&sv_templ, 0, sizeof(sv_templ));
      u_sampler_view_default_template(&sv_templ, buffer->resources.cr, buffer->resources.cr->format);
      buffer->sampler_views.cr = pipe->create_sampler_view(pipe, buffer->resources.cr, &sv_templ);
      if (!buffer->sampler_views.cr)
         goto error;
   }

   return &buffer->sampler_views;

error:
   pipe_sampler_view_reference(&buffer->sampler_views.y, NULL);
   pipe_sampler_view_reference(&buffer->sampler_views.cb, NULL);
   pipe_sampler_view_reference(&buffer->sampler_views.cr, NULL);
   return NULL;
}

struct vl_ycbcr_surfaces *vl_ycbcr_get_surfaces(struct vl_ycbcr_buffer *buffer)
{
   struct pipe_surface surf_templ;
   struct pipe_context *pipe;

   assert(buffer);

   pipe = buffer->pipe;

   if (!buffer->surfaces.y) {
      memset(&surf_templ, 0, sizeof(surf_templ));
      surf_templ.format = buffer->resources.y->format;
      surf_templ.usage = PIPE_BIND_SAMPLER_VIEW | PIPE_BIND_RENDER_TARGET;
      buffer->surfaces.y = pipe->create_surface(pipe, buffer->resources.y, &surf_templ);
      if (!buffer->surfaces.y)
         goto error;
   }

   if (!buffer->surfaces.cb) {
      memset(&surf_templ, 0, sizeof(surf_templ));
      surf_templ.format = buffer->resources.cb->format;
      surf_templ.usage = PIPE_BIND_SAMPLER_VIEW | PIPE_BIND_RENDER_TARGET;
      buffer->surfaces.cb = pipe->create_surface(pipe, buffer->resources.cb, &surf_templ);
      if (!buffer->surfaces.cb)
         goto error;
   }

   if (!buffer->surfaces.cr) {
      memset(&surf_templ, 0, sizeof(surf_templ));
      surf_templ.format = buffer->resources.cr->format;
      surf_templ.usage = PIPE_BIND_SAMPLER_VIEW | PIPE_BIND_RENDER_TARGET;
      buffer->surfaces.cr = pipe->create_surface(pipe, buffer->resources.cr, &surf_templ);
      if (!buffer->surfaces.cr)
         goto error;
   }

   return &buffer->surfaces;

error:
   pipe_surface_reference(&buffer->surfaces.y, NULL);
   pipe_surface_reference(&buffer->surfaces.cb, NULL);
   pipe_surface_reference(&buffer->surfaces.cr, NULL);
   return NULL;
}

void vl_ycbcr_buffer_cleanup(struct vl_ycbcr_buffer *buffer)
{
   pipe_surface_reference(&buffer->surfaces.y, NULL);
   pipe_surface_reference(&buffer->surfaces.cb, NULL);
   pipe_surface_reference(&buffer->surfaces.cr, NULL);

   pipe_sampler_view_reference(&buffer->sampler_views.y, NULL);
   pipe_sampler_view_reference(&buffer->sampler_views.cb, NULL);
   pipe_sampler_view_reference(&buffer->sampler_views.cr, NULL);

   pipe_resource_reference(&buffer->resources.y, NULL);
   pipe_resource_reference(&buffer->resources.cb, NULL);
   pipe_resource_reference(&buffer->resources.cr, NULL);
}
