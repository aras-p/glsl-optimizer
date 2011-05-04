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

#include <assert.h>

#include <pipe/p_video_context.h>
#include <pipe/p_state.h>

#include <util/u_memory.h>
#include <util/u_debug.h>

#include "vdpau_private.h"

VdpStatus
vlVdpVideoSurfaceCreate(VdpDevice device, VdpChromaType chroma_type,
                        uint32_t width, uint32_t height,
                        VdpVideoSurface *surface)
{
   vlVdpSurface *p_surf;
   VdpStatus ret;

   VDPAU_MSG(VDPAU_TRACE, "[VDPAU] Creating a surface\n");

   if (!(width && height)) {
      ret = VDP_STATUS_INVALID_SIZE;
      goto inv_size;
   }

   if (!vlCreateHTAB()) {
      ret = VDP_STATUS_RESOURCES;
      goto no_htab;
   }

   p_surf = CALLOC(1, sizeof(vlVdpSurface));
   if (!p_surf) {
      ret = VDP_STATUS_RESOURCES;
      goto no_res;
   }

   vlVdpDevice *dev = vlGetDataHTAB(device);
   if (!dev) {
      ret = VDP_STATUS_INVALID_HANDLE;
      goto inv_device;
   }

   p_surf->device = dev;
   p_surf->video_buffer = dev->context->vpipe->create_buffer
   (
      dev->context->vpipe,
      PIPE_FORMAT_YV12, // most common used
      ChromaToPipe(chroma_type),
      width, height
   );

   *surface = vlAddDataHTAB(p_surf);
   if (*surface == 0) {
      ret = VDP_STATUS_ERROR;
      goto no_handle;
   }

   return VDP_STATUS_OK;

no_handle:
   p_surf->video_buffer->destroy(p_surf->video_buffer);

inv_device:
   FREE(p_surf);

no_res:
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

   if (p_surf->video_buffer)
      p_surf->video_buffer->destroy(p_surf->video_buffer);

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

   *width = p_surf->video_buffer->width;
   *height = p_surf->video_buffer->height;
   *chroma_type = PipeToChroma(p_surf->video_buffer->chroma_format);

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

   //if (!p_surf->psurface)
   //   return VDP_STATUS_RESOURCES;

   //return VDP_STATUS_OK;
   return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
vlVdpVideoSurfacePutBitsYCbCr(VdpVideoSurface surface,
                              VdpYCbCrFormat source_ycbcr_format,
                              void const *const *source_data,
                              uint32_t const *source_pitches)
{
   enum pipe_format pformat = FormatToPipe(source_ycbcr_format);
   struct pipe_video_context *context;
   struct pipe_sampler_view **sampler_views;
   unsigned i;

   if (!vlCreateHTAB())
      return VDP_STATUS_RESOURCES;

   vlVdpSurface *p_surf = vlGetDataHTAB(surface);
   if (!p_surf)
      return VDP_STATUS_INVALID_HANDLE;

   context = p_surf->device->context->vpipe;
   if (!context)
      return VDP_STATUS_INVALID_HANDLE;

   if (p_surf->video_buffer == NULL || pformat != p_surf->video_buffer->buffer_format) {
      assert(0); // TODO Recreate resource
      return VDP_STATUS_NO_IMPLEMENTATION;
   }

   sampler_views = p_surf->video_buffer->get_sampler_view_planes(p_surf->video_buffer);
   if (!sampler_views)
      return VDP_STATUS_RESOURCES;

   for (i = 0; i < 3; ++i) { //TODO put nr of planes into util format
      struct pipe_sampler_view *sv = sampler_views[i ? i ^ 3 : 0];
      struct pipe_box dst_box = { 0, 0, 0, sv->texture->width0, sv->texture->height0, 1 };
      context->upload_sampler(context, sv, &dst_box, source_data[i], source_pitches[i], 0, 0);
   }

   return VDP_STATUS_OK;
}
