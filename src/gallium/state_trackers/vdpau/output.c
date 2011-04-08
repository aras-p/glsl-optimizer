/**************************************************************************
 *
 * Copyright 2010 Thomas Balling Sørensen.
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

#include <vdpau/vdpau.h>

#include <util/u_debug.h>
#include <util/u_memory.h>

#include "vdpau_private.h"

VdpStatus
vlVdpOutputSurfaceCreate(VdpDevice device,
                         VdpRGBAFormat rgba_format,
                         uint32_t width, uint32_t height,
                         VdpOutputSurface  *surface)
{
   struct pipe_video_context *context;
   struct pipe_resource res_tmpl, *res;
   struct pipe_sampler_view sv_templ;
   struct pipe_surface surf_templ;

   vlVdpOutputSurface *vlsurface = NULL;

   debug_printf("[VDPAU] Creating output surface\n");
   if (!(width && height))
      return VDP_STATUS_INVALID_SIZE;

   vlVdpDevice *dev = vlGetDataHTAB(device);
   if (!dev)
      return VDP_STATUS_INVALID_HANDLE;

   context = dev->context->vpipe;
   if (!context)
      return VDP_STATUS_INVALID_HANDLE;

   vlsurface = CALLOC(1, sizeof(vlVdpOutputSurface));
   if (!vlsurface)
      return VDP_STATUS_RESOURCES;

   memset(&res_tmpl, 0, sizeof(res_tmpl));

   res_tmpl.target = PIPE_TEXTURE_2D;
   res_tmpl.format = FormatRGBAToPipe(rgba_format);
   res_tmpl.width0 = width;
   res_tmpl.height0 = height;
   res_tmpl.depth0 = 1;
   res_tmpl.array_size = 1;
   res_tmpl.bind = PIPE_BIND_SAMPLER_VIEW | PIPE_BIND_RENDER_TARGET;
   res_tmpl.usage = PIPE_USAGE_STATIC;

   res = context->screen->resource_create(context->screen, &res_tmpl);
   if (!res) {
      FREE(dev);
      return VDP_STATUS_ERROR;
   }

   memset(&sv_templ, 0, sizeof(sv_templ));
   u_sampler_view_default_template(&sv_templ, res, res->format);
   vlsurface->sampler_view = context->create_sampler_view(context, res, &sv_templ);
   if (!vlsurface->sampler_view) {
      FREE(dev);
      return VDP_STATUS_ERROR;
   }

   memset(&surf_templ, 0, sizeof(surf_templ));
   surf_templ.format = res->format;
   surf_templ.usage = PIPE_BIND_SAMPLER_VIEW | PIPE_BIND_RENDER_TARGET;
   vlsurface->surface = context->create_surface(context, res, &surf_templ);
   if (!vlsurface->surface) {
      FREE(dev);
      return VDP_STATUS_ERROR;
   }

   *surface = vlAddDataHTAB(vlsurface);
   if (*surface == 0) {
      FREE(dev);
      return VDP_STATUS_ERROR;
   }

   return VDP_STATUS_OK;
}
