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

   VDPAU_MSG(VDPAU_TRACE, "[VDPAU] Creating output surface\n");
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

VdpStatus
vlVdpOutputSurfaceDestroy(VdpOutputSurface surface)
{
   vlVdpOutputSurface *vlsurface;

   VDPAU_MSG(VDPAU_TRACE, "[VDPAU] Destroying output surface\n");

   vlsurface = vlGetDataHTAB(surface);
   if (!vlsurface)
      return VDP_STATUS_INVALID_HANDLE;

   pipe_surface_reference(&vlsurface->surface, NULL);
   pipe_sampler_view_reference(&vlsurface->sampler_view, NULL);

   vlRemoveDataHTAB(surface);
   FREE(vlsurface);

   return VDP_STATUS_OK;
}

VdpStatus
vlVdpOutputSurfaceGetParameters(VdpOutputSurface surface,
                                VdpRGBAFormat *rgba_format,
                                uint32_t *width, uint32_t *height)
{
   return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
vlVdpOutputSurfaceGetBitsNative(VdpOutputSurface surface,
                                VdpRect const *source_rect,
                                void *const *destination_data,
                                uint32_t const *destination_pitches)
{
   return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
vlVdpOutputSurfacePutBitsNative(VdpOutputSurface surface,
                                void const *const *source_data,
                                uint32_t const *source_pitches,
                                VdpRect const *destination_rect)
{
   return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
vlVdpOutputSurfacePutBitsIndexed(VdpOutputSurface surface,
                                 VdpIndexedFormat source_indexed_format,
                                 void const *const *source_data,
                                 uint32_t const *source_pitch,
                                 VdpRect const *destination_rect,
                                 VdpColorTableFormat color_table_format,
                                 void const *color_table)
{
   return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
vlVdpOutputSurfacePutBitsYCbCr(VdpOutputSurface surface,
                               VdpYCbCrFormat source_ycbcr_format,
                               void const *const *source_data,
                               uint32_t const *source_pitches,
                               VdpRect const *destination_rect,
                               VdpCSCMatrix const *csc_matrix)
{
   return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
vlVdpOutputSurfaceRenderOutputSurface(VdpOutputSurface destination_surface,
                                      VdpRect const *destination_rect,
                                      VdpOutputSurface source_surface,
                                      VdpRect const *source_rect,
                                      VdpColor const *colors,
                                      VdpOutputSurfaceRenderBlendState const *blend_state,
                                      uint32_t flags)
{
   return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
vlVdpOutputSurfaceRenderBitmapSurface(VdpOutputSurface destination_surface,
                                      VdpRect const *destination_rect,
                                      VdpBitmapSurface source_surface,
                                      VdpRect const *source_rect,
                                      VdpColor const *colors,
                                      VdpOutputSurfaceRenderBlendState const *blend_state,
                                      uint32_t flags)
{
   return VDP_STATUS_NO_IMPLEMENTATION;
}
