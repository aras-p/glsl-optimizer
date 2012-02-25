/**************************************************************************
 *
 * Copyright 2010 Thomas Balling Sørensen.
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

#include "util/u_memory.h"
#include "util/u_sampler.h"

#include "vdpau_private.h"

/**
 * Create a VdpBitmapSurface.
 */
VdpStatus
vlVdpBitmapSurfaceCreate(VdpDevice device,
                         VdpRGBAFormat rgba_format,
                         uint32_t width, uint32_t height,
                         VdpBool frequently_accessed,
                         VdpBitmapSurface *surface)
{
   struct pipe_context *pipe;
   struct pipe_resource res_tmpl, *res;
   struct pipe_sampler_view sv_templ;

   vlVdpBitmapSurface *vlsurface = NULL;

   if (!(width && height))
      return VDP_STATUS_INVALID_SIZE;

   vlVdpDevice *dev = vlGetDataHTAB(device);
   if (!dev)
      return VDP_STATUS_INVALID_HANDLE;

   pipe = dev->context;
   if (!pipe)
      return VDP_STATUS_INVALID_HANDLE;

   if (!surface)
      return VDP_STATUS_INVALID_POINTER;

   vlsurface = CALLOC(1, sizeof(vlVdpBitmapSurface));
   if (!vlsurface)
      return VDP_STATUS_RESOURCES;

   vlsurface->device = dev;

   memset(&res_tmpl, 0, sizeof(res_tmpl));

   res_tmpl.target = PIPE_TEXTURE_2D;
   res_tmpl.format = FormatRGBAToPipe(rgba_format);
   res_tmpl.width0 = width;
   res_tmpl.height0 = height;
   res_tmpl.depth0 = 1;
   res_tmpl.array_size = 1;
   res_tmpl.bind = PIPE_BIND_SAMPLER_VIEW | PIPE_BIND_RENDER_TARGET;
   res_tmpl.usage = frequently_accessed ? PIPE_USAGE_DYNAMIC : PIPE_USAGE_STATIC;
   res = pipe->screen->resource_create(pipe->screen, &res_tmpl);
   if (!res) {
      FREE(dev);
      return VDP_STATUS_RESOURCES;
   }

   memset(&sv_templ, 0, sizeof(sv_templ));
   u_sampler_view_default_template(&sv_templ, res, res->format);
   vlsurface->sampler_view = pipe->create_sampler_view(pipe, res, &sv_templ);
   if (!vlsurface->sampler_view) {
      pipe_resource_reference(&res, NULL);
      FREE(dev);
      return VDP_STATUS_RESOURCES;
   }

   *surface = vlAddDataHTAB(vlsurface);
   if (*surface == 0) {
      pipe_resource_reference(&res, NULL);
      FREE(dev);
      return VDP_STATUS_ERROR;
   }

   pipe_resource_reference(&res, NULL);

   return VDP_STATUS_OK;
}

/**
 * Destroy a VdpBitmapSurface.
 */
VdpStatus
vlVdpBitmapSurfaceDestroy(VdpBitmapSurface surface)
{
   vlVdpBitmapSurface *vlsurface;

   vlsurface = vlGetDataHTAB(surface);
   if (!vlsurface)
      return VDP_STATUS_INVALID_HANDLE;

   vlVdpResolveDelayedRendering(vlsurface->device, NULL, NULL);

   pipe_sampler_view_reference(&vlsurface->sampler_view, NULL);

   vlRemoveDataHTAB(surface);
   FREE(vlsurface);

   return VDP_STATUS_OK;
}

/**
 * Retrieve the parameters used to create a VdpBitmapSurface.
 */
VdpStatus
vlVdpBitmapSurfaceGetParameters(VdpBitmapSurface surface,
                                VdpRGBAFormat *rgba_format,
                                uint32_t *width, uint32_t *height,
                                VdpBool *frequently_accessed)
{
   if (!(rgba_format && width && height && frequently_accessed))
      return VDP_STATUS_INVALID_POINTER;

   return VDP_STATUS_NO_IMPLEMENTATION;
}

/**
 * Copy image data from application memory in the surface's native format to
 * a VdpBitmapSurface.
 */
VdpStatus
vlVdpBitmapSurfacePutBitsNative(VdpBitmapSurface surface,
                                void const *const *source_data,
                                uint32_t const *source_pitches,
                                VdpRect const *destination_rect)
{
   vlVdpBitmapSurface *vlsurface;
   struct pipe_box dst_box;
   struct pipe_context *pipe;

   vlsurface = vlGetDataHTAB(surface);
   if (!vlsurface)
      return VDP_STATUS_INVALID_HANDLE;

   if (!(source_data && source_pitches))
      return VDP_STATUS_INVALID_POINTER;

   pipe = vlsurface->device->context;

   vlVdpResolveDelayedRendering(vlsurface->device, NULL, NULL);

   dst_box.x = 0;
   dst_box.y = 0;
   dst_box.z = 0;
   dst_box.width = vlsurface->sampler_view->texture->width0;
   dst_box.height = vlsurface->sampler_view->texture->height0;
   dst_box.depth = 1;

   if (destination_rect) {
      dst_box.x = MIN2(destination_rect->x0, destination_rect->x1);
      dst_box.y = MIN2(destination_rect->y0, destination_rect->y1);
      dst_box.width = abs(destination_rect->x1 - destination_rect->x0);
      dst_box.height = abs(destination_rect->y1 - destination_rect->y0);
   }

   pipe->transfer_inline_write(pipe, vlsurface->sampler_view->texture, 0,
                               PIPE_TRANSFER_WRITE, &dst_box, *source_data,
                               *source_pitches, 0);
   return VDP_STATUS_OK;
}
