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

#include "vdpau_private.h"
#include <pipe/p_screen.h>
#include <pipe/p_state.h>
#include <util/u_memory.h>
#include <util/u_format.h>

VdpStatus
vlVdpVideoSurfaceCreate(VdpDevice device, VdpChromaType chroma_type,
                        uint32_t width, uint32_t height,
                        VdpVideoSurface *surface)
{
   printf("[VDPAU] Creating a surface\n");

   vlVdpSurface *p_surf;
   VdpStatus ret;

   if (!(width && height)) {
      ret = VDP_STATUS_INVALID_SIZE;
      goto inv_size;
   }

   if (!vlCreateHTAB()) {
      ret = VDP_STATUS_RESOURCES;
      goto no_htab;
   }

   p_surf = CALLOC(1, sizeof(p_surf));
   if (!p_surf) {
      ret = VDP_STATUS_RESOURCES;
      goto no_res;
   }

   vlVdpDevice *dev = vlGetDataHTAB(device);
   if (!dev) {
      ret = VDP_STATUS_INVALID_HANDLE;
      goto inv_device;
   }

   p_surf->chroma_format = TypeToPipe(chroma_type);
   p_surf->device = dev;
   p_surf->width = width;
   p_surf->height = height;

   *surface = vlAddDataHTAB(p_surf);
   if (*surface == 0) {
      ret = VDP_STATUS_ERROR;
      goto no_handle;
   }

   return VDP_STATUS_OK;

no_handle:
   FREE(p_surf->psurface);
inv_device:
no_surf:
   FREE(p_surf);
no_res:
   // vlDestroyHTAB(); XXX: Do not destroy this tab, I think.
no_htab:
inv_size:
   return ret;
}

VdpStatus
vlVdpVideoSurfaceDestroy(VdpVideoSurface surface)
{
   vlVdpSurface *p_surf;

   p_surf = (vlVdpSurface *)vlGetDataHTAB((vlHandle)surface);
   if (!p_surf)
      return VDP_STATUS_INVALID_HANDLE;

   if (p_surf->psurface) {
      if (p_surf->psurface->texture) {
         if (p_surf->psurface->texture->screen)
            p_surf->psurface->context->surface_destroy(p_surf->psurface->context, p_surf->psurface);
      }
   }
   FREE(p_surf);
   return VDP_STATUS_OK;
}

VdpStatus
vlVdpVideoSurfaceGetParameters(VdpVideoSurface surface,
                               VdpChromaType *chroma_type,
                               uint32_t *width, uint32_t *height)
{
   if (!(width && height && chroma_type))
      return VDP_STATUS_INVALID_POINTER;

   vlVdpSurface *p_surf = vlGetDataHTAB(surface);
   if (!p_surf)
      return VDP_STATUS_INVALID_HANDLE;

   if (!(p_surf->chroma_format > 0 && p_surf->chroma_format < 3))
      return VDP_STATUS_INVALID_CHROMA_TYPE;

   *width = p_surf->width;
   *height = p_surf->height;
   *chroma_type = PipeToType(p_surf->chroma_format);

   return VDP_STATUS_OK;
}

VdpStatus
vlVdpVideoSurfaceGetBitsYCbCr(VdpVideoSurface surface,
                              VdpYCbCrFormat destination_ycbcr_format,
                              void *const *destination_data,
                              uint32_t const *destination_pitches)
{
   if (!vlCreateHTAB())
      return VDP_STATUS_RESOURCES;

   vlVdpSurface *p_surf = vlGetDataHTAB(surface);
   if (!p_surf)
      return VDP_STATUS_INVALID_HANDLE;

   if (!p_surf->psurface)
      return VDP_STATUS_RESOURCES;

   return VDP_STATUS_OK;
}

VdpStatus
vlVdpVideoSurfacePutBitsYCbCr(VdpVideoSurface surface,
                              VdpYCbCrFormat source_ycbcr_format,
                              void const *const *source_data,
                              uint32_t const *source_pitches)
{
   uint32_t size_surface_bytes;
   const struct util_format_description *format_desc;
   enum pipe_format pformat = FormatToPipe(source_ycbcr_format);

   if (!vlCreateHTAB())
      return VDP_STATUS_RESOURCES;

   vlVdpSurface *p_surf = vlGetDataHTAB(surface);
   if (!p_surf)
      return VDP_STATUS_INVALID_HANDLE;

   //size_surface_bytes = ( source_pitches[0] * p_surf->height util_format_get_blockheight(pformat) );
   /*util_format_translate(enum pipe_format dst_format,
   void *dst, unsigned dst_stride,
   unsigned dst_x, unsigned dst_y,
   enum pipe_format src_format,
   const void *src, unsigned src_stride,
   unsigned src_x, unsigned src_y,
   unsigned width, unsigned height);*/

   return VDP_STATUS_NO_IMPLEMENTATION;
}
